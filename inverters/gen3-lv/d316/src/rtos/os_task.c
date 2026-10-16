/* os_task.c: the RTOS's state, start and task creation, and the switch from an interrupt.

   Data (0xDA40 .. 0xDABF):
     os_cur        the running task's id (set by os_sched before the switch)
     os_next       the task os_sched chose
     os_ready      one bit per ready task, bit n for task n
     os_next_tcb   the TCB the context switch restores
     os_stack_free the next free word of os_stack
     os_cur_tcb    the running task's TCB (set by the context switch)
     os_tcb[8]     the task control blocks (OS_TCB, os.h), 8 words each at 0xDA80. Note that
                   os_tcb[5].events, task 5's event word at 0xDAAC, is the word that sci.h calls
                   arm_rx_block (sci_rx clears it on every ARM byte; docs/findings-comm.md).
   Entry points: os_init, os_start, os_task_create (from main); os_sched_isr has no callers.
   Neighbours: os_main.c, os_event.c, os_tick.c, os_cpu_a.asm (os_start_high, os_int_ctx_sw).
   name: descriptive (map/filenames.tsv), name kept.
   at: os_init 0x3F19D3, os_start 0x3F19DA, os_task_create 0x3F19ED, os_sched_isr 0x3F1A0B;
   os_cur 0xDA40 .. os_tcb 0xDA80 */
#include "os.h"
#include "util.h"

/* ebss-begin */
volatile unsigned int os_cur;
unsigned int os_next;
volatile unsigned int os_ready;
OS_TCB * volatile os_next_tcb;
unsigned int * volatile os_stack_free;
OS_TCB * volatile os_cur_tcb;
OS_TCB os_tcb[OS_TASKS];
/* ebss-end */

/* Sets up the RTOS before any task exists: the stack allocator, the state and the TCBs. */
void os_init(void)
{
    os_init_stack();
    os_init_vars();
    os_init_tcbs();
}

/* Starts the highest ready task (task 0, since all are ready after os_task_create) by
   restoring its first frame (os_start_high). Does not return. */
void os_start(void)
{
    os_next = os_highest();
    os_cur_tcb = os_next_tcb = &os_tcb[os_next];
    os_cur = os_next;
    os_start_high();
}

/* Creates task id: takes stk words from os_stack, builds the first frame there
   (os_stack_init) and marks the task ready. Its period and count are 0; main sets them
   afterwards. */
void os_task_create(unsigned int id, unsigned int stk, void (*task)(void))
{
    unsigned int *top, *sp;
    OS_TCB *t;
    top = os_stack_free;
    os_stack_free += stk;
    sp = os_stack_init(task, top);
    t = &os_tcb[id];
    t->sp = sp;
    t->period = 0;
    t->count = 0;
    t->events = 0;
    os_ready |= 1 << id;
}

/* The reschedule for the end of an interrupt: when a higher task is ready it switches to it
   through os_int_ctx_sw. Nothing calls it, so the RTOS never switches tasks from an interrupt. */
void os_sched_isr(void)
{
    os_next = os_highest();
    if (os_next != os_cur) {
        os_cur = os_next;
        os_next_tcb = &os_tcb[os_next];
        os_int_ctx_sw();
    }
}
