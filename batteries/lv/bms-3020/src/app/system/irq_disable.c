/* at: sys_irq_disable 0x0801422C */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I keep the global IRQ-disable wrapper in its original independent unit.
   name: The filename describes the observed interrupt-mask operation. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
void sys_irq_disable();

/* Disable maskable interrupts. */
void sys_irq_disable() {

    disableIRQinterrupts();
    return;
}
