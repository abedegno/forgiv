/* os_tick.c: the RTOS tick, task sleep and a new task's first stack frame.

   The tick is CPU timer 2 at 1 ms (timer2_init: 90 MHz x 1000 us) and its interrupt timer2_isr
   (os_tick_isr.c). os_tick posts EV_TICK to each task whose period has elapsed.

   Entry points: os_tick (from timer2_isr), os_stack_init (from os_task_create); os_sleep has no
   callers.
   Data: none of its own (os_tcb).
   Neighbours: os_tick_isr.c, os_task.c, os_event.c (os_post_isr, os_wait), os_cpu_a.asm (the
   frame layout os_stack_init builds).
   name: descriptive (map/filenames.tsv), name kept.
   at: os_tick 0x3F1A9D .. os_stack_init 0x3F1ACC */
#include "os.h"
#include "util.h"

/* Counts each task's ticks; when a task with a period reaches it, the count restarts and the
   task gets EV_TICK (os_post_isr). Runs in timer2_isr; it never switches tasks. */
void os_tick(void)
{
    OS_TCB *t;
    unsigned int i;
    i = 0;
    for (t = os_tcb; t <= &os_tcb[7]; t++, i++) {
        if (t->period != 0) {
            if (++t->count >= t->period) {
                t->count = 0;
                os_post_isr(i, EV_TICK);
            }
        }
    }
}

/* Waits once for task id's tick after ticks ticks, then gives it period as its new tick
   period. Nothing calls it. */
void os_sleep(unsigned int id, unsigned int ticks, unsigned int period)
{
    OS_TCB *t;
    t = &os_tcb[id];
    t->count = 0;
    t->period = ticks;
    os_wait(0);
    t->count = 0;
    t->period = period;
}

/* Builds a new task's first frame at sp, the frame os_ctx_sw_isr and os_start_high restore,
   and returns the task's saved SP:
     0..13   the interrupt frame IRET pops: ST0, T, AL, AH, PL, PH, AR0, AR1 = 0; ST1 = 0x0A0A
             (DBGM, VMAP, OBJMODE, M0M1MAP; INTM clear, so interrupts are on); DP = 0;
             IER = the current IER with INT14 (the tick) on; DBGSTAT = 0; the return address =
             the task's entry
     14..33  what os_ctx_sw_isr pushes after it: DP:ST1 (ST1 0x0A0A again), AR1H:AR0H,
             XAR2..XAR7, XT and RPC, all 0
   The returned SP is sp + 35, an odd address. The C28x ignores bit 0 of the address in 32-bit
   accesses, and the save runs after ASP, so I read the odd SP as the aligned state the switch
   expects; the pops then find each 32-bit value at an even address. */
unsigned int *os_stack_init(void (*task)(void), unsigned int *sp)
{
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0x0A0A;
    *sp++ = 0;
    *sp++ = IER | 0x2000;
    *sp++ = 0;
    *sp++ = (unsigned long)task;
    *sp++ = (unsigned long)task >> 16;
    *sp++ = 0x0A0A;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp++ = 0;
    *sp = 0;
    sp += 2;
    return sp;
}
