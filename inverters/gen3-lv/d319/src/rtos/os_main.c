/* os_main.c: the RTOS scheduler, main and start.

   start (0x3E8004, app_entry) is where boot_main sends a valid application: it disables the
   watchdog and runs c_int00, the C start-up, which calls main. main sets up the device
   (init_system, device_init.c) and the application (app_init), creates the seven tasks and starts
   the first one. Each task gets a stack from os_stack (0xDBC0, growing up), a tick period in
   1 ms ticks and a first count that staggers the first ticks:

     id  task                  stack   period  first count  priority
     0   task0_state_machine   0xE4    20 ms   1            highest; also the idle loop
     1   task1_ac_measure      0xC8    20 ms   2
     2   task2_control         0xD4    20 ms   3
     3   task3_measure         0xF2    10 ms   4
     4   task4 (grid_support)  0xC8    20 ms   5
     5   task5_arm_link        0xC8    60 ms   7
     6   task6_bms_link        0x12C   40 ms   9            lowest

   The priority is the task number, lowest first (os_highest). Scheduling is cooperative:
   os_sched runs only from os_wait, so a task keeps the CPU until it waits; the tick and the
   interrupts only post events (os_event.c).

   Entry points: main (from c_int00), start (from boot_main), os_sched (from os_wait),
   os_highest, os_init_stack, os_init_vars, os_init_tcbs (from os_init).
   Data: os_unmap_tbl, the lowest set bit of each 4-bit value (0xD2EC).
   Neighbours: os_task.c, os_event.c, os_tick.c, os_cpu_a.asm (TRAP #31, os_ctx_sw_isr),
   drivers/device_init.c (init_system), state/app_init.c, the task functions.
   name: descriptive (map/filenames.tsv), name kept.
   at: os_highest 0x3F17E1 .. start 0x3F18BA; os_unmap_tbl 0xD2EC */
#include "os.h"
#include "state.h"
#include "init.h"
#include "F2806x_SysCtrl.h"
#include "util.h"

/* the lowest set bit of a 4-bit group of os_ready (0xFF: none) */
unsigned int os_unmap_tbl[16] = {0xFF, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

/* Returns the ready task with the lowest number, looking at 4 bits of os_ready at a time
   through os_unmap_tbl. With no task ready it returns 0, task 0. */
unsigned int os_highest(void)
{
    unsigned int x;
    if ((x = os_ready & 0xF) != 0) return os_unmap_tbl[x];
    if (os_ready & 0xF0) return os_unmap_tbl[(os_ready & 0xF0) >> 4] + 4;
    if (os_ready & 0xF00) return os_unmap_tbl[(os_ready & 0xF00) >> 8] + 8;
    if (os_ready & 0xF000) return os_unmap_tbl[os_ready >> 12] + 12;
    return 0;
}

/* Switches to the highest ready task: sets os_cur, os_next and os_next_tcb, then TRAP #31
   enters os_ctx_sw_isr (the USER12 vector, os_cpu_a.asm), which saves this task's context and
   restores the next one's. Returns when this task is switched back in. Called only by os_wait. */
void os_sched(void)
{
    DINT;
    os_next = os_highest();
    os_cur = os_next;
    os_next_tcb = &os_tcb[os_next];
    asm(" TRAP #31");
    EINT;
}

/* Starts the stack allocator at os_stack (os_task_create takes each task's stack from it). */
void os_init_stack(void)
{
    os_stack_free = os_stack;
}

/* Clears the scheduler's state: no task ready, task 0 current, no TCB pointers. */
void os_init_vars(void)
{
    os_ready = 0;
    os_cur = 0;
    os_next = 0;
    os_next_tcb = 0;
    os_cur_tcb = 0;
}

/* Clears the 8 task control blocks; the mask 0xFFFFFFFF means the task waits for nothing yet. */
void os_init_tcbs(void)
{
    OS_TCB *t;
    unsigned int i;
    t = os_tcb;
    for (i = 0; i < 8; i++, t++) {
        t->sp = 0;
        t->period = 0;
        t->count = 0;
        t->events = 0;
        t->mask = 0xFFFFFFFF;
    }
}

/* The application's main: device and application set-up, the seven tasks (see the file
   header), interrupts on (EINT, ERTM), then os_start, which never returns. */
void main(void)
{
    init_system();
    app_init();
    os_init();
    os_task_create(0, 0xE4, task0_state_machine);
    os_tcb[0].count = 1;
    os_tcb[0].period = 20;
    os_task_create(1, 0xC8, task1_ac_measure);
    os_tcb[1].count = 2;
    os_tcb[1].period = 20;
    os_task_create(2, 0xD4, task2_control);
    os_tcb[2].count = 3;
    os_tcb[2].period = 20;
    os_task_create(3, 0xF2, task3_measure);
    os_tcb[3].count = 4;
    os_tcb[3].period = 10;
    os_task_create(4, 0xC8, task4);
    os_tcb[4].count = 5;
    os_tcb[4].period = 20;
    os_task_create(5, 0xC8, task5_arm_link);
    os_tcb[5].count = 7;
    os_tcb[5].period = 60;
    os_task_create(6, 0x12C, task6_bms_link);
    os_tcb[6].count = 9;
    os_tcb[6].period = 40;
    EINT;
    ERTM;
    os_start();
}

/* The application's entry from boot_main (app_entry, 0x3E8004): watchdog off (WDCR 0x68),
   then c_int00. The watchdog stays off for good; nothing in the image enables or services it. */
void start(void)
{
    EALLOW;
    SysCtrlRegs.WDCR = 0x0068;
    EDIS;
    c_int00();
}
