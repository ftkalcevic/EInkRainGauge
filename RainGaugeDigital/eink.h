#pragma once

#include <stdint.h>

#include "EPD_2in13_V4.h"
#include "GUI_Paint.h"
#include "Debug.h"


class EInkDisplay
{
	uint8_t Image[((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT];
	uint16_t Imagesize; 

public:

	// Display rotated 90
	const int width = EPD_2in13_V4_HEIGHT;
	const int height = EPD_2in13_V4_WIDTH;

	void InitDisplay()
	{
		if (DEV_Module_Init() != 0) 
		{
			Debug("Failed to init module...\r\n");
			return;
		}

		//EPD_2in13_V4_Init();
		EPD_2in13_V4_Init_Fast();
	}
	
	void ClearDisplay()
	{
		//Create a new image cache
		Paint_Clear(WHITE);
	}
	
	void CommitDisplay()
	{
		EPD_2in13_V4_Display_Base(Image);
		EPD_2in13_V4_Sleep();

		Debug("close 5V, Module enters 0 power consumption ...\r\n");
		DEV_Module_Exit();
	}
	
	void CommitDisplayFast()
	{
		EPD_2in13_V4_Display_Fast(Image);
		EPD_2in13_V4_Sleep();

		Debug("close 5V, Module enters 0 power consumption ...\r\n");
		DEV_Module_Exit();
	}
	
	void CommitDisplayPartial()
	{
		EPD_2in13_V4_Display_Partial(Image);
		EPD_2in13_V4_Sleep();

		Debug("close 5V, Module enters 0 power consumption ...\r\n");
		DEV_Module_Exit();
	}
	
	void CommitDisplayPartial_Only()
	{
		EPD_2in13_V4_Display_Partial(Image);
	}
	
	EInkDisplay()
	{
		Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0) ? (EPD_2in13_V4_WIDTH / 8) : (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
		Paint_NewImage(Image, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
	}

	void DrawNumberX10(UWORD x, UWORD y, int number, const FontDefinition* Font, UWORD Color_Foreground, UWORD Color_Background)
	{
		int offset = 0; 
		if (number < 100)
			offset = 2;
		else if (number < 1000)
			offset = 1;
		else if (number < 10000)
			offset = 0;
	
		uint16_t width_pixels;
		if (Font->proportional)
			width_pixels = Font->p.chars_table['0' - Font->char_start_index].width;
		else
			width_pixels = Font->f.width_pixels;
		DOT_PIXEL pixel_size = (DOT_PIXEL)(Font->height_pixels / 8 < 1 ? 1 : Font->height_pixels / 8 > 6 ? 6 : Font->height_pixels / 8);
		
		Paint_DrawNum(x + offset*(width_pixels + Font->char_space), y, number / 10, Font, BLACK, WHITE);
		Paint_DrawPoint(x + (width_pixels + Font->char_space) * 3 + (int)pixel_size, y + Font->height_pixels - 6, BLACK, pixel_size, DOT_STYLE_DFT);
		Paint_DrawNum(x + (width_pixels + Font->char_space) * 3 + 2 * (int)pixel_size, y, number % 10, Font, BLACK, WHITE);
	}

	
	void DrawGraph(const char *title, uint32_t *data, uint16_t count, uint16_t max_count, uint8_t segments, int max_valuex10 )
	{
		const int CHART_TICK_LENGTH = 7;
		InitDisplay();
		ClearDisplay();

		// Axis
		Paint_DrawLine(0, 4, 0, height - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
		Paint_DrawLine(0, height - 1, width - 1, height - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
		
		// Horizontal ticks
		for (int i = 1; i < segments; i++)
		{
			int x = i * width / segments;
			Paint_DrawLine(x, height - CHART_TICK_LENGTH, x, height - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
		}
		
		// Data
		uint32_t min = data[0];
		uint32_t max = data[count - 1];
		uint32_t diff = max - min;
		uint8_t last_x, last_y;
		for ( int i = 0; i < count; i++ )
		{
			uint8_t x = i * width / max_count;
			uint8_t y = height - 1 - ((data[i] - min) * (height-1-4) / diff);
			if (count == 1)
			{
				Paint_DrawPoint(x, y, BLACK, DOT_PIXEL_1X1, DOT_FILL_AROUND);
			}
			else if (i > 0)
			{
				Paint_DrawLine(last_x, last_y, x, y, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
			}
			last_x = x;
			last_y = y;
		}
		
		int len1 = StringLength(&Arial8, title) + Arial8.char_space;
		int len2 = count > 0 ? StringLength(&Arial8, "000.0") + Arial8.char_space : 0;
		
		UWORD x1 = width - (len1+len2);
		UWORD y1 = height - (Arial8.height_pixels + CHART_TICK_LENGTH);
		if (count > max_count / 2)	// if only half the data is available, show labels on the right, else left
		{
			x1 = 3;
			y1 = 0;
		}
		
		DrawRectangle( x1, y1, x1 + len1, y1 + Arial8.height_pixels, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		Paint_DrawString_EN(x1, y1, title, &Arial8, BLACK, WHITE);

		if (count > 0)
		{
			DrawNumberX10( x1+len1, y1, max_valuex10, &Arial8b, BLACK, WHITE);
		}
		
		CommitDisplayFast();
	}

	void DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
	{
		Paint_DrawRectangle(Xstart, Ystart, Xend, Yend, Color, Line_width, Draw_Fill);
	}
	
	int DrawString(UWORD Xstart, UWORD Ystart, const char * pString, const FontDefinition* Font, UWORD Color_Foreground, UWORD Color_Background)
	{
		return Paint_DrawString_EN(Xstart, Ystart, pString, Font, Color_Foreground, Color_Background);
	}

	int StringLength(const FontDefinition *Font, const char *s)
	{
		return Paint_GetStringWidth(Font, s);
	}
	
	void DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius, UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
	{
		Paint_DrawCircle(X_Center, Y_Center, Radius, Color, Line_width, Draw_Fill);
	}
};