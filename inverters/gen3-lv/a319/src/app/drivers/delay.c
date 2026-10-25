// clang-format off
/* at: sys_delay_loops 0x08014310 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* delay.c: retained busy-wait helper with no identified caller or function pointer.
   Entry: sys_delay_loops; no owned data. The preceding application_startup.c owns main.
   match: keep this unit separate; joining it to main moves it ahead of main's literal pool.
   name: descriptive; no evidence establishes the original filename. */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/sys.h"

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
