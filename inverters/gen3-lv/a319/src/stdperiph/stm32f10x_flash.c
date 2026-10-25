/* at: FLASH_SetLatency 0x0802E8C8, FLASH_HalfCycleAccessCmd 0x0802E8DA, FLASH_PrefetchBufferCmd 0x0802E8F0, FLASH_Unlock 0x0802E906, FLASH_UnlockBank1 0x0802E912, FLASH_Lock 0x0802E91E, FLASH_LockBank1 0x0802E92C, FLASH_GetBank1Status 0x0802E93A, FLASH_WaitForLastOperation 0x0802E96A, FLASH_ErasePage 0x0802E990, FLASH_EraseAllPages 0x0802E9D8, FLASH_WaitForLastBank1Operation 0x0802EA1C, FLASH_EraseAllBank1Pages 0x0802EA42, FLASH_GetReadOutProtectionStatus 0x0802EA86, FLASH_EraseOptionBytes 0x0802EA9A, FLASH_ProgramWord 0x0802EB30, FLASH_ProgramHalfWord 0x0802EB96, FLASH_ProgramOptionByteData 0x0802EBD2, FLASH_EnableWriteProtection 0x0802EC1A, FLASH_ReadOutProtection 0x0802ECE2, FLASH_UserOptionByteConfig 0x0802ED7E, FLASH_GetUserOptionByte 0x0802EDD6, FLASH_GetWriteProtectionOptionByte 0x0802EDDE, FLASH_GetPrefetchBufferStatus 0x0802EDE4, FLASH_ITConfig 0x0802EDF8, FLASH_GetFlagStatus 0x0802EE12, FLASH_ClearFlag 0x0802EE3C, FLASH_GetStatus 0x0802EE42 */
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
