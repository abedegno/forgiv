/* sys.h: system start-up, clocks, interrupts and small helpers (main_p1.c, main_p4.c, util.c,
   cmsis/system_stm32f10x.c, board.c).
   name: descriptive (readability step 1). */
#ifndef SYS_H
#define SYS_H

#include "ghidra_types.h"

/* data */
extern uint SystemCoreClock;
extern uchar AHBPrescTable[];
extern ushort sys_mode; /* DSP page 0 (IR49): 1 on grid, 0 off grid (inference from the public
                            list) */

/* functions */
void SystemCoreClockUpdate();
void SystemInit();
int clamp(int lower, int value, int upper);
int counter_wrap_inc(int count, int min_count, int max_count);
uint div_u16_safe(uint dividend, uint divisor);
uchar hex_nibble_to_ascii(int nibble);
uint main();
int max3(int first, int second, int third);
int median3(int first, int second, int third);
int min3(int first, int second, int third);
void sys_delay_loops(uint loops);
void sys_irq_disable();
void sys_irq_enable();
void sys_runtime_vars_init();
void sys_startup_delay();

#endif
