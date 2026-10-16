/* at: FLASH_SetLatency 0x0802E048, FLASH_HalfCycleAccessCmd 0x0802E05A, FLASH_PrefetchBufferCmd 0x0802E070, FLASH_Unlock 0x0802E086, FLASH_UnlockBank1 0x0802E092, FLASH_Lock 0x0802E09E, FLASH_LockBank1 0x0802E0AC, FLASH_GetBank1Status 0x0802E0BA, FLASH_WaitForLastOperation 0x0802E0EA, FLASH_ErasePage 0x0802E110, FLASH_EraseAllPages 0x0802E158, FLASH_WaitForLastBank1Operation 0x0802E19C, FLASH_EraseAllBank1Pages 0x0802E1C2, FLASH_GetReadOutProtectionStatus 0x0802E206, FLASH_EraseOptionBytes 0x0802E21A, FLASH_ProgramWord 0x0802E2B0, FLASH_ProgramHalfWord 0x0802E316, FLASH_ProgramOptionByteData 0x0802E352, FLASH_EnableWriteProtection 0x0802E39A, FLASH_ReadOutProtection 0x0802E462, FLASH_UserOptionByteConfig 0x0802E4FE, FLASH_GetUserOptionByte 0x0802E556, FLASH_GetWriteProtectionOptionByte 0x0802E55E, FLASH_GetPrefetchBufferStatus 0x0802E564, FLASH_ITConfig 0x0802E578, FLASH_GetFlagStatus 0x0802E592, FLASH_ClearFlag 0x0802E5BC, FLASH_GetStatus 0x0802E5C2 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_flash.c: the flash memory driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: usbh_usr.c unlocks and programs the internal flash when it takes ARMStore.bin
   from a USB stick (FLASH_Unlock, FLASH_ClearFlag, FLASH_ProgramWord).
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c"
