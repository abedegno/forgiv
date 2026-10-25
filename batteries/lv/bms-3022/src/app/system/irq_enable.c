/* at: sys_irq_enable 0x08014230 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I keep the global IRQ-enable wrapper in its original independent unit.
   name: The filename describes the observed interrupt-mask operation. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
void sys_irq_enable();

/* Enable maskable interrupts. */
void sys_irq_enable() {

    enableIRQinterrupts();
    return;
}
