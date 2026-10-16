/* at: USART_DeInit 0x0802FE90, USART_Init 0x0802FF16, USART_StructInit 0x0802FFE8, USART_ClockInit 0x08030000, USART_ClockStructInit 0x08030022, USART_Cmd 0x0803002E, USART_ITConfig 0x08030046, USART_DMACmd 0x08030090, USART_SetAddress 0x080300A2, USART_WakeUpConfig 0x080300B4, USART_ReceiverWakeUpCmd 0x080300C6, USART_LINBreakDetectLengthConfig 0x080300DE, USART_LINCmd 0x080300F0, USART_SendData 0x08030108, USART_ReceiveData 0x08030110, USART_SendBreak 0x0803011A, USART_SetGuardTime 0x08030124, USART_SetPrescaler 0x08030134, USART_SmartCardCmd 0x08030144, USART_SmartCardNACKCmd 0x0803015C, USART_HalfDuplexCmd 0x08030174, USART_OverSampling8Cmd 0x0803018C, USART_OneBitMethodCmd 0x080301A2, USART_IrDAConfig 0x080301BA, USART_IrDACmd 0x080301CC, USART_GetFlagStatus 0x080301E4, USART_ClearFlag 0x080301FE, USART_GetITStatus 0x08030210, USART_ClearITPendingBit 0x08030264 */
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
