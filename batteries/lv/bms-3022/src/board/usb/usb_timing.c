/* at: USB_OTG_BSP_DriveVBUS 0x080159D8, BSP_SetTime 0x080159DA, BSP_Delay 0x08015A4A, USB_OTG_BSP_mDelay 0x08015A6A, USB_OTG_BSP_ConfigVBUS 0x08015A6E, USB_OTG_BSP_uDelay 0x08015A72, USB_OTG_BSP_TimerIRQ 0x08015A76 */
/* at: BSP_delay 0x2000019C */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow USB timing support here; I keep the delay and tick operations in their matched order.
   Entry examples: USB_OTG_BSP_DriveVBUS, BSP_SetTime, BSP_Delay.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
/* ST USB host delay timer, adapted from the matched A316 source. */
#include "bms_usb_bsp_types.h"
/* Does nothing: VBUS is not switched by the ARM on this board (inference). */
void USB_OTG_BSP_DriveVBUS() {
    return;
}

/* Starts TIM2 for one delay unit: 1 us (period 12 at 72/6 MHz) or 1 ms (period 12000). */
void BSP_SetTime(int unit) {
    TIM_TimeBaseInitTypeDef timer_config;

    TIM_Cmd(TIM2_BASE, 0);
    TIM_ITConfig(TIM2_BASE, 1, 0);
    if (unit == 2) {
        timer_config.TIM_Period = 0xb;
    } else {
        if (unit == 1) {
            timer_config.TIM_Period = 0x2edf;
        }
    }
    timer_config.TIM_Prescaler = 5;
    timer_config.TIM_ClockDivision = 0;
    timer_config.TIM_CounterMode = 0;
    TIM_TimeBaseInit(TIM2_BASE, &timer_config);
    TIM_ClearITPendingBit(TIM2_BASE, 1);
    TIM_ARRPreloadConfig(TIM2_BASE, 1);
    TIM_ITConfig(TIM2_BASE, 1, 1);
    TIM_Cmd(TIM2_BASE, 1);
}

/* Busy-waits n delay units of TIM2 (TIM2_IRQHandler counts BSP_delay down). */
void BSP_Delay(int duration, int unit) {

    BSP_delay = duration;
    BSP_SetTime(unit);
    do {
    } while (BSP_delay != 0);
    TIM_Cmd(TIM2_BASE, 0);
    return;
}

/* Waits n milliseconds. */
void USB_OTG_BSP_mDelay(int ms) {

    BSP_Delay(ms, 1);
    return;
}

/* Waits 200 ms (no VBUS switch to configure). */
void USB_OTG_BSP_ConfigVBUS(uint unused_core) {

    USB_OTG_BSP_mDelay(200);
    return;
}

/* Waits n microseconds. */
void USB_OTG_BSP_uDelay(int us) {

    BSP_Delay(us, 2);
    return;
}

/* TIM2 update: counts BSP_delay down and stops the timer at 0. */
void USB_OTG_BSP_TimerIRQ() {
    if (TIM_GetITStatus(TIM2_BASE, 1) != 0) {
        TIM_ClearITPendingBit(TIM2_BASE, 1);
        if (!(BSP_delay == 0)) {
            BSP_delay = BSP_delay - 1;
        } else {
            TIM_Cmd(TIM2_BASE, 0);
        }
    }
    return;
}
