/* at: RTC_ITConfig 0x0802EF2C, RTC_EnterConfigMode 0x0802EF46, RTC_ExitConfigMode 0x0802EF58, RTC_GetCounter 0x0802EF6C, RTC_SetCounter 0x0802EF7E, RTC_SetPrescaler 0x0802EF98, RTC_SetAlarm 0x0802EFB4, RTC_GetDivider 0x0802EFCE, RTC_WaitForLastTask 0x0802EFE4, RTC_WaitForSynchro 0x0802EFF6, RTC_GetFlagStatus 0x0802F01A, RTC_ClearFlag 0x0802F030, RTC_GetITStatus 0x0802F040, RTC_ClearITPendingBit 0x0802F060 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_rtc.c: the real time clock driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: rtc.c keeps the calendar in the RTC counter; schedule_p3.c and modbus_slave.c
   read and set the time through it.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c"
