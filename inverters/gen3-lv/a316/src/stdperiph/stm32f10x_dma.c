/* at: DMA_DeInit 0x0802DCD8, DMA_Init 0x0802DE1C, DMA_StructInit 0x0802DE58, DMA_Cmd 0x0802DE72, DMA_ITConfig 0x0802DE8A, DMA_SetCurrDataCounter 0x0802DE9C, DMA_GetCurrDataCounter 0x0802DEA0, DMA_GetFlagStatus 0x0802DEA8, DMA_ClearFlag 0x0802DED0, DMA_GetITStatus 0x0802DEE6, DMA_ClearITPendingBit 0x0802DF0E */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_dma.c: the DMA driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: board.c uses one DMA channel for the ADC samples.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c"
