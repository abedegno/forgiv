// clang-format off
/* at: sys_irq_disable 0x0801422C */
/* at: sys_irq_enable 0x08014230 */
/* at: sys_startup_delay 0x08014234, main 0x08014242 */
/* at: task_tbl 0x20007C6C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* main_p1.c: the program's entry from the C library start-up, and three tiny helpers.

   Entry points: main (called by Keil's __rt_entry), sys_irq_disable, sys_irq_enable (also called
   by nvic_init), sys_startup_delay.
   Data: none of its own; main fills the task table task_tbl (0x20007C6C, OS_TCB[11]) through
   OSTaskCreate and sets task 5's first period.
   Neighbours: the Keil C library start-up before it; main_p4.c and then stm32f10x_it.c after it.
   main calls board.c (board_init), the scheduler in src/rtos (OSInit, OSTaskCreate,
   OS_CPU_SysTickInit, OSStart) and nvic.c.

   The tasks main creates (address of the entry, stack words, priority; 0 runs first):

     prio 0  task_measure      300 words  control/measure.c
     prio 1  task_dsp_rx       300 words  comm/dsp_link.c
     prio 2  task_dsp_tx       300 words  comm/dsp_link.c
     prio 3  task_modbus_rs485 400 words  comm/modbus_slave.c
     prio 4  task_meter        400 words  comm/meter.c
     prio 5  task_monitor      300 words  control/monitor.c
     prio 6  task_schedule     500 words  control/schedule_p3.c
     prio 7  task_usb          800 words  usb/usb_task.c

   name: likely. ST's template order puts main.c before stm32f10x_it.c and system_stm32f10x.c;
   this is the first application object after the C library. It could instead be a sys.c. Named
   _p1 because part 4 (main_p4.c) could not be joined to it: main's literal pool ends this object
   (map/filenames.tsv). */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/sys.h"
#include "../include/a316_types.h"
#include "../include/board.h"
#include "../include/tasks.h"

/* Masks interrupts (CPSID i). */
void sys_irq_disable() {

    disableIRQinterrupts();
    return;
}

/* Unmasks interrupts (CPSIE i). nvic_init ends with this. */
void sys_irq_enable() {

    enableIRQinterrupts();
    return;
}

/* Busy-waits 20,000,000 loop passes before any peripheral is set up. At -O0 and 72 MHz that is
   of the order of a second or two (inference); the reason, perhaps to let the DSP and the supply
   settle, is not in the code. */
void sys_startup_delay() {
    uint pass_count;

    for (pass_count = 0; pass_count < 20000000; pass_count = pass_count + 1) {
    }
    return;
}

/* Boot: clock set-up (SystemInit), interrupts off, the start-up delay, board_init (GPIO,
   settings from EEPROM, ADC, UARTs), then the scheduler: OSInit, the eight tasks listed in the
   file header, nvic_init (which turns interrupts back on), the 1 ms SysTick and OSStart, which
   never returns.

   The task entries are written as numbers with bit 0 set (Thumb addresses), not as function
   names, because OSTaskCreate takes an int.
   Task 5 (task_monitor) is given its 20-tick period here; the others set their own. */
uint main() {

    SystemInit();
    sys_irq_disable();
    sys_startup_delay();
    board_init();
    OSInit();
    OSTaskCreate(0x802735b, 0, 300, TASK_MEASURE);
    OSTaskCreate(0x8022f7b, 0, 400, TASK_MODBUS);
    OSTaskCreate(0x802d41f, 0, 400, TASK_METER);
    OSTaskCreate(0x8016fdd, 0, 300, TASK_DSP_TX);
    OSTaskCreate(0x80171db, 0, 300, TASK_DSP_RX);
    OSTaskCreate(0x8026427, 0, 300, TASK_MONITOR);
    task_tbl[TASK_MONITOR].count = 0;
    task_tbl[TASK_MONITOR].period = 0x14;
    OSTaskCreate(0x8029aa7, 0, 500, TASK_SCHEDULE);
    OSTaskCreate(0x802aef5, 0, 800, TASK_USB);
    nvic_init();
    OS_CPU_SysTickInit();
    OSStart();
    return 0;
}
