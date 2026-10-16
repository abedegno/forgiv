// clang-format off
/* at: USB_OTG_BSP_TimeInit 0x08029C84, USB_OTG_BSP_Init 0x08029CB2, USB_OTG_BSP_EnableInterrupt 0x08029CCA, USB_OTG_BSP_DriveVBUS 0x08029CF6 */
/* at: BSP_SetTime 0x08029CF8, BSP_Delay 0x08029D66, USB_OTG_BSP_mDelay 0x08029D8C, USB_OTG_BSP_ConfigVBUS 0x08029D9A, USB_OTG_BSP_uDelay 0x08029DA6, USB_OTG_BSP_TimerIRQ 0x08029DB4 */
/* at: BSP_delay 0x200003D8 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* usb_bsp.c: the board support layer of ST's USB OTG host library (from ST's USB host example):
   clocks, interrupts and the microsecond and millisecond delays on TIM2.

   Entry points: the USB_OTG_BSP_* functions the library calls; USB_OTG_BSP_TimerIRQ from
   TIM2_IRQHandler. Data: BSP_delay (the delay counter).
   Neighbours: schedule_p3.c before, usbh_usr.c after; src/lib/usb_otg and src/lib/usb_host.

   name: original. ST's usb_bsp.c (map/filenames.tsv). Two parts joined, the joined file compiles
   to the same bytes. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/stdperiph.h"
#include "../../include/usb.h"

/* Enables TIM2's interrupt (priority group 2, preemption 1, sub 1) and its clock. Changes the
   NVIC priority grouping set by nvic_init. */
void USB_OTG_BSP_TimeInit() {
    NVIC_InitTypeDef nvic_config;

    NVIC_PriorityGroupConfig(0x500);
    nvic_config.NVIC_IRQChannel = 0x1c;
    nvic_config.NVIC_IRQChannelPreemptionPriority = 0x1;
    nvic_config.NVIC_IRQChannelSubPriority = 0x1;
    nvic_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&nvic_config);
    RCC_APB1PeriphClockCmd(1, 1);
    return;
}

/* USB OTG FS clock from the PLL VCO / 3 (48 MHz), the OTG FS AHB clock on, then the delay timer. */
void USB_OTG_BSP_Init(uint unused_core) {

    RCC_OTGFSCLKConfig(0);
    RCC_AHBPeriphClockCmd(0x1000, 1);
    USB_OTG_BSP_TimeInit();
    return;
}

/* Enables the OTG FS interrupt (priority group 1, preemption 1, sub 3). */
void USB_OTG_BSP_EnableInterrupt(uint unused_core) {
    NVIC_InitTypeDef nvic_config;

    NVIC_PriorityGroupConfig(0x600);
    nvic_config.NVIC_IRQChannel = 0x43;
    nvic_config.NVIC_IRQChannelPreemptionPriority = 0x1;
    nvic_config.NVIC_IRQChannelSubPriority = 0x3;
    nvic_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&nvic_config);
    return;
}

/* Does nothing: VBUS is not switched by the ARM on this board (inference). */
void USB_OTG_BSP_DriveVBUS() { return; }

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
    int unused_value;

    if (TIM_GetITStatus(TIM2_BASE, 1) != 0) {
        TIM_ClearITPendingBit(TIM2_BASE, 1);
        if (!(BSP_delay == 0)) {
            BSP_delay = BSP_delay + -1;
        } else {
            TIM_Cmd(TIM2_BASE, 0);
        }
    }
    return;
}
