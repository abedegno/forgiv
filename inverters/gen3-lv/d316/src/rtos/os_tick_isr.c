#include "os.h"
#include "util.h"
/* os_tick_isr.c: the CPU timer 2 interrupt (INT14, put in the PIE table by init_system), the
   RTOS tick: every 1 ms it runs os_tick, which posts EV_TICK to the tasks whose period is up.
   It does not switch tasks.
   Entry point: timer2_isr. Data: none. Neighbours: os_tick.c, drivers/device_init.c
   (timer2_init, the vector).
   Its last words (SETC INTM|DBGM before POP RB) are what 15.12.2 and later compile; 15.12.0
   and 15.12.1 leave the SETC out (defect SDSCM00039432).
   name: descriptive (map/filenames.tsv: was rtos_isr.c).
   at: timer2_isr 0x3F11FD */

/* The 1 ms tick. */
interrupt void timer2_isr(void)
{
    os_tick();
}
