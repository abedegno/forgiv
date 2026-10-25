/* at: EXTI_DeInit 0x0802E7AC, EXTI_Init 0x0802E7C8, EXTI_StructInit 0x0802E856, EXTI_GenerateSWInterrupt 0x0802E866, EXTI_GetFlagStatus 0x0802E876, EXTI_ClearFlag 0x0802E88C, EXTI_GetITStatus 0x0802E894, EXTI_ClearITPendingBit 0x0802E8B6 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_exti.c: the external interrupt driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: EXTI1_IRQHandler (stm32f10x_it.c) checks and clears the line that reports USB
   over-current, as in ST's USB host examples.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c"
