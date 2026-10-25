/* USB interrupt priority and enable configuration. */
/* at: usb_board_initialize_interrupts 0x080159B0 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow USB interrupt priority and enable setup here; controller interrupt handlers stay in their own unit.
   Entry examples: usb_board_initialize_interrupts.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "../../include/stdperiph_types.h"
void NVIC_PriorityGroupConfig(unsigned int);
void NVIC_Init(NVIC_InitTypeDef *);
/* Configures USB OTG interrupt channel67 and its priority. */
void usb_board_initialize_interrupts(void) {
    NVIC_InitTypeDef irq;
    NVIC_PriorityGroupConfig(0x500);
    irq.NVIC_IRQChannel = 67;
    irq.NVIC_IRQChannelPreemptionPriority = 1;
    irq.NVIC_IRQChannelSubPriority = 3;
    irq.NVIC_IRQChannelCmd = 1;
    NVIC_Init(&irq);
}
