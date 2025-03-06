#include "RainGaugeApp.h"
#include "eink.h"
#include "main.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f1xx_hal_rtc.h"
#ifdef FAST_SEMIHOSTING_STDIO_DRIVER
#include "FastSemihosting.h"
#endif



#define countof(x)		(sizeof(x)/sizeof(x[0]))

extern RTC_HandleTypeDef hrtc;

static int bucket_dumps = 0;
static int bucket_dumps_offset = 0;
static int last_bucket_dumps = -1;
static uint32_t last_dump_time = 0;

// daily	24*10 - 240 - each 6 minutes
#define DAILY_RAIN_SAMPLES		240
static uint32_t daily_rain[DAILY_RAIN_SAMPLES];
static uint8_t daily_rain_count = 0;
static uint32_t yesterday_rain[DAILY_RAIN_SAMPLES];
	
// weekly 7 * 24 - 168 - each 60 minutes
#define WEEKLY_RAIN_SAMPLES		168
static uint32_t weekly_rain[WEEKLY_RAIN_SAMPLES];
static uint8_t weekly_rain_count = 0;

// -yearly n - 1 per day.This won't display without anti-aliasing, or multiple pixels per day.
#define YEARLY_RAIN_SAMPLES		365
static uint32_t yearly_rain[YEARLY_RAIN_SAMPLES];
static uint16_t yearly_rain_count = 0;

static uint32_t mm_per_dump = 100;		// mm of rain per bucket dump.  Scaled * 1000

#define BUCKET_DEBOUNCE			1
#define PRESS_TIME				50
#define LONG_PRESS_TIME			2000
#define LONG_LONG_PRESS_TIME	5000

#define REFRESH_DELAY	60					// Wait at least this long (seconds) after bucket tip before refreshing the display
#define SLEEP_TIME		60					// Main loop sleep time
//#define SLEEP_TIME		5

enum Events
{
	BUTTON1_PRESS = 1,
	BUTTON1_LONG_PRESS,
	BUTTON1_LONG_LONG_PRESS,
	BUTTON2_PRESS,
	BUTTON2_LONG_PRESS,
	BUTTON2_LONG_LONG_PRESS,
};

enum Keys
{
	KEY_BUTTON1 = 1,
	KEY_BUTTON2 = 2,
	KEY_BUCKET  = 4
};


static Queue<uint8_t, 16> ButtonQueue;
static uint8_t keydown = 0;

static EInkDisplay eink;
	


static enum DisplayMode
{
	Data,
	Menu,
	Edit
} display_mode = Data;

static enum Display
{
	Number,
	Today,
	Yesterday,
	Week,
	Year,
	MAX_DISPLAY
} screen_display = Number;

static int menu_item = 0;


const char *menu_list[] = { 
	"Clear Week",
	"Clear All",
	"Set Date",
	"Set Time",
	"Set mm per tip",
	"About"
};

enum EMenuItem
{
	ClearWeek,
	ClearAll,
	SetDate,
	SetTime,
	SetMmPerTip,
	AboutScreen
};


struct Field
{
	int x, y, w, h;
	int min, max;
};

struct IntEdit
{
	const char * title;
	uint8_t count;
	Field fields[];
};

// Size and positioning for arial8b
const IntEdit set_date = { 
	"Set Date",
	3,
	{
		{ 40, 50, 26, 20, 1, 31 },
		{ 70, 50, 26, 20, 1, 12 },
		{100, 50, 46, 20, 2025, 2100 }
	}
};

const IntEdit set_time = { 
	"Set Time",
	2,
	{
		{ 40, 50, 26, 20, 0, 23 },
		{ 70, 50, 26, 20, 0, 59 }
	}
};

const IntEdit set_tip = { 
	"Set mm per bucket tip",
	4,
	{
		{ 40, 50, 16, 20, 0, 9 },
		{ 70, 50, 16, 20, 0, 9 },
		{ 90, 50, 16, 20, 0, 9 },
		{110, 50, 16, 20, 0, 9 }
	}
};

static int edit_field = 0;
static int values[4];




// From Drivers\STM32F1xx_HAL_Driver\Src\stm32f1xx_hal_rtc.c
static uint32_t RTC_ReadTimeCounter(RTC_HandleTypeDef *hrtc)
{
	uint16_t high1 = 0U, high2 = 0U, low = 0U;
	uint32_t timecounter = 0U;

	high1 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);
	low   = READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT);
	high2 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);

	if (high1 != high2)
	{
		/* In this case the counter roll over during reading of CNTL and CNTH registers,
		   read again CNTL register then return the counter value */
		timecounter = (((uint32_t) high2 << 16U) | READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT));
	}
	else
	{
		/* No counter roll over during reading of CNTL and CNTH registers, counter
		   value is equal to first value of CNTL and CNTH */
		timecounter = (((uint32_t) high1 << 16U) | low);
	}

	return timecounter;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//Debug("GPIO_Pin: %d\n", GPIO_Pin);
	if (GPIO_Pin & GPIO_BUCKET_Pin)
	{
		//static uint32_t time_pressed = 0;
		if (HAL_GPIO_ReadPin(GPIO_BUCKET_GPIO_Port, GPIO_BUCKET_Pin))
		{
			keydown &= ~KEY_BUCKET;
			//uint32_t time = HAL_GetTick() - time_pressed;
			//if (time > BUCKET_DEBOUNCE)
			{
				bucket_dumps++;
				Debug("dump %d\n", bucket_dumps);
			}
		}
		else
		{
			keydown |= KEY_BUCKET;
			//time_pressed = HAL_GetTick();
		}
	}
	if (GPIO_Pin & GPIO_BUTTON1_Pin)
	{
		static uint32_t time_pressed=0;
		if (HAL_GPIO_ReadPin(GPIO_BUTTON1_GPIO_Port, GPIO_BUTTON1_Pin))
		{
			keydown &= ~KEY_BUTTON1;
			uint32_t time = HAL_GetTick() - time_pressed;
			if (time > LONG_LONG_PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON1_LONG_LONG_PRESS);
			else if (time > LONG_PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON1_LONG_PRESS);
			else if (time > PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON1_PRESS);
		}
		else
		{
			keydown |= KEY_BUTTON1;
			time_pressed = HAL_GetTick();
		}
	}
	if (GPIO_Pin & GPIO_BUTTON2_Pin)
	{
		static uint32_t time_pressed=0;
		if (HAL_GPIO_ReadPin(GPIO_BUTTON2_GPIO_Port, GPIO_BUTTON2_Pin))
		{
			keydown &= ~KEY_BUTTON2;
			uint32_t time = HAL_GetTick() - time_pressed;
			if (time > LONG_LONG_PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON2_LONG_LONG_PRESS);
			else if (time > LONG_PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON2_LONG_PRESS);
			else if (time > PRESS_TIME)
				ButtonQueue.AddEvent(BUTTON2_PRESS);
		}
		else
		{
			keydown |= KEY_BUTTON2;
			time_pressed = HAL_GetTick();
		}
	}
}

/// @brief  Determine if we should update the display.  
bool RefreshDisplay()
{
	if (last_bucket_dumps == -1)
	{
		return true;
	}
	
	uint32_t time = RTC_ReadTimeCounter(&hrtc);
	if ( bucket_dumps != last_bucket_dumps )
	{
		if ( last_dump_time == 0 )
		{
			last_dump_time = time;
		}
	}

	if (last_dump_time != 0 && time - last_dump_time > REFRESH_DELAY)
	{
		last_dump_time = 0;
		return true;
	}
	
	return false;
}

void add_seconds(RTC_TimeTypeDef *time, int seconds)
{
    time->Seconds += seconds;
    while (time->Seconds >= 60)
    {
        time->Seconds -= 60;
        time->Minutes++;
        while (time->Minutes >= 60)
        {
            time->Minutes -= 60;
            time->Hours++;
            while (time->Hours >= 24)
            {
                time->Hours -= 24;
            }
        }
    }
}


// number is value*10
static void DisplayNumber(int number)
{
	last_bucket_dumps = bucket_dumps;

	eink.InitDisplay();
	eink.ClearDisplay();
		
	int top = 6; //(eink.height - arial_narrow_280.height_pixels) / 2;
	int bottom = top + arial_narrow_280.height_pixels + 1;
	int left = (eink.width - 4 * (arial_narrow_280.f.width_pixels + 3 * arial_narrow_280.char_space + 2*(arial_narrow_280.height_pixels/4))) / 2;
	if (left < 0)
		left = 0;
	eink.DrawNumberX10(left, top, number, &arial_narrow_280, BLACK, WHITE);

	char buf[20];
	RTC_DateTypeDef date;
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	RTC_TimeTypeDef time;
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);

	snprintf(buf, sizeof(buf), "%02d:%02d %d/%d/%02d", time.Hours, time.Minutes, date.Date, date.Month, date.Year);
	eink.DrawString(0, eink.height - 12, buf, &Arial8, BLACK, WHITE);
	
	// show raw counts at bottom right
	//snprintf(buf, sizeof(buf), "%d", bucket_dumps);
	//int len = eink.StringLength(&Arial8, buf);
	//eink.DrawString( eink.width - 1 - len, eink.height - 12, buf, &Arial8, BLACK, WHITE);
	
	eink.CommitDisplay();
}
	
int ScaleDump(int n)
{
	uint64_t scaled_dump = 10 * n * mm_per_dump / 1000;	// scaled_dump is mm*10
	return n;
}

static void UpdateDisplay()
{
	switch (screen_display)
	{
		case Number:
			DisplayNumber(ScaleDump(bucket_dumps - bucket_dumps_offset));
			break;
		case Today:
			eink.DrawGraph("Today", daily_rain, daily_rain_count, DAILY_RAIN_SAMPLES, 8, ScaleDump(daily_rain[daily_rain_count-1] - daily_rain[0]));
			break;
		case Yesterday:
			eink.DrawGraph("Yesterday", yesterday_rain, DAILY_RAIN_SAMPLES, DAILY_RAIN_SAMPLES, 8, ScaleDump(yesterday_rain[DAILY_RAIN_SAMPLES-1] - yesterday_rain[0]));
			break;
		case Week:
			eink.DrawGraph("Week", weekly_rain, weekly_rain_count, WEEKLY_RAIN_SAMPLES, 7, ScaleDump(weekly_rain[weekly_rain_count-1] - weekly_rain[0]));
			break;
		case Year:
			eink.DrawGraph("Year", yearly_rain, yearly_rain_count, YEARLY_RAIN_SAMPLES, 12, ScaleDump(yearly_rain[yearly_rain_count-1] - yearly_rain[0]));
			break;
	}
}

	
static void DrawMenuItem(int index, int highlighted)
{
	int h = Arial8.height_pixels + 2;
	int len = eink.StringLength(&Arial8, menu_list[index]);
	if ( highlighted )
	{
		eink.DrawRectangle(0, 2+(index + 1)*h - 1, len + 1, (index + 2)*h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		eink.DrawString(0, 2+(index + 1)*h, menu_list[index], &Arial8, WHITE, BLACK);
	}
	else
	{
		eink.DrawRectangle(0, 2+(index + 1)*h - 1, len + 1, (index + 2)*h, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		eink.DrawString(0, 2+(index + 1)*h, menu_list[index], &Arial8, BLACK, WHITE);
	}
}	

static void DrawMenu( int highlighted = -1 )
{
	eink.InitDisplay();
	eink.ClearDisplay();

	eink.DrawRectangle(0, 0, eink.width - 1, Arial8.height_pixels + 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	eink.DrawString(90, 1, "Menu", &Arial8, WHITE, BLACK);
	for (int i = 0; i < countof(menu_list); i++)
	{
		DrawMenuItem(i, i == highlighted);
	}

	eink.CommitDisplayFast();
}

static void ScrollMenu(int oldi, int newi)
{
//	eink.InitDisplay();
//	
//	eink.CommitDisplayPartial_Only();
//	DrawMenuItem(oldi,0);
//	eink.CommitDisplayPartial_Only();
//	DrawMenuItem(newi,1);
//	eink.CommitDisplayPartial();
	
	eink.InitDisplay();
	DrawMenuItem(oldi,0);
	DrawMenuItem(newi,1);
	eink.CommitDisplayFast();	
}
static void EditInt_update_field(const IntEdit *edit, int field, int selected)
{
	const Field *e = edit->fields + field;
	char buf[10];
	snprintf(buf, sizeof(buf), "%d", values[field]);
	
	eink.DrawRectangle(e->x, e->y, e->x + e->w, e->y + e->h, BLACK, DOT_PIXEL_1X1, selected ? DRAW_FILL_FULL : DRAW_FILL_EMPTY);
	eink.DrawRectangle(e->x, e->y, e->x + e->w, e->y + e->h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
		
	eink.DrawString(e->x+4, e->y+4, buf, &Arial8b, selected ? WHITE : BLACK, selected ? BLACK : WHITE);
}

static void EditInt_clear_field(const IntEdit *edit, int field, UWORD colour)
{
	const Field *e = edit->fields + field;
	eink.DrawRectangle(e->x, e->y, e->x + e->w, e->y + e->h, colour, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	eink.DrawRectangle(e->x, e->y, e->x + e->w, e->y + e->h, colour, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}


static void EditInt_show(const IntEdit *edit)
{
	eink.DrawRectangle(20, 20, 230, 100, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	eink.DrawString(25, 25, edit->title, &Arial8, BLACK, WHITE);
	
	for (int i = 0; i < edit->count; i++)
	{
		EditInt_update_field(edit, i, i == edit_field);
	}
		
	if (menu_item == SetMmPerTip)
	{
		const Field *f = edit->fields;
		eink.DrawCircle((f[0].x + f[0].w + f[1].x)/2, f[0].y + f[0].h / 2, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	}
}

static const IntEdit *EditInt_get(int index)
{
	switch (index)
	{
		case SetDate:
			return &set_date;
		case SetTime:
			return &set_time;
		case SetMmPerTip:
			return &set_tip;
		default:
			return NULL;
	}
}

static void EditInt_StoreValue()
{
	switch (menu_item)
	{
		case SetDate:
			RTC_DateTypeDef date;
			date.Date = values[0];
			date.Month = values[1];
			date.Year = values[2];
			HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
			break;
		case SetTime:
			RTC_TimeTypeDef time;
			time.Hours = values[0];
			time.Minutes = values[1];
			HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
			break;
		case SetMmPerTip:
			mm_per_dump = values[0] * 1000 + values[1] * 100 + values[2] * 10 + values[3];
			break;
	}
}

static void EditInt_GetValue()
{
	switch (menu_item)
	{
		case SetDate:
			RTC_DateTypeDef date;
			HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
			values[0] = date.Date;
			values[1] = date.Month;
			values[2] = date.Year;
			break;
		case SetTime:
			RTC_TimeTypeDef time;
			HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
			values[0] = time.Hours;
			values[1] = time.Minutes;
			break;
		case SetMmPerTip:
			int n = mm_per_dump;
			values[3] = n % 10; n /= 10;
			values[2] = n % 10; n /= 10;
			values[1] = n % 10; n /= 10;
			values[0] = n % 10; n /= 10;
			break;
	}
}

static void EditInt_cycle_field()
{
	const IntEdit *e = EditInt_get(menu_item);
	
	edit_field++;
	eink.InitDisplay();
	eink.ClearDisplay();
	if (edit_field >= e->count)
	{
		EditInt_StoreValue();
		edit_field = 0;
		display_mode = Menu;
		DrawMenu(menu_item);
	}
	else
	{
		EditInt_show(e);
	}
	
	eink.CommitDisplayFast();
}

static void EditInt_increment()
{
	const IntEdit *e = EditInt_get(menu_item);

	values[edit_field]++;
	if (values[edit_field] >= e->fields[edit_field].max)
	{
		values[edit_field] = e->fields[edit_field].min;
	}
	eink.InitDisplay();
	EditInt_clear_field(e, edit_field, BLACK);
	eink.CommitDisplayPartial_Only();
	EditInt_clear_field(e, edit_field, WHITE);
	EditInt_update_field(e,edit_field,1);
	eink.CommitDisplayPartial();
}

static uint16_t ReadADC(uint8_t channel, uint8_t sample_time)
{
	ADC_ChannelConfTypeDef sConfig = { 0 };
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = sample_time;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	uint32_t sum = 0;
	for (int i = 0; i < 16; i++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 100);
		sum += HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Stop(&hadc1);
	
	return sum/16;
}

static void DisplayAboutScreen()
{
	uint16_t vref_int_1p2 = ReadADC(ADC_CHANNEL_VREFINT, ADC_SAMPLETIME_239CYCLES_5);						// vref 1.2v 
	uint16_t vsolar = 1200 * ReadADC(ADC_CHANNEL_8, ADC_SAMPLETIME_28CYCLES_5) / vref_int_1p2;				// vsolar mv
	uint16_t vbat = 1200 * ReadADC(ADC_CHANNEL_9, ADC_SAMPLETIME_28CYCLES_5) / vref_int_1p2;				// vbat mv
	uint16_t vtemp = 1200 * ReadADC(ADC_CHANNEL_TEMPSENSOR, ADC_SAMPLETIME_28CYCLES_5) / vref_int_1p2;		// vtemp mV
	uint16_t temp = 10 * vtemp / 43;

	vsolar = vsolar * 49 / 10;	// Resistor divider
	vsolar /= 10;				// V * 100
	vbat = vbat * 49 / 10;
	vbat /= 10;

	char buf[22];
	eink.DrawRectangle(20, 20, 230, 100, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	snprintf( buf, sizeof(buf), "Vsolar=%d.%02dv", vsolar / 100, vsolar % 100);
	eink.DrawString(40, 30, buf, &Arial8, BLACK, WHITE);
	snprintf( buf, sizeof(buf), "Vbat=%d.%02dv", vbat / 100, vbat % 100);
	eink.DrawString(40, 45, buf, &Arial8, BLACK, WHITE);
	snprintf( buf, sizeof(buf), "Internal Temp=%d.%01dC", vtemp / 100, vtemp % 100);
	eink.DrawString(40, 60, buf, &Arial8, BLACK, WHITE);
	//eink.CommitDisplay();
}
				
static void DisplayEdit()
{
	eink.InitDisplay();
	eink.ClearDisplay();
	switch (menu_item)
	{
		case ClearWeek:
			eink.DrawRectangle(20, 20, 230, 100, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
			eink.DrawString(40, 30, "Long press B1", &Arial8, BLACK, WHITE);
			eink.DrawString(40, 45, "to clear week data", &Arial8, BLACK, WHITE);
			break;
		case ClearAll:
			eink.DrawRectangle(20, 20, 230, 100, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
			eink.DrawString(40, 30, "Long press B1", &Arial8, BLACK, WHITE);
			eink.DrawString(40, 45, "to clear ALL data", &Arial8, BLACK, WHITE);
			break;
		case SetDate:
		case SetTime:
		case SetMmPerTip:
		{
			EditInt_GetValue();
			const IntEdit *edit = EditInt_get(menu_item);
			EditInt_show(edit);
			break;
		}
		case AboutScreen:
			DisplayAboutScreen();
			break;
	}
	eink.CommitDisplayFast();
}



// Returns the array index for time of day recordings
static uint8_t DayCount()
{
	return ((RTC_ReadTimeCounter(&hrtc)/60) % (24*60))/6;		// Every 6 minutes, 360 seconds, 10 times an hour, 240 times a day.
	
	// DEBUG
	//return ((RTC_ReadTimeCounter(&hrtc)) % (40*60))*240/2400;		// 1 tick every 10 seconds, 240 times in 40 minutes,
	//return ((RTC_ReadTimeCounter(&hrtc)) % (10*60))*240/600;		// 1 tick every 2.5 seconds, 240 times in 10 minutes,
}


static void ClearWeekData()
{
	memset(daily_rain, 0, sizeof(daily_rain));
	memset(yesterday_rain, 0, sizeof(yesterday_rain));
	bucket_dumps_offset = bucket_dumps;
	
	Debug("ClearWeekData\n");
}
static void ClearAllData()
{
	memset(daily_rain, 0, sizeof(daily_rain));
	memset(yesterday_rain, 0, sizeof(yesterday_rain));
	memset(weekly_rain, 0, sizeof(weekly_rain));
	memset(yearly_rain, 0, sizeof(yearly_rain));

	daily_rain_count = DayCount();
	weekly_rain_count = daily_rain_count / 24;
	yearly_rain_count = 0;
	bucket_dumps = 0;
	bucket_dumps_offset = 0;

	Debug("ClearAllData\n");
}

static void EditDisplay(int event)
{
	switch (menu_item)
	{
		case ClearWeek:
			if (event == BUTTON1_LONG_LONG_PRESS)
			{
				ClearWeekData();
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			else if (event == BUTTON1_PRESS)
			{
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			break;
		case ClearAll:
			if (event == BUTTON1_LONG_LONG_PRESS)
			{
				ClearAllData();
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			else if (event == BUTTON1_PRESS)
			{
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			break;
		case SetDate:
		case SetTime:
		case SetMmPerTip:
			if (event == BUTTON1_PRESS)
			{
				EditInt_cycle_field();
			}
			else if (event == BUTTON2_PRESS)
			{
				EditInt_increment();
			}
			break;
		case AboutScreen:
			if (event == BUTTON1_PRESS)
			{
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			break;
		default:
			break;
	}
}
//	- menu item (edit int)
//		- B1 Press, cycle through
//		- B2 Press, change
//		- B2 Long Press, exit menu

void ProcessKey(uint8_t event)
{
	switch (display_mode)
	{
		default:
		case Data:
			if (event == BUTTON1_PRESS)
			{
				screen_display = (Display)(screen_display + 1);
				if (screen_display >= MAX_DISPLAY)
				{
					screen_display = Number;	
				}
				UpdateDisplay();
			}
			else if ( event == BUTTON1_LONG_PRESS )
			{
				display_mode = Menu;
				DrawMenu(menu_item);
			}
			break;

		case Menu:
			if ( event == BUTTON1_PRESS )
			{
				int old = menu_item;
				menu_item++;
				if (menu_item >= countof(menu_list))
					menu_item = 0;
				ScrollMenu(old, menu_item);
			}
			else if ( event == BUTTON1_LONG_PRESS )
			{
				display_mode = Data;
				screen_display = Number;
				UpdateDisplay();
			}
			else if ( event == BUTTON2_PRESS )
			{
				display_mode = Edit;
				DisplayEdit();
			}
			break;
		
		case Edit:
			EditDisplay(event);
			break;
	}
}

extern "C" void rain_guage_app()
{
//	daily_rain[daily_rain_count] = 0;
//	daily_rain_count++;
//	for (int i = 1; i < 120; i++)
//	{
//		uint32_t r = rand();
//		uint32_t rm = RAND_MAX >> 3;
//		daily_rain[daily_rain_count] = daily_rain[daily_rain_count-1] + (r<rm ? 1 : 0);
//		daily_rain_count++;
//	}
	//DisplayAboutScreen();
	
	ClearAllData();

	// Set daily count based on time.
	daily_rain_count = DayCount();
	Debug("daily_rain_count=%d\n", daily_rain_count);
	weekly_rain_count = daily_rain_count / 24;
		
	// ToD samples
	for (;;)
	{
		int sleep_time = SLEEP_TIME;
		
		int daily = DayCount();
		Debug("daily=%d\n", daily);
		while (daily != daily_rain_count)
		{
			//	- Daily - Every 6 minutes - compute time relative to clock.  Every :00, :06, :12, ...
			//		- ?stored based on at end of struct, so now is on the right.
			//	- Yesterday - on day roll, copy data 
			//	- weekly = Every hour (every 10th 6 minutes)
			//	- yearly - one per day.
			Debug("new daily sample\n");
			daily_rain[daily_rain_count % DAILY_RAIN_SAMPLES] = bucket_dumps - bucket_dumps_offset;
			
// todo - weekly must be set based on time of day.  weekly_rain[ day_of_week + daily/24]  ? Set weekly_rain_count = DayCount() / 24.			
			if (daily_rain_count % 10 == 0)
			{
				Debug("new weekly sample----------\n");
				weekly_rain[weekly_rain_count % WEEKLY_RAIN_SAMPLES] = bucket_dumps - bucket_dumps_offset;
				weekly_rain_count++;
			}
			daily_rain_count++;
			if (daily_rain_count >= countof(daily_rain))
			{
				static int day = 0;
				Debug("new day--------------------- %d\n", day++);
				// yearly
				yearly_rain[yearly_rain_count % YEARLY_RAIN_SAMPLES] = bucket_dumps;
				yearly_rain_count++;
				if (yearly_rain_count >= YEARLY_RAIN_SAMPLES)
				{
					memmove(yearly_rain, yearly_rain + 1, sizeof(yearly_rain) - sizeof(yearly_rain[0]));
					yearly_rain_count = YEARLY_RAIN_SAMPLES - 1;
				}
				
				// last 7 days - shift 1 day
				if (weekly_rain_count >= countof(weekly_rain))
				{
					memmove(weekly_rain, weekly_rain + countof(weekly_rain) / 7, sizeof(weekly_rain) - sizeof(weekly_rain) / 7);
					memset(weekly_rain + countof(weekly_rain) - countof(weekly_rain) / 7, 0, sizeof(weekly_rain) / 7);
					weekly_rain_count = countof(weekly_rain) - countof(weekly_rain) / 7;
				}
				
				// Copy daily to yesterday
				memcpy(yesterday_rain, daily_rain, sizeof(daily_rain));
				
				// Clear daily
				memset(daily_rain, 0, sizeof(daily_rain));
				daily_rain_count = 0;
			}
		}
		
		// Update the display if there is something to display
		if ( display_mode == Data && RefreshDisplay() )
		{
			screen_display = Number;
			UpdateDisplay();
////////////////////////////////////////////////////////////////////////////////////////			
////////////////////////////////////////////////////////////////////////////////////////			
////////////////////////////////////////////////////////////////////////////////////////			
//menu_item = SetDate;
//display_mode = Edit;
//edit_field = 0;			
//DisplayEdit();
		}
		
		while (!ButtonQueue.IsEmpty())
		{
			uint8_t event = ButtonQueue.GetEvent();
			Debug("Button: %d\n", event);
			ProcessKey(event);
		}
		
#define USE_STOP_MOODE
#ifdef USE_STOP_MOODE		
		// Don't sleep if we are in a keypress down, display_mode isn't data, or there are events in the queue.
		//Debug("keydown=%d, display_mode=%d, queue.empty=%d\n", keydown, display_mode, ButtonQueue.IsEmpty());
		//if (!keydown && display_mode == Data && ButtonQueue.IsEmpty())
		if (!keydown && ButtonQueue.IsEmpty())
		{
			// Wake up after 
			RTC_AlarmTypeDef alarm = { 0 };
			alarm.Alarm = RTC_ALARM_A;
			HAL_RTC_GetTime(&hrtc, &alarm.AlarmTime, RTC_FORMAT_BIN);
			Debug("now: %02d:%02d:%02d\n", alarm.AlarmTime.Hours, alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);
			alarm.Alarm = RTC_ALARM_A;
			add_seconds(&alarm.AlarmTime, sleep_time);
		
		
			Debug("set: %02d:%02d:%02d\n", alarm.AlarmTime.Hours, alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);

			HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
		
			// enter low power stop mode
			Debug("Low power stop\n"); fflush(stdout); HAL_Delay(250);
		
			#ifdef FAST_SEMIHOSTING_STDIO_DRIVER
				SuspendFastSemihostingPolling();
			#endif

			HAL_PWR_EnableSEVOnPend();
			HAL_SuspendTick();

		
			HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
		
			HAL_ResumeTick();


			#ifdef FAST_SEMIHOSTING_STDIO_DRIVER
				ResumeFastSemihostingPolling();
			#endif
			Debug("Wake from Low power stop\n");
		}
#endif 
	}
}




/*
TODO
- Store data.
	- store counts in buckets. (display is 250 pixels wide)
		- daily	24*10 - 240 - each 6 minutes
		- yesterday
		- weekly 7*24 - 168 - each 60 minutes
		- yearly n - 1 per day.	This won't display without anti-aliasing, or multiple pixels per day.
	- Daily - Every 6 minutes - compute time relative to clock.  Every :00, :06, :12, ...
		- stored based on at end of struct, so now is on the right.
	- Yesterday - on day roll, copy data 
	- weekly = Every hour (every 10th 6 minutes)
	- yearly - one per day.
- Temperature + humidity?
	- dht11
	- storage?
- Buttons
	- Long press, clear daily + weekly
	- Long Long press, when daily and weekly are clear, clear yearly.
	- Short press, toggle between count, daily, weekly, yearly.
	- Any way to reset the time of day?  Date?
	- Menu?
	- 2 buttons?
		- 1 toggle between items
		- 2 menu
			- clear week
			- clear all
			- set date/time 
	- Data mode
		- B1 Press, cycle display
		- B2 Long Press, menu
	- Menu
		- B1 Press, cycle menu
		- B2 Press, select menu item
	- menu item (clear)
		- B1 Long Long press, clear
	- menu item (edit int)
		- B1 Press, cycle through
		- B2 Press, change
		- B2 Long Press, exit menu
- Display dot in the corner if we wrap 999.9 mm



* Do we care about keydown event?
	* Not really.  We just need to know if this is a press, long press, long long press.
	* So we wake up, remember the key down, then event off the key release, either click, long, real long
	
*/