# Main and tasks

Files: `src/app/main_p1.c`, `src/app/main_p4.c`, `src/app/stm32f10x_it.c`, `src/app/util.c`,
`src/app/stubs.c`, and the scheduler in `src/rtos` (commented by the other pass; I describe here
only how the application uses it).

## Architecture

The ARM starts at `main` from Keil's `__rt_entry`. `main` sets the clock (`SystemInit`, 72 MHz),
masks interrupts, waits through `sys_startup_delay` (20 million empty loop passes), runs
`board_init`, creates eight tasks and starts the scheduler. `nvic_init` turns the interrupts
back on just before `OSStart`. The vector table sits at 0x08014000; `nvic_init` points VTOR there.
A bootloader lives below (inference from VTOR, from HR164's jump through the word at 0x2C and
from the version word at 0x08013FFC that the ARM self-update clears).

The scheduler is uC/OS-II shaped but not Micrium's code: eleven 20-byte task control blocks at
0x20007C6C (`task_tbl`, indexed by priority), a 16-bit ready mask, and event flags per task
instead of semaphores or queues. A task sets its own `period` and `count` and then calls
`os_flags_pend(0)`; `OSTimeTick` (from the 1 ms SysTick) posts flag 1 when the period runs out.
Interrupts post other flags or set them directly:

| Flag | Task | Set by | Meaning |
|---|---|---|---|
| 1 | any | OSTimeTick | the task's period ran out |
| 2 | 0 task_measure | ADC1_2_IRQHandler | a measurement window ended |
| 8 | 1 task_dsp_rx | uart_rx_isr (UART4) | 45 bytes of a 0x44 frame are in |
| 0x10 | 2 task_dsp_tx | uart_tx_isr_uart4 | the last 0x43 frame has gone |

The tasks (priority 0 runs first):

| Prio | Task | File | Period (ticks) | Job |
|---|---|---|---|---|
| 0 | task_measure | control/measure.c | 25, or each ADC window | output enable, RMS values, battery pack poll |
| 1 | task_dsp_rx | comm/dsp_link.c | 200, or each frame | DSP frames in; DSP, ARM and battery firmware updates |
| 2 | task_dsp_tx | comm/dsp_link.c | 60 | DSP frames out |
| 3 | task_modbus_rs485 | comm/modbus_slave.c | 5 | Modbus slave on RS485 |
| 4 | task_meter | comm/meter.c | 5 | meter master on RS485, register images |
| 5 | task_monitor | control/monitor.c | 20 | LEDs, SOC, temperatures, CT, power flow, EEPROM saves |
| 6 | task_schedule | control/schedule_p3.c | 500 | clock, charge and discharge decisions, energy counters |
| 7 | task_usb | usb/usb_task.c | 1 | USB host: stick or dongle |

`task_monitor` gives the others a long first wait once the settings are loaded (task 0 1000
ticks, task 1 150, task 2 20, tasks 3 and 4 1500, tasks 6 and 7 500), so the DSP link starts
first and the Modbus slave and meters about 1.5 s later.

The interrupt handlers (`stm32f10x_it.c`) do the time-critical work: TIM3 at about 19.1 kHz
starts an ADC scan every second interrupt; the ADC handler accumulates squares and products per
measurement window, closed by level changes on PD2; the USART handlers feed the queues of
`drivers/uart.c`; USART1 receives the auxiliary MCU's waveform pages; SysTick runs the scheduler
and releases the RS485 driver enables. A hard fault resets the chip; the other faults loop.

## Data

- `task_tbl` (OS_TCB[11]) at 0x20007C6C: period, count, FlagsRdy, FlagsWait per task. Several
  files reach it through small structs at fixed offsets (for example `struct N_0802D41E` in
  meter.c is task 4's period and count); they are the same fields.
- The ADC accumulators and the aux link state in `stm32f10x_it.c` (`uart.h`, `measure.h`).
- `util.c` and `stubs.c` own no data and are dead.

## Rules found in the code

- A hard fault resets the ARM (SYSRESETREQ). MemManage, BusFault and UsageFault are not enabled,
  so they escalate to the hard fault.
- Each task's period is set by the task itself at the end of each pass; another task can
  stop one by setting its period to 0 (the DSP update stops tasks 0, 2, 5 and 6 that way).
- The RS485 driver enables (PB4 Modbus, PA4 meters) are released two SysTick ticks after the
  transmitter's last byte.
- `modbus_usb_wait` is cleared by the next SysTick, so the wait before a USB CDC reply is at
  most one tick, whatever value the slave sets.

## Open questions

- What the 20-million-pass start-up delay waits for. My guess is the DSP's own start, but the
  code does not say.
- What PD2 is. It closes the ADC windows, so it is probably a mains zero-crossing or polarity
  signal; the code only shows a debounced level.
- Which signals ADC channels 0, 5 and 7 carry. Channel 0 is compared with the aux MCU's CT
  waveform, and channel 0 x channel 7 gives the EPS power on grid.
