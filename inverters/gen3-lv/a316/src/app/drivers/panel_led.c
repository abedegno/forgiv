// clang-format off
/* at: gpio_pe2_init 0x08024AA8, gpio_pe2_set 0x08024ACE, panel_led_pc7_set 0x08024AE8, panel_led_pc6_set 0x08024B02, panel_led_pv_set 0x08024B1C, panel_led_pc9_set 0x08024B3A, panel_led_batt_discharge_set 0x08024B58, panel_led_batt_charge_set 0x08024B76 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* panel_led.c: the front panel indicator outputs.

   Entry points: panel_led_*_set (from status_outputs_update in schedule_p3.c). Dead:
   gpio_pe2_init, gpio_pe2_set.
   Data: none.
   Neighbours: stubs.c before, util.c after; board.c (panel_gpio_init sets the pins up).

   What each indicator shows comes from its caller (status_outputs_update):
     PA8   PV present (PV1 or PV2 above 100.0 V, inference from 0.1 V units)
     PB14  battery charging (batt_power_calc >= 0), PB15 discharging
     PC6, PC7  charge status 1-4, or otherwise; both off at status 7
     PC9   load above 50 W (g_200002B8 > 500, 0.1 W units, inference)

   name: descriptive (map/filenames.tsv). */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/board.h"
#include "../../include/stdperiph.h"

/* Dead. PE2 as a push-pull output. */
void gpio_pe2_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x40, 1);
    gpio_config.GPIO_Pin = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOE_BASE, &gpio_config);
    return;
}

/* Dead. Drives PE2. */
void gpio_pe2_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOE_BASE, 4);
    } else {
        GPIO_SetBits(GPIOE_BASE, 4);
    }
    return;
}

/* Drives PC7 (on when the charge status is not 1-4 and not 7). */
void panel_led_pc7_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOC_BASE, 0x80);
    } else {
        GPIO_SetBits(GPIOC_BASE, 0x80);
    }
    return;
}

/* Drives PC6 (on while the charge status is 1-4). */
void panel_led_pc6_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOC_BASE, 0x40);
    } else {
        GPIO_SetBits(GPIOC_BASE, 0x40);
    }
    return;
}

/* Drives PA8 (PV present). */
void panel_led_pv_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOA_BASE, 0x100);
    } else {
        GPIO_SetBits(GPIOA_BASE, 0x100);
    }
    return;
}

/* Drives PC9 (load present). gpio_pc9_set in board.c drives the same pin. */
void panel_led_pc9_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOC_BASE, 0x200);
    } else {
        GPIO_SetBits(GPIOC_BASE, 0x200);
    }
    return;
}

/* Drives PB15 (battery discharging). */
void panel_led_batt_discharge_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x8000);
    } else {
        GPIO_SetBits(GPIOB_BASE, 0x8000);
    }
    return;
}

/* Drives PB14 (battery charging). */
void panel_led_batt_charge_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x4000);
    } else {
        GPIO_SetBits(GPIOB_BASE, 0x4000);
    }
    return;
}
