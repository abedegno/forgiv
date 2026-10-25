/* F2806x_SysCtrl.c: TI's InitFlash (F2806x_SysCtrl.c), run from RAM: init_system copies it
   with the other ramfuncs from 0x3F1BB7 to 0x8000 and then calls it. It sets the flash wait
   states for 90 MHz (page and random 3, standby and active 0x1FF) and turns the flash pipeline
   on.
   The one change from TI's text: OTPWAIT is 3 where TI's example has 5. (The boot loader's
   inline copy in boot_flash_loader.c keeps 5.)
   TI's file also holds InitSysCtrl, XtalOscSel, InitPll and InitPeripheralClocks, which are in
   drivers/device_init.c here: that object cannot be split, so this file has only InitFlash.
   Entry point: InitFlash. Data: none. Neighbours: drivers/device_init.c.
   name: likely (map/filenames.tsv): TI's file name for InitFlash.
   at: InitFlash 0x3F334E, runs at 0x9797 */
#include "F2806x_SysCtrl.h"
#include "F2806x_GlobalPrototypes.h"

#pragma CODE_SECTION(InitFlash, "ramfuncs");
/* TI's InitFlash with OTPWAIT 3; must run from RAM, as it changes the flash timing. */
void InitFlash(void)
{
    EALLOW;
    FlashRegs.FOPT.bit.ENPIPE = 1;
    FlashRegs.FBANKWAIT.bit.PAGEWAIT = 3;
    FlashRegs.FBANKWAIT.bit.RANDWAIT = 3;
    FlashRegs.FOTPWAIT.bit.OTPWAIT = 3;
    FlashRegs.FSTDBYWAIT.bit.STDBYWAIT = 0x01FF;
    FlashRegs.FACTIVEWAIT.bit.ACTIVEWAIT = 0x01FF;
    EDIS;
    asm(" RPT #7 || NOP");
}
