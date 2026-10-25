/* at: PWR_DeInit 0x0802F1E0, PWR_BackupAccessCmd 0x0802F1F6, PWR_PVDCmd 0x0802F1FC, PWR_PVDLevelConfig 0x0802F202, PWR_WakeUpPinCmd 0x0802F214, PWR_EnterSTOPMode 0x0802F21C, PWR_EnterSTANDBYMode 0x0802F252, PWR_GetFlagStatus 0x0802F27C, PWR_ClearFlag 0x0802F290 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_pwr.c: the power control driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: rtc.c enables access to the backup domain (PWR_BackupAccessCmd).
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c"
