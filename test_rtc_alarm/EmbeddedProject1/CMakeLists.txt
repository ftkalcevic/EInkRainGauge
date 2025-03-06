cmake_minimum_required(VERSION 3.15)

project(EmbeddedProject1 LANGUAGES C CXX ASM)

find_bsp(
	ID com.sysprogs.project_importers.stm32.cubemx
	SOURCE_PROJECT EmbeddedProject1.ioc
	FRAMEWORKS com.sysprogs.embedded.semihosting_and_profiler
	FWCONFIGURATION
		com.sysprogs.efp.semihosting.blocking_mode=0
		com.sysprogs.efp.profiling.debugger_check=SYSPROGS_PROFILER_DEBUGGER_CHECK_MODE=1
		com.sysprogs.efp.profiling.rtos=
		com.sysprogs.efp.profiling.hold_interrupts=FAST_SEMIHOSTING_HOLD_INTERRUPTS=0
	HWREGISTER_LIST_FILE ../../../../../VisualGDB/EmbeddedBSPs/arm-eabi/com.sysprogs.arm.stm32/2020.01/STM32F1xxxx/DeviceDefinitions/stm32f103x6.xml.gz)

add_bsp_based_executable(
	NAME EmbeddedProject1
	GENERATE_BIN
	GENERATE_MAP)

