/* at: USART_DeInit 0x08030710, USART_Init 0x08030796, USART_StructInit 0x08030868, USART_ClockInit 0x08030880, USART_ClockStructInit 0x080308A2, USART_Cmd 0x080308AE, USART_ITConfig 0x080308C6, USART_DMACmd 0x08030910, USART_SetAddress 0x08030922, USART_WakeUpConfig 0x08030934, USART_ReceiverWakeUpCmd 0x08030946, USART_LINBreakDetectLengthConfig 0x0803095E, USART_LINCmd 0x08030970, USART_SendData 0x08030988, USART_ReceiveData 0x08030990, USART_SendBreak 0x0803099A, USART_SetGuardTime 0x080309A4, USART_SetPrescaler 0x080309B4, USART_SmartCardCmd 0x080309C4, USART_SmartCardNACKCmd 0x080309DC, USART_HalfDuplexCmd 0x080309F4, USART_OverSampling8Cmd 0x08030A0C, USART_OneBitMethodCmd 0x08030A22, USART_IrDAConfig 0x08030A3A, USART_IrDACmd 0x08030A4C, USART_GetFlagStatus 0x08030A64, USART_ClearFlag 0x08030A7E, USART_GetITStatus 0x08030A90, USART_ClearITPendingBit 0x08030AE4 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_usart.c: the USART driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: the four serial ports: USART1 (auxiliary), USART2 (RS485 to the meter), USART3
   (RS485 Modbus slave) and UART4 (the link to the DSP), in board.c, uart.c, monitor.c and
   the interrupt handlers in stm32f10x_it.c.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c"
