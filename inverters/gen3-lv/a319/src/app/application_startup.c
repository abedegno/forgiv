// clang-format off
/* at: sys_irq_disable 0x0801422C */
/* at: sys_irq_enable 0x08014230 */
/* at: sys_startup_delay 0x08014234, main 0x08014242 */
/* at: task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* application_startup.c: application entry, interrupt masking and the initial delay.
   main is called by Keil's runtime. It initializes the board, creates eight RTOS tasks,
   enables interrupts and starts the scheduler. The task table belongs to the RTOS;
   peripheral setup is in board.c and the entry prototypes are shared subsystem headers.
   Tasks by priority: measure (0), DSP receive (1), DSP transmit (2), Modbus (3),
   meter (4), monitor (5), schedule (6), USB (7). Lower values run first.
   match: keep drivers/delay.c separate; main's native literal pool precedes that helper.
   name: descriptive; the original vendor filename is not known. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/sys.h"
#include "../include/application_types.h"
#include "../include/board.h"
#include "../include/tasks.h"
#include "../include/measure.h"
#include "../include/modbus.h"
#include "../include/meter.h"
#include "../include/dsp_link.h"
#include "../include/schedule.h"
#include "../include/usb.h"

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

   OSTaskCreate's recovered interface takes an int; the casts retain each function's
   Thumb entry address.
   Task 5 (task_monitor) is given its 20-tick period here; the others set their own. */
uint main() {

    SystemInit();
    sys_irq_disable();
    sys_startup_delay();
    board_init();
    OSInit();
    OSTaskCreate((int)task_measure, 0, 300, TASK_MEASURE);
    OSTaskCreate((int)task_modbus_rs485, 0, 400, TASK_MODBUS);
    OSTaskCreate((int)task_meter, 0, 400, TASK_METER);
    OSTaskCreate((int)task_dsp_tx, 0, 300, TASK_DSP_TX);
    OSTaskCreate((int)task_dsp_rx, 0, 300, TASK_DSP_RX);
    OSTaskCreate((int)task_monitor, 0, 300, TASK_MONITOR);
    task_tbl[TASK_MONITOR].count = 0;
    task_tbl[TASK_MONITOR].period = 0x14;
    OSTaskCreate((int)task_schedule, 0, 500, TASK_SCHEDULE);
    OSTaskCreate((int)task_usb, 0, 800, TASK_USB);
    nvic_init();
    OS_CPU_SysTickInit();
    OSStart();
    return 0;
}
