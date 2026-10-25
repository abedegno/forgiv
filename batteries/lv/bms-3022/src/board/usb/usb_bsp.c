/* at: USB_OTG_BSP_TimeInit 0x0801596C, USB_OTG_BSP_Init 0x0801599A */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow board support for the USB controller here; the host stack uses these hardware hooks.
   Entry examples: USB_OTG_BSP_TimeInit, USB_OTG_BSP_Init.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
/* ST USB host board support, adapted from the matched A316 source. */
#include "bms_usb_bsp_types.h"
/* Enables TIM2's interrupt (priority group 3, preemption 1, sub 1) and its clock. Changes the
   NVIC priority grouping set by nvic_init. */
void USB_OTG_BSP_TimeInit() {
    NVIC_InitTypeDef nvic_config;

    NVIC_PriorityGroupConfig(0x400);
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
