// clang-format off
/* Sets a block of constants: a pointer-sized 0x1000 and the IQ30 values 1/6 and 1/12. Nothing
   else in the image reads iq_flag .. iq_consts_zero, so this is dead data (probably left from a TI
   IQmath or library set-up).
   Entry point: iq_consts_init, called once by init_system (device_init.c).
   Data: iq_flag, iq_count and iq_consts at 0xCCB2 .. 0xCCBA.
   name: descriptive (was coef_init.c; map/filenames.tsv).
   at: iq_consts_init 0x3F1B96; iq_flag 0xCCB2 .. 0xCCBA */
// clang-format on
#include "util.h"

// clang-format off
/* ebss-begin */
unsigned int iq_count;
unsigned int iq_flag = 0;
struct iq_consts_t iq_consts;   /* 0xCCB4 .. 0xCCBA */
/* ebss-end */
// clang-format on

/* Writes the constants (0x0AAA0000 = 1/6 and 0x05550000 = 1/12 in IQ30, rounded down) and clears
   the count. */
void iq_consts_init(void) {
    iq_consts.k1000h = 0x1000;
    iq_count = 0;
    iq_consts.zero = 0;
    iq_consts.one_sixth = 0x0AAA0000;
    iq_consts.one_twelfth = 0x05550000;
}
