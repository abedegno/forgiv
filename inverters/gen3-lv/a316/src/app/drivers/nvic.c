// clang-format off
/* at: nvic_init 0x080249D4 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* nvic.c: interrupt set-up at the end of boot.

   Entry points: nvic_init (from main). Data: none.
   Neighbours: board.c before it, stubs.c after; main_p1.c (sys_irq_enable).

   name: likely (map/filenames.tsv). */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/board.h"
#include "../../include/stdperiph.h"
#include "../../include/sys.h"

/* Moves the vector table to 0x08014000 (the application starts there; a bootloader sits below,
   inference), selects priority group 0 (no preemption between these IRQs, 4 bits of
   sub-priority) and enables, in sub-priority order: TIM3 (0), ADC1_2 (1), USART1 (4), USART2
   (5), USART3 (6), UART4 (7). Then turns interrupts on. The USB code later changes the priority
   grouping again (usb_bsp.c, finally group 1); these six keep priority bytes below 0x80, so they
   still all have preemption level 0. */
void nvic_init() {
    NVIC_InitTypeDef irq_config;

    NVIC_SetVectorTable(0x8000000, 0x14000);
    NVIC_PriorityGroupConfig(0x700);
    irq_config.NVIC_IRQChannel = 0x1d;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x0;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    irq_config.NVIC_IRQChannel = 0x12;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x1;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    irq_config.NVIC_IRQChannel = 0x25;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x4;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    irq_config.NVIC_IRQChannel = 0x26;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x5;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    irq_config.NVIC_IRQChannel = 0x27;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x6;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    irq_config.NVIC_IRQChannel = 0x34;
    irq_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    irq_config.NVIC_IRQChannelSubPriority = 0x7;
    irq_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&irq_config);
    sys_irq_enable();
    return;
}
