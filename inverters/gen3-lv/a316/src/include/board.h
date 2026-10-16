/* board.h: board I/O: LEDs, buzzer, panel indicators, GPIO and timer set-up (board.c,
   drivers/panel_led.c, drivers/nvic.c, stubs.c, monitor.c's buzzer and LED functions).
   name: descriptive (readability step 1). */
#ifndef BOARD_H
#define BOARD_H

#include "ghidra_types.h"

/* data */
extern uchar board_init_done; /* set by board_init; then also the meter fault flag (1 = no meter
                                  answer) */
extern uchar buzzer_div;
extern uchar g_2000018C;
extern uchar g_2000019E;
extern uchar hr32_value;
extern uchar g_200001AF;
extern uchar g_200001B2;
extern uchar g_200001D6;
extern uchar g_20000261;
extern uchar g_20000262;
extern uchar g_20000263;
extern uchar g_20000264;
extern uchar g_20000265;
extern uchar g_20000267[];
extern uchar fault_recover_window; /* set after fault 25, 30 or 31 lasts 3 s (sys_tick_1s) */
extern uchar g_2000028A;
extern uchar g_2000028B;
extern uchar g_20000297;
extern uchar led_status_tick; /* blink counter of led_status_update */

/* functions */
void __aeabi_memclr(int, int);
void adc_dma_init();
void board_init();
void buzzer_gpio_init();
void buzzer_off();
void buzzer_on();
void buzzer_pin_set(int level);
void gpio_pc3_init();
void gpio_pc3_set(int level);
void gpio_pc9_init();
void gpio_pc9_set(int level);
void gpio_pd2_init();
void gpio_pe2_init();
void gpio_pe2_set(int level);
void led_fault_gpio_init();
void led_fault_set(int level);
void led_gpio_init();
void led_set(int led, int level);
void led_status_update();
void nvic_init();
void panel_gpio_init();
void panel_led_batt_charge_set(int level);
void panel_led_batt_discharge_set(int level);
void panel_led_pc6_set(int level);
void panel_led_pc7_set(int level);
void panel_led_pc9_set(int level);
void panel_led_pv_set(int level);
void stub_noop_1();
void stub_noop_2();
void stub_noop_3();
void stub_noop_4();
void tim3_init();
void tim4_init();

#endif
