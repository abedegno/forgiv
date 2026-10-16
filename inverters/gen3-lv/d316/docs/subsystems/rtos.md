# The RTOS

Files: `src/rtos/os_main.c`, `os_task.c`, `os_event.c`, `os_tick.c`, `os_tick_isr.c`,
`os_cpu_a.asm`, `os_cpu_sr.asm`, `src/include/os.h`.

A small custom kernel with uC/OS-like names: eight task control blocks, one 32-bit event word per
task, a 1 ms tick and a context switch through TRAP #31.

## Start

boot_main jumps to start (0x3E8004), which turns the watchdog off and runs c_int00. main then
calls init_system (device_init.c) and app_init, creates the tasks with os_task_create, sets each
task's tick period and first count, turns interrupts on (EINT, ERTM) and calls os_start. os_start
restores the first frame of task 0 (os_start_high) and never returns.

## Tasks

| Id | Task | Stack (words) | Period | First count | Events besides the tick |
|---|---|---|---|---|---|
| 0 | task0_state_machine | 0xE4 | 20 ms | 1 | EV0_STOP_FAULT 1, EV0_GRID_FAULT 2, EV0_PV_FAULT 3, EV0_STOP 4, EV0_EPS_FAULT 5, EV0_UPDATE 6, EV0_BOOT 7 |
| 1 | task1_ac_measure | 0xC8 | 20 ms | 2 | EV1_GRID_PERIOD 1, EV1_RMS_SUMS 2 |
| 2 | task2_control | 0xD4 | 20 ms | 3 | none |
| 3 | task3_measure | 0xF2 | 10 ms | 4 | EV3_SLOW_AVG 1 |
| 4 | task4 (grid_support.c) | 0xC8 | 20 ms | 5 | none |
| 5 | task5_arm_link | 0xC8 | 60 ms | 7 | EV5_FRAME 8 |
| 6 | task6_bms_link | 0x12C | 40 ms | 9 | none |

TCB 7 exists but no task uses it. The stacks come one after another from os_stack (0xDBC0) and
grow up, as the C28x stack does. The first counts stagger the first ticks so that the tasks do
not all wake on the same tick.

Who posts the events (os.h):

| Event | Posted by |
|---|---|
| EV_TICK (bit 0, every task) | os_tick, from timer2_isr |
| EV0_STOP_FAULT | task3_measure: has_stop_fault |
| EV0_GRID_FAULT | task3_measure and ac_measure.c: has_grid_fault, the grid loss watchdog |
| EV0_PV_FAULT | task3_measure: has_pv_fault |
| EV0_STOP | the control interrupt's fast faults (converter_ctrl.c), ARM run command 10, the ARM's aging test (arm_page2), dc_measure.c, bms_link.c, ac_measure.c |
| EV0_EPS_FAULT | task3_measure: has_eps_fault |
| EV0_UPDATE | arm_run_command: the 6th frame with ARM run command 4 |
| EV0_BOOT | state4_update itself, after 100 ticks |
| EV1_GRID_PERIOD | the grid period code in converter_ctrl.c |
| EV1_RMS_SUMS | eps_rms_accum.c |
| EV3_SLOW_AVG | dc_avg.c |
| EV5_FRAME | sci_rx: 45 bytes of a 0x43 frame |

## Priorities and scheduling

os_ready has one bit per ready task. os_highest returns the lowest set bit, so task 0 has the
highest priority and task 6 the lowest. With no task ready it returns 0.

Scheduling is cooperative. os_sched, the only place that switches tasks, is called only from
os_wait. A post (os_post, os_post_isr, or the tick) sets the event bit and the ready bit but
does not switch. So a task keeps the CPU until it calls os_wait, and only then does the highest
ready task run. os_sched_isr, which would switch at the end of an interrupt, has no callers.

When no task is ready, os_highest gives 0, so task 0 is resumed even though it is not ready, and
its os_wait returns 0. task0_state_machine's state functions loop on os_wait(0), so task 0 is
also the idle loop.

## Events

- `os_wait(mask)` stores mask (the bits not waited for; every caller passes 0), returns the
  pending bits that are not masked and clears them. With none pending it clears the task's ready
  bit and calls os_sched.
- `os_post(id, ev)` (task code, interrupts off around it) sets bit ev and makes the task ready if
  that bit is not masked.
- `os_post_isr(id, ev)` does the same without DINT/EINT, for interrupts. arm_page2 also calls it
  from task 5.
- `os_post_flag` (set a bit without making the task ready), `os_take_events` and `os_sleep` have
  no callers.

## The tick

CPU timer 2 counts 90,000 clocks (1 ms at 90 MHz) and interrupts on INT14. timer2_isr calls
os_tick, which counts each task's ticks and posts EV_TICK when the count reaches the period. The
tick never switches tasks.

## The context switch (TRAP #31)

os_sched sets os_next and os_cur and points os_next_tcb at the next task's TCB, then executes
TRAP #31. Vector 31 is USER12 in the PIE table; init_system puts os_ctx_sw_isr there. The CPU
pushes its interrupt frame (ST0, T, AL, AH, PL, PH, AR0, AR1, ST1, DP, IER, DBGSTAT, the return
address). os_ctx_sw_isr aligns the stack (ASP), pushes DP:ST1, AR1H:AR0H, XAR2..XAR7, XT and RPC,
stores SP in os_cur_tcb->sp, loads SP from os_next_tcb->sp, sets os_cur_tcb = os_next_tcb, pops
the same registers, NASP and IRET. When the same task is chosen again, it saves and restores
itself.

os_stack_init builds this frame for a new task: zeros, ST1 = 0x0A0A (DBGM, VMAP, OBJMODE,
M0M1MAP; interrupts on), IER with INT14 set, and the task's entry as the return address. It
returns base + 35. I read the odd address as deliberate: C28x 32-bit accesses ignore address
bit 0, so the pops find each value at the even address the frame was built for.

os_int_ctx_sw is the same switch for the end of an interrupt (it drops two return addresses and
joins the switch after the save). It is reached only from os_sched_isr, which has no callers.
os_cpu_sr_save and os_cpu_sr_restore (os_cpu_sr.asm) have no callers either; the C code uses
DINT and EINT.

## Critical sections

The RTOS functions use DINT and EINT. EINT always enables interrupts, so a call to os_post or
os_wait from code that had interrupts off turns them back on.

## Open points

See docs/findings-comm.md: the word sci.h calls arm_rx_block is os_tcb[5].events, and sci_rx
clears it on every ARM byte.
