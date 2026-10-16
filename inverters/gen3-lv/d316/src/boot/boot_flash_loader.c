/* boot_flash_loader.c: the boot loader's update routine, the DSP end of a firmware update. The
   ARM runs it when it updates the DSP from a USB stick's DSPStore.bin (A316
   docs/subsystems/usb.md): the ARM sends run command 4 six times, the state machine goes to
   OP_UPDATE, and after 100 ticks state4_update calls boot_update, which unlocks the CSM, copies
   this routine to RAM at 0x0400 and jumps to it (boot_csm.c). boot_main also comes here
   directly, with no application, when the word at 0x3F3FFC is not 0xAA. The routine never
   returns.

   It sets the clocks, the PIE, the flash wait states, SCI-A and GPIO27 itself (TI's XtalOscSel,
   InitPll(9, 2), part of InitPeripheralClocks, InitPieCtrl, InitFlash with OTPWAIT 5 and
   InitSciaGpio written inline), because it must not call into the flash it is about to erase.
   SCI-A runs at 9600 8N1 (BRR 0x0124 at LSPCLK 22.5 MHz), polled, with no interrupts.

   The protocol (the ARM is the master; the A316 calls these the "W", "S", "D", "E" frames):
     frame  0x0A, len, cmd, len data bytes, sum high, sum low      (len + 5 bytes)
            sum = the 16-bit sum of the bytes 0x0A, len, cmd and the data
     reply  0x0A, 0, code, sum high, sum low (the sum of 0x0A, 0 and code)
            code 'O' (0x4F) done, 'C' (0x43) bad sum, else the flash API's status (nonzero)
     'W'    Flash_Erase(0xFE): sectors B..H, everything but sector A (0x3F4000..0x3F7FFF), which
            holds this boot loader, codestart and the CSM passwords. Then 'O' or the status.
     'S'    data 4 bytes: the flash word address, high byte first. 'O'.
     'D'    data 2 + 128 bytes: a block number n (high byte first), then 64 words, each high
            byte first. Flash_Program writes them at address + 64 x n when that is below
            0x3F4000; at or above it nothing is written and the reply is still 'O'. The status
            is the reply when it is not 0.
     'F'    'O' and nothing else (the hello).
     'E'    'O', then a pause, GPIO27 high and a loop until the chip is reset from outside (the
            watchdog is off on every path, so it is not the watchdog that resets it).
     other  reply code 0.
   A frame with a bad sum gets 'C'; 20,000,000 polls with no byte clear the frame state.

   The application becomes bootable when the 'D' blocks have written its last block, which holds
   app_valid = 0xAA at 0x3F3FFC (app_header.c): boot_main starts the application only when that
   word is 0xAA, and 'W' erases it first. I found no signature and no check of the whole image:
   the only checks are each frame's 16-bit sum and the 0xAA word. The boot_rx_active tests at n 1
   and n 2 can never fail (it is set to 1 as each byte arrives), so the rule that a session must
   start with an 'F' frame of length 0 does nothing. The ARM never sends 'F' (it sends 'W',
   'S', the 'D' blocks and 'E'), so it relies on that (docs/findings-comm.md).

   Entry point: boot_flash_loader (from boot_copy_run, at its RAM address 0x0400).
   Data: boot_rx_active, FlashStatus, boot_reply[5], boot_prog_buf[70], boot_rx_frame[140]
   (0xD640 .. 0xD78B); their .cinit records sit in the application's .cinit, because the boot
   loader is linked into the same image.
   Neighbours: boot_csm.c (boot_copy_run, CsmUnlock), boot_main.asm, the boot ROM's flash API
   (Flash2806x_API_Library.h: Flash_Erase, Flash_Program, Flash_CPUScaleFactor, Flash_CallbackPtr).
   name: descriptive (map/filenames.tsv: was boot_ram.c).
   at: boot_flash_loader 0x3F5004 (load address; it runs at 0x0400); boot_rx_active ..
   boot_rx_frame */
#include "F2806x_Device.h"
#include "F2806x_SysCtrl.h"
#include "F2806x_PieCtrl.h"
#include "F2806x_Gpio.h"
#include "F2806x_Sci.h"
#include "Flash2806x_API_Library.h"
#include "boot.h"
#include "util.h"

// ebss-begin
Uint16 boot_rx_active = 0;
FLASH_ST FlashStatus;
Uint16 boot_reply[5] = {0};
Uint16 boot_prog_buf[70] = {0};
Uint16 boot_rx_frame[140] = {0};
// ebss-end

#pragma CODE_SECTION(boot_flash_loader, "ramfuncs");
/* The update routine (see the file header). The reply code is a Uint16 of its own (r); the
   flash API's status goes to the 32-bit loop counter i first. */
void boot_flash_loader(void)
{
    Uint16 n;
    Uint16 c;
    Uint16 reset;
    Uint16 r;
    Uint32 timeout;
    Uint16 len;
    Uint16 *addr;
    Uint32 off;
    Uint16 sum;
    Uint32 i;
    Uint16 s2;
    Uint32 l2;
    Uint32 rx;

    timeout = 0;
    for (i = 0; i < 140; i++) boot_rx_frame[i] = 0;
    for (i = 0; i < 70; i++) boot_prog_buf[i] = 0;
    n = 0;
    c = 0;
    reset = 0;

    EALLOW;
    SysCtrlRegs.CLKCTL.bit.XTALOSCOFF = 0;
    for (i = 0; i < 20000; i++) {
    }
    SysCtrlRegs.CLKCTL.bit.XCLKINOFF = 1;
    SysCtrlRegs.CLKCTL.bit.OSCCLKSRC2SEL = 0;
    SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL = 1;
    SysCtrlRegs.CLKCTL.bit.WDCLKSRCSEL = 0;
    SysCtrlRegs.CLKCTL.bit.INTOSC2OFF = 1;
    SysCtrlRegs.CLKCTL.bit.INTOSC1OFF = 0;
    EDIS;

    if (SysCtrlRegs.PLLSTS.bit.MCLKSTS != 0) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKCLR = 1;
        EDIS;
        asm(" ESTOP0");
    }
    if (SysCtrlRegs.PLLSTS.bit.DIVSEL != 0) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.DIVSEL = 0;
        EDIS;
    }
    if (SysCtrlRegs.PLLCR.bit.DIV != 9) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKOFF = 1;
        SysCtrlRegs.PLLCR.bit.DIV = 9;
        EDIS;
        while (SysCtrlRegs.PLLSTS.bit.PLLLOCKS != 1) {
        }
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKOFF = 0;
        EDIS;
    }
    EALLOW;
    SysCtrlRegs.PLLSTS.bit.DIVSEL = 2;
    EDIS;

    EALLOW;
    SysCtrlRegs.LOSPCP.all = 0x0002;
    SysCtrlRegs.XCLK.bit.XCLKOUTDIV = 2;
    SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 1;
    EDIS;

    DINT;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 0;
    PieCtrlRegs.PIEIER1.all = 0;
    PieCtrlRegs.PIEIER2.all = 0;
    PieCtrlRegs.PIEIER3.all = 0;
    PieCtrlRegs.PIEIER4.all = 0;
    PieCtrlRegs.PIEIER5.all = 0;
    PieCtrlRegs.PIEIER6.all = 0;
    PieCtrlRegs.PIEIER7.all = 0;
    PieCtrlRegs.PIEIER8.all = 0;
    PieCtrlRegs.PIEIER9.all = 0;
    PieCtrlRegs.PIEIER10.all = 0;
    PieCtrlRegs.PIEIER11.all = 0;
    PieCtrlRegs.PIEIER12.all = 0;
    PieCtrlRegs.PIEIFR1.all = 0;
    PieCtrlRegs.PIEIFR2.all = 0;
    PieCtrlRegs.PIEIFR3.all = 0;
    PieCtrlRegs.PIEIFR4.all = 0;
    PieCtrlRegs.PIEIFR5.all = 0;
    PieCtrlRegs.PIEIFR6.all = 0;
    PieCtrlRegs.PIEIFR7.all = 0;
    PieCtrlRegs.PIEIFR8.all = 0;
    PieCtrlRegs.PIEIFR9.all = 0;
    PieCtrlRegs.PIEIFR10.all = 0;
    PieCtrlRegs.PIEIFR11.all = 0;
    PieCtrlRegs.PIEIFR12.all = 0;
    IER = 0x0000;
    IFR = 0x0000;

    EALLOW;
    FlashRegs.FOPT.bit.ENPIPE = 1;
    FlashRegs.FBANKWAIT.bit.PAGEWAIT = 3;
    FlashRegs.FBANKWAIT.bit.RANDWAIT = 3;
    FlashRegs.FOTPWAIT.bit.OTPWAIT = 5;
    FlashRegs.FSTDBYWAIT.bit.STDBYWAIT = 0x01FF;
    FlashRegs.FACTIVEWAIT.bit.ACTIVEWAIT = 0x01FF;
    EDIS;
    asm(" RPT #7 || NOP");

    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;
    EDIS;

    SciaRegs.SCICTL1.all = 0x0023;
    SciaRegs.SCICCR.all = 0x0007;
    SciaRegs.SCICTL1.all = 0x0003;
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 0;
    SciaRegs.SCIHBAUD = 0x0001;
    SciaRegs.SCILBAUD = 0x0024;
    SciaRegs.SCICTL1.all = 0x0023;

    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO27 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO27 = 1;
    Flash_CallbackPtr = 0;
    Flash_CPUScaleFactor = SCALE_FACTOR;
    GpioDataRegs.GPASET.bit.GPIO27 = 0;  /* writing 0 to a SET bit does nothing */
    EDIS;

    for (;;) {
        while (!SciaRegs.SCIRXST.bit.RXRDY) {
            if (SciaRegs.SCIRXST.bit.RXERROR) {
                SciaRegs.SCICTL1.all = 0x0003;
                SciaRegs.SCICTL2.all = 0x0003;
                SciaRegs.SCICTL1.bit.RXERRINTENA = 1;
                SciaRegs.SCICTL1.all = 0x0023;
            }
            if (++timeout > 20000000) {
                for (i = 0; i < 140; i++) boot_rx_frame[i] = 0;
                for (i = 0; i < 70; i++) boot_prog_buf[i] = 0;
                boot_rx_active = 0;
                n = 0;
                c = 0;
                timeout = 0;
            }
        }
        boot_rx_active = 1;
        timeout = 0;
        if (SciaRegs.SCIRXST.bit.RXRDY) c = ((volatile Uint16 *)&SciaRegs)[7] & 0xFF;
        boot_rx_frame[n] = c;
        if (n == 0) {
            if (boot_rx_frame[0] != 0x0A) continue;
        } else if (n == 1) {
            if (boot_rx_frame[1] >= 140) { n = 0; continue; }  /* a len of 136..139 still writes up to boot_rx_frame[143] */
            if (boot_rx_active == 0 && boot_rx_frame[1] != 0) { n = 0; continue; }  /* never true: boot_rx_active was set to 1 above */
            len = boot_rx_frame[1];
        } else if (n == 2) {
            if (boot_rx_active == 0 && boot_rx_frame[2] != BOOT_CMD_HELLO) { n = 0; continue; }  /* never true, as above */
            boot_rx_active = 1;
        }
        if (n - len == 4) {
            sum = 0;
            for (i = 0; i < len + 3; i++) sum += boot_rx_frame[i];
            if (sum == (boot_rx_frame[n - 1] << 8) + boot_rx_frame[n]) {
                n = 0;
                r = 0;
                switch (boot_rx_frame[2]) {
                case BOOT_CMD_DATA:
                    off = (long)((boot_rx_frame[3] << 8) + boot_rx_frame[4]) << 6;
                    if (addr + off < (Uint16 *)0x3F4000) {  /* only the first word is checked, and len is not */
                        for (i = 0; i < 64; i++) boot_prog_buf[i] = (boot_rx_frame[2 * i + 5] << 8) + boot_rx_frame[2 * i + 6];
                        i = Flash_Program(addr + off, boot_prog_buf, 64, &FlashStatus);
                        if (i == 0) r = BOOT_REPLY_OK; else r = i;
                    } else r = BOOT_REPLY_OK;
                    break;
                case BOOT_CMD_ADDR:
                    addr = (Uint16 *)(((((((Uint32)boot_rx_frame[3] << 8) + boot_rx_frame[4]) << 8) + boot_rx_frame[5]) << 8) + boot_rx_frame[6]);
                    r = BOOT_REPLY_OK;
                    break;
                case BOOT_CMD_ERASE:
                    i = Flash_Erase(0xFE, &FlashStatus);  /* sectors B..H; sector A (this loader) stays */
                    if (i != 0) { r = i; break; }
                case BOOT_CMD_HELLO:
                    r = BOOT_REPLY_OK;
                    break;
                case BOOT_CMD_END:
                    r = BOOT_REPLY_OK;
                    reset = 1;
                    break;
                }
            } else {
                n = 0;
                r = BOOT_REPLY_BAD_SUM;
            }
            boot_reply[0] = 0x0A;
            boot_reply[1] = 0;
            boot_reply[2] = r;
            sum = boot_reply[0] + boot_reply[1];
            sum += boot_reply[2];
            boot_reply[3] = sum >> 8;
            boot_reply[4] = sum & 0xFF;
            for (i = 0; i < 5; i++) {
                SciaRegs.SCITXBUF = boot_reply[i];
                while (!SciaRegs.SCICTL2.bit.TXRDY) {
                }
            }
            if (reset == 1) {
                for (i = 0; i < 50000; i++) {
                }
                GpioDataRegs.GPASET.bit.GPIO27 = 1;  /* I take GPIO27 to ask an outside part for a reset */
                for (;;) {
                }
            }
        } else {
            n++;
        }
    }
}
