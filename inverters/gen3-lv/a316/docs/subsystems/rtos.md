# The task scheduler

The A316 ARM firmware runs eight tasks under a small scheduler of the vendor's own. It uses
uC/OS-II's names (OSInit, OSTaskCreate, OSStart, OS_Sched, OSTimeTick, OSTCBCur, OSRdyGrp ...)
and one file of Micrium's Cortex-M3 port, but it is not uC/OS-II: there are no event control
blocks, semaphores, queues, delays in ticks or idle task, and no task is ever pre-empted.

Sources: `src/rtos/` (os_core.c, os_cpu_c.c, os_cpu_a.s, os_flag.c, os_init.c, os_task.c,
os_time.c), `src/include/rtos_types.h` and `src/include/tasks.h`. The task set-up is in `main`
(`src/app/main_p1.c`) and `task_monitor` (`src/app/control/monitor.c`). The names in `src/rtos`
are uC/OS-II's where the function does what uC/OS-II's does (`map/filenames.tsv` rates the
files "likely"), and mine otherwise (`os_flags_pend`, `os_task_delay`, `OS_InitStkPool`).

## What comes from Micrium

- `OSTaskStkInit` (os_cpu_c.c) is the uC/OS-II Cortex-M3 port's code word for word: the
  exception frame with xPSR 0x01000000, PC = the task, LR 0xFFFFFFFE, the fill values
  0x12121212 to 0x01010101, R0 = the argument, then R11 to R4 (0x11111111 to 0x04040404).
- `os_cpu_a.s` is in the shape of the port's `os_cpu_a.asm` (`OSStartHighRdy`, `OSCtxSw`,
  `OSIntCtxSw`, `PendSV_Handler`), cut down: R4 and R5 as scratch, one word store of
  0xFFFF0000 to SHPR3 (PendSV and SysTick both at the lowest priority), no OSRunning, no
  OSTaskSwHook call, a one-byte OSPrioCur, interrupts enabled again between saving and
  restoring, and no OSStartHang loop (docs/matching.md, round 4).
- `OSTaskSwHook` is an empty function after `OSTaskStkInit`, as in the port; nothing calls it.

The rest is the vendor's.

## Data

| name | address | what |
|---|---|---|
| `task_tbl` | 0x20007C6C | eleven `OS_TCB`s of 20 bytes, one per priority 0 to 10 (the map row is 0xC8 bytes and a separate row `g_20007D34` holds TCB 10) |
| `OSRdyGrp` | 0x200004C8 | one ready bit per task (bits 0 to 11 are looked at) |
| `OSUnMapTbl` | 0x200004A6 | lowest set bit of a 4-bit value, a table in RAM (`.data`) |
| `OSPrioHighRdy` | 0x200004CA | the task to run next |
| `OSPrioCur` (map: `os_task_dly_idx`) | 0x200004CC | the running task; tasks pass it to `os_task_delay` as their own index, hence the map's name |
| `OSTCBHighRdy`, `OSTCBCur` | 0x200004D0, 0x200004D4 | TCB pointers used by PendSV_Handler |
| `os_stk_pool` | 0x20003DEC | 4000 words of task stack, zeroed by `OS_InitStkPool` |
| `os_stk_next` | 0x200004D8 | next free stack top; starts at `os_stk_pool_top` (0x20007C68, the pool's last word) |

An `OS_TCB` (`rtos_types.h`) is:

| offset | field | use |
|---|---|---|
| 0x00 | `StkPtr` | saved stack pointer (PendSV_Handler) |
| 0x04 | (Reserved_) | the task's stack top, set by `OSTaskCreate`, read by `OSTaskStkChk` |
| 0x08 | (Reserved_) | the stack size in words, the same |
| 0x0A | `period` | tick period in ms; 0 for none |
| 0x0C | `count` | ticks since the last wake-up |
| 0x0E | `FlagsRdy` | flags posted to the task |
| 0x10 | `FlagsWait` | mask of flags the task does not wait for |

## How it runs

- **Ticks.** `OS_CPU_SysTickInit` sets SysTick to `SystemCoreClock / 1000`, a 1 ms tick at
  72 MHz. `SysTick_Handler` calls `OSTimeTick`, which, for each TCB with a non-zero period,
  counts up and at the period posts flag 1 (bit 0) and sets the task's ready bit.
- **Waiting.** Every task loop ends in `os_flags_pend(0)`. If no flag is posted, the task's
  ready bit is cleared and `OS_Sched` switches to the highest ready task. When the task runs
  again, `os_flags_pend` returns the posted flags and clears them.
- **Choosing.** `OS_SchedNew` returns the lowest set bit of `OSRdyGrp`: priority 0 is the most
  urgent. If no bit is set it returns 7, so task 7 (`task_usb`) is the fall-back when nothing
  else is ready; there is no idle task.
- **Switching.** `OS_Sched` calls `OSCtxSw`, which pends PendSV. `PendSV_Handler` saves R4-R11
  on the process stack, stores the stack pointer in `OSTCBCur->StkPtr`, and loads the next
  task's. PendSV and SysTick are at the lowest interrupt priority.
- **No pre-emption.** The only caller of `OS_Sched` is `os_flags_pend`. `OSIntExit` (which
  would switch on the way out of an interrupt) has no callers, and no interrupt handler calls
  `OS_Sched`. So the tick and the interrupts only mark tasks ready; a task keeps the CPU until
  it waits. In effect the scheduler is cooperative, with priorities deciding who goes next.
- **Other wake-ups.** `ADC1_2_IRQHandler` posts flag 2 to task 0 (`os_flags_post(0, 2)`) at
  the end of each cycle of samples. `uart.c` and `dsp_link.c` set flag 0x10 in task 2's
  `FlagsRdy` and flag 8 in task 1's directly, without the ready bit: those tasks see them on
  their next tick.
- **Sleeping.** `os_task_delay(idx, first, period)` sets the task's period to `first`, waits,
  then sets it back to `period`. Callers: `rtc.c`, `eeprom.c`, `meter.c` (3 ms steps of its
  poll), `modbus_slave.c`, `dsp_link.c`, `monitor.c` and `usbh_usr.c`.
- **Critical sections.** `OS_ENTER_CRITICAL` and `OS_EXIT_CRITICAL` are functions that call
  `sys_irq_disable` and `sys_irq_enable` (`cpsid i`, `cpsie i`). They do not save the old
  state, so they do not nest.

Unused parts: `OSIntExit`, `OSTaskStkChk` (stack use in words, by scanning for ten zero
words), `os_flags_accept`, `OSTaskSwHook`, and TCBs 8 to 10.

## Start-up

`main` (`src/app/main_p1.c`):

1. `SystemInit`, `sys_irq_disable`, `sys_startup_delay`, `board_init`.
2. `OSInit`: zero the stack pool, the scheduler variables and the TCBs.
3. Eight `OSTaskCreate(task, 0, stack words, priority)` calls, in the order below.
4. Task 5's period set to 20.
5. `nvic_init` (vector table, IRQ priorities, then interrupts on), `OS_CPU_SysTickInit`,
   `OSStart`.

`OSTaskCreate` does not make a task ready. `OSStart` runs the highest ready task, and since
none is ready that is task 7, `task_usb`. Its first pass sets its own period to 1 and waits.
After that only tasks 5 and 7 have a period, so `task_monitor` runs every 20 ms, `task_usb`
every millisecond and whenever nothing else is ready, and the others wait. Once the EEPROM settings are loaded
(`eeprom_load_ok` 1), `task_monitor` gives every other task a first period, and each task
starts on its first tick. From then on most tasks set their own period at the end of each
pass.

## The task table

Priority is the TCB index; 0 runs first when several are ready. The stacks are carved from
the top of the pool down, in the order `main` creates the tasks. All numbers are read from the
code: `OSTaskCreate` arguments in `main`, the periods from the `task_tbl[n].period` stores.

| prio | task | file | stack (words / bytes) | stack range | first period (ms) | period it sets itself (ms) |
|---|---|---|---|---|---|---|
| 0 | `task_measure` | control/measure.c | 300 / 1200 | 0x200077BC-0x20007C68 | 1000 | 25; also woken by the ADC (flag 2) |
| 1 | `task_dsp_rx` | comm/dsp_link.c | 300 / 1200 | 0x200061DC-0x20006688 | 150 | 200; 100 during a BMS update, 10 during a DSP update |
| 2 | `task_dsp_tx` | comm/dsp_link.c | 300 / 1200 | 0x2000668C-0x20006B38 | 20 | 60 |
| 3 | `task_modbus_rs485` | comm/modbus_slave.c | 400 / 1600 | 0x2000717C-0x200077B8 | 1500 | 5 |
| 4 | `task_meter` | comm/meter.c | 400 / 1600 | 0x20006B3C-0x20007178 | 1500 | 5 |
| 5 | `task_monitor` | control/monitor.c | 300 / 1200 | 0x20005D2C-0x200061D8 | 20 (set in `main`) | 20 |
| 6 | `task_schedule` | control/schedule_p3.c | 500 / 2000 | 0x2000555C-0x20005D28 | 500 | 500 |
| 7 | `task_usb` | usb/usb_task.c | 800 / 3200 | 0x200048DC-0x20005558 | runs first (`OSStart`) | 1 |

Creation order: 0, 3, 4, 2, 1, 5, 6, 7. The eight stacks take 3300 of the 4000 words; the 700
words from 0x20003DEC to 0x200048D8 are not used. Each range is the lowest word to the stack
top, which is the first word the task's initial frame uses.

Other period changes: a DSP firmware update (`dsp_link.c`) sets the periods of tasks 0, 5 and
6 to 0 (they stop) and stops TIM3; `modbus_slave.c` stops tasks 1 and 2 (period 0) on two
paths. `task_usb` writes period 1 on every pass, so the 500 that `task_monitor` gives it lasts one
pass at most.

The main stack (0x800 bytes from 0x20009D98, `startup_stm32f10x_cl.s`) is used before
`OSStart` and by the interrupt handlers; the tasks run on the process stack.
