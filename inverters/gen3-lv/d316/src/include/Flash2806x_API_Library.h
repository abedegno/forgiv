/* Flash2806x_API_Library.h: TI's flash API in the boot ROM, as Flash2806x_API_Library.h
   declares it (Flash_Erase, Flash_Program, the FLASH_ST status, Flash_CPUScaleFactor and
   Flash_CallbackPtr), and SCALE_FACTOR from Flash2806x_API_Config.h for 90 MHz.
   Users: boot/boot_flash_loader.c only.
   name: original, TI's file names; only the calls the update routine makes are declared. */
#ifndef FLASH2806X_API_LIBRARY_H
#define FLASH2806X_API_LIBRARY_H

#include "F2806x_Device.h"

typedef struct {
    Uint32 FirstFailAddr;
    Uint16 ExpectedData;
    Uint16 ActualData;
} FLASH_ST;

extern Uint16 Flash_Erase(Uint16 SectorMask, FLASH_ST *FEraseStat);
extern Uint16 Flash_Program(Uint16 *FlashAddr, Uint16 *BuffAddr, Uint32 Length,
                            FLASH_ST *FProgStatus);
extern Uint32 Flash_CPUScaleFactor;
extern void (*Flash_CallbackPtr)(void);

#define SCALE_FACTOR 0x012000BCUL /* 1048576.0 * (200 / CPU_RATE), CPU_RATE 11.111 ns */

#endif
