// clang-format off
/* at: sys_delay_loops 0x08014310 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* main_p4.c: one dead delay helper that sits between main's literal pool and stm32f10x_it.c.

   Entry points: sys_delay_loops (no caller, no pointer). Data: none.
   Neighbours: main_p1.c before, stm32f10x_it.c after.

   name: likely. That it came from main.c is a guess; it cannot be in the same object as main,
   because one armcc object has one literal pool at its end and main's pool (0x080142F4) comes
   before this function (map/filenames.tsv). */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/sys.h"

/* Dead. Busy-waits n x 5 empty loop passes. */
void sys_delay_loops(uint loops) {
    uint i = 0;
    uchar j = 0;

    for (i = 0; i < loops; i = i + 1) {
        for (j = 0; j < 5; j = j + 1) {
        }
    }
    return;
}
