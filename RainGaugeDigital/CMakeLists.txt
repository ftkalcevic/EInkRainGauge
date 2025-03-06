cmake_minimum_required(VERSION 3.15)

project(RainGaugeDigital LANGUAGES C CXX ASM)

find_bsp(
	ID com.sysprogs.project_importers.stm32.cubemx
	SOURCE_PROJECT RainGaugeDigital.ioc
	CONFIGURATION com.sysprogs.toolchainoptions.arm.syscallspecs=
	HWREGISTER_LIST_FILE ../../../../VisualGDB/EmbeddedBSPs/arm-eabi/com.sysprogs.arm.stm32/2020.01/STM32F1xxxx/DeviceDefinitions/stm32f103x6.xml.gz)

add_bsp_based_executable(
	NAME RainGaugeDigital
	GENERATE_BIN
	GENERATE_MAP
	SOURCES
		${BSP_ROOT}/eink.h
		${BSP_ROOT}/RainGaugeApp.h
		${BSP_ROOT}/RainGaugeApp.cpp
		${BSP_ROOT}/DEV_Config.c
		${BSP_ROOT}/Debug.h
		${BSP_ROOT}/DEV_Config.h
		${BSP_ROOT}/eink.cpp
		${BSP_ROOT}/GUI_Paint.c
		${BSP_ROOT}/GUI_Paint.h
		${BSP_ROOT}/ImageData.h
		${BSP_ROOT}/Fonts/fonts.h
		${BSP_ROOT}/EPD_2in13_V4.c
		${BSP_ROOT}/EPD_2in13_V4.h
		${BSP_ROOT}/EPD_2in13_V4_test.c
		${BSP_ROOT}/Fonts/arial_narrow_280.c
		${BSP_ROOT}/queue.h
		${BSP_ROOT}/Fonts/Arial8.c
		${BSP_ROOT}/Fonts/Arial8b.c)
set_source_files_properties(${BSP_ROOT}/Core/Startup/startup_stm32f103c8tx.S ${BSP_ROOT}/EPD_2in13_V4_test.c PROPERTIES HEADER_FILE_ONLY TRUE)

#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")
