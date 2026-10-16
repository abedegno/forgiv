/* at: RCC_DeInit 0x0802EA2C, RCC_HSEConfig 0x0802EA7A, RCC_GetFlagStatus 0x0802EAC0, RCC_WaitForHSEStartUp 0x0802EAF8, RCC_AdjustHSICalibrationValue 0x0802EB30, RCC_HSICmd 0x0802EB44, RCC_PLLConfig 0x0802EB4A, RCC_PLLCmd 0x0802EB62, RCC_PREDIV1Config 0x0802EB68, RCC_PREDIV2Config 0x0802EB80, RCC_PLL2Config 0x0802EB92, RCC_PLL2Cmd 0x0802EBA4, RCC_PLL3Config 0x0802EBAA, RCC_PLL3Cmd 0x0802EBBC, RCC_SYSCLKConfig 0x0802EBC2, RCC_GetSYSCLKSource 0x0802EBD4, RCC_HCLKConfig 0x0802EBDE, RCC_PCLK1Config 0x0802EBF0, RCC_PCLK2Config 0x0802EC02, RCC_ITConfig 0x0802EC16, RCC_OTGFSCLKConfig 0x0802EC30, RCC_ADCCLKConfig 0x0802EC38, RCC_I2S2CLKConfig 0x0802EC4A, RCC_I2S3CLKConfig 0x0802EC50, RCC_LSEConfig 0x0802EC58, RCC_LSICmd 0x0802EC8A, RCC_RTCCLKConfig 0x0802EC90, RCC_RTCCLKCmd 0x0802EC9C, RCC_GetClocksFreq 0x0802ECA4, RCC_AHBPeriphClockCmd 0x0802EE48, RCC_APB2PeriphClockCmd 0x0802EE62, RCC_APB1PeriphClockCmd 0x0802EE7C, RCC_AHBPeriphResetCmd 0x0802EE96, RCC_APB2PeriphResetCmd 0x0802EEB0, RCC_APB1PeriphResetCmd 0x0802EECA, RCC_BackupResetCmd 0x0802EEE4, RCC_ClockSecuritySystemCmd 0x0802EEEA, RCC_MCOConfig 0x0802EEF0, RCC_ClearFlag 0x0802EEF6, RCC_GetITStatus 0x0802EF04, RCC_ClearITPendingBit 0x0802EF18, APBAHBPrescTable 0x20000491, ADCPrescTable 0x200004A1 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_rcc.c: the reset and clock control driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: peripheral clock enables (board.c, panel_led.c, usb_bsp.c), the LSE and RTC clock
   (rtc.c), the USB OTG FS clock divider (usb_bsp.c). USART_Init gets the bus clocks from
   RCC_GetClocksFreq here.
   Changes: none in the file. HSE_VALUE is 8000000 (an 8 MHz crystal) where V3.5.0's
   stm32f10x.h sets 25000000 for STM32F10X_CL: RCC_GetClocksFreq loads 8000000 for the HSE
   and PREDIV1 cases and its pool has no 25000000. A project define (the -DHSE_VALUE in the
   opts line) and an edit of the HSE_VALUE line in stm32f10x.h give the same bytes, so I
   cannot tell which the vendor did (docs/matching.md, round 4).
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c"
