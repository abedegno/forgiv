/* at: GPIO_DeInit 0x0802E604, GPIO_AFIODeInit 0x0802E6B0, GPIO_Init 0x0802E6C4, GPIO_StructInit 0x0802E7DA, GPIO_ReadInputDataBit 0x0802E7EA, GPIO_ReadInputData 0x0802E7FC, GPIO_ReadOutputDataBit 0x0802E804, GPIO_ReadOutputData 0x0802E816, GPIO_SetBits 0x0802E81E, GPIO_ResetBits 0x0802E822, GPIO_WriteBit 0x0802E826, GPIO_Write 0x0802E830, GPIO_PinLockConfig 0x0802E834, GPIO_EventOutputConfig 0x0802E846, GPIO_EventOutputCmd 0x0802E860, GPIO_PinRemapConfig 0x0802E866, GPIO_EXTILineConfig 0x0802E8F0, GPIO_ETH_MediaInterfaceConfig 0x0802E932 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_gpio.c: the GPIO driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: every pin set-up and most pin writes: board.c, panel_led.c, i2c.c, eeprom.c and
   stm32f10x_it.c.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c"
