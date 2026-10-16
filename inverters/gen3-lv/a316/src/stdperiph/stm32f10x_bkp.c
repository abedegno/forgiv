/* at: BKP_DeInit 0x0802DC14, BKP_TamperPinLevelConfig 0x0802DC24, BKP_TamperPinCmd 0x0802DC2A, BKP_ITConfig 0x0802DC32, BKP_RTCOutputConfig 0x0802DC3A, BKP_SetRTCCalibrationValue 0x0802DC50, BKP_WriteBackupRegister 0x0802DC66, BKP_ReadBackupRegister 0x0802DC7E, BKP_GetFlagStatus 0x0802DC98, BKP_ClearFlag 0x0802DCA2, BKP_GetITStatus 0x0802DCB4, BKP_ClearITPendingBit 0x0802DCBE */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_bkp.c: the backup register driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: rtc.c keeps 0xA5A5 in backup register 4 once the calendar has been set;
   modbus_slave.c writes the same mark when the time is set over Modbus.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c"
