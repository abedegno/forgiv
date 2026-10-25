/* boot_csm.c: the boot loader's CSM unlock and the step that moves the update routine to RAM.

   The code security module (CSM) locks the flash, OTP and the L0..L3 RAM against code that runs
   outside them while the 128-bit password at 0x3F7FF8 (F2806x_CSMPasswords.asm) is not all
   0xFFFF. This image's password is 0xFFFF x 4, 0xF5F4, 0xF0F3, 0xF5F2, 0xF2F1, so the device is
   secure. The update routine runs from M1 RAM (0x0400), which is outside the secure area, so it
   could not erase or program the flash. CsmUnlock writes the same eight words to the KEY
   registers and then reads the eight password locations, as the CSM requires; when they match,
   CSMSCR.SECURE reads 0 and the device stays open until the next reset. CsmUnlock is TI's code;
   only the keys are this build's. boot_copy_run ignores its result.

   Entry points: CsmUnlock; boot_copy_run (from boot_update).
   Data: none.
   Neighbours: boot_update.c, boot_flash_loader.c, ti/F2806x_MemCopy.c (MemCopy),
   ti/F2806x_CSMPasswords.asm (the password), boot_main.asm.
   name: descriptive (map/filenames.tsv); CsmUnlock is TI's name from its examples.
   at: CsmUnlock 0x3F528C, boot_copy_run 0x3F52C4 (boot_update 0x3F5000 is in boot_update.c,
   MemCopy 0x3F52BB in F2806x_MemCopy.c; boot_main 0x3F5226 is assembly, boot_main.asm) */
#include "F2806x_Device.h"
#include "F2806x_SysCtrl.h"
#include "boot.h"
#include "F2806x_GlobalPrototypes.h"
#include "util.h"

#define STATUS_FAIL     0
#define STATUS_SUCCESS  1

/* TI's CsmUnlock: writes the keys, reads the password locations and returns STATUS_SUCCESS (1)
   when the CSM is open, else STATUS_FAIL (0). */
Uint16 CsmUnlock(void)
{
    volatile Uint16 temp;

    EALLOW;
    CsmRegs.KEY0 = 0xFFFF;
    CsmRegs.KEY1 = 0xFFFF;
    CsmRegs.KEY2 = 0xFFFF;
    CsmRegs.KEY3 = 0xFFFF;
    CsmRegs.KEY4 = 0xF5F4;
    CsmRegs.KEY5 = 0xF0F3;
    CsmRegs.KEY6 = 0xF5F2;
    CsmRegs.KEY7 = 0xF2F1;
    EDIS;

    temp = CsmPwl.PSWD0;
    temp = CsmPwl.PSWD1;
    temp = CsmPwl.PSWD2;
    temp = CsmPwl.PSWD3;
    temp = CsmPwl.PSWD4;
    temp = CsmPwl.PSWD5;
    temp = CsmPwl.PSWD6;
    temp = CsmPwl.PSWD7;

    if (CsmRegs.CSMSCR.bit.SECURE == 0) return STATUS_SUCCESS;
    else return STATUS_FAIL;
}

/* Unlocks the CSM, copies the update routine from flash (0x3F5004 .. 0x3F5225) to RAM at
   0x0400 and runs it there. It does not return. */
void boot_copy_run(void)
{
    CsmUnlock();
    MemCopy(BootRamfuncsLoadStart, BootRamfuncsLoadEnd, BootRamfuncsRunStart);
    boot_flash_loader();
}
