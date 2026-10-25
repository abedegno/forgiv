/* os_event.c: the RTOS's events. Each task has a 32-bit word of posted event bits and a mask
   of the bits it does not wait for (OS_TCB.events and .mask). A post sets a bit and, when that
   bit is not masked, marks the task ready in os_ready. os_wait returns the unmasked bits and
   clears them, and blocks while there are none.

   The RTOS is cooperative: no post switches tasks. A posted task runs when the running task next
   calls os_wait and it is then the highest ready task (the lowest task number). The tick
   (os_tick, from timer2_isr) posts with os_post_isr and does not switch either.

   The events in use (os.h): bit 0 EV_TICK, every task's periodic tick (os_tick). Task 0:
   EV0_STOP_FAULT 1, EV0_GRID_FAULT 2, EV0_PV_FAULT 3, EV0_STOP 4, EV0_EPS_FAULT 5, EV0_UPDATE 6,
   EV0_BOOT 7. Task 1: EV1_GRID_PERIOD 1, EV1_RMS_SUMS 2. Task 3: EV3_SLOW_AVG 1. Task 5:
   EV5_FRAME 8 (sci_rx). Tasks 2, 4 and 6 get only the tick.

   Entry points: os_wait, os_post (task code), os_post_isr (interrupts). os_take_events and
   os_post_flag have no callers.
   Data: none of its own; os_tcb, os_ready, os_cur and os_cur_tcb are in os_task.c.
   Neighbours: os_main.c (os_sched), os_task.c, os_tick.c; every task and several interrupts.
   name: descriptive (map/filenames.tsv), name kept; the RTOS is a small custom one with uC/OS-like
   names.
   at: os_wait 0x3F16CE .. os_post_isr 0x3F173D */
#include "os.h"
#include "util.h"

/* Waits for any event bit of the running task that is not in mask (every caller passes 0, so
   any bit), returns those bits and clears them. With none pending it clears the task's ready
   bit and calls os_sched, which runs the highest ready task. When no task is ready, os_highest
   gives task 0, so task 0 resumes and its os_wait returns 0: task 0's loop is also the idle
   loop. */
unsigned long os_wait(unsigned long mask)
{
    OS_TCB *t;
    unsigned long ev, r;
    DINT;
    t = os_cur_tcb;
    t->mask = mask;
    ev = t->events;
    r = ev & ~mask;
    if (r) {
        t->events &= mask;
        EINT;
        return r;
    }
    os_ready &= ~(1u << os_cur);
    EINT;
    os_sched();
    DINT;
    t = os_cur_tcb;
    ev = t->events;
    t->events &= mask;
    EINT;
    return ev & ~mask;
}

/* Returns all of the running task's events and clears them, without waiting. No callers. */
unsigned long os_take_events(void)
{
    OS_TCB *t;
    unsigned long ev;
    DINT;
    t = os_cur_tcb;
    ev = t->events;
    t->events = 0;
    EINT;
    return ev;
}

/* Posts event ev to task id from task code (interrupts off around it): sets the bit and makes
   the task ready when the bit is one it waits for. It does not switch tasks. */
void os_post(unsigned int id, unsigned int ev)
{
    OS_TCB *t;
    DINT;
    t = &os_tcb[id];
    t->events |= 1 << ev;
    if (t->events & ~t->mask)
        os_ready = os_ready | (1 << id);
    EINT;
}

/* Sets event bit ev of task id without making it ready. No callers. */
void os_post_flag(unsigned int id, unsigned int ev)
{
    DINT;
    os_tcb[id].events |= 1 << ev;
    EINT;
}

/* os_post for interrupt code, without DINT/EINT (the interrupt has them off already). One task
   calls it too (arm_page2, from task 5), which can race with the tick; see
   docs/findings-comm.md. */
void os_post_isr(unsigned int id, unsigned int ev)
{
    OS_TCB *t;
    t = &os_tcb[id];
    t->events |= 1 << ev;
    if (t->events & ~t->mask)
        os_ready = os_ready | (1 << id);
}

