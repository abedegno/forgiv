#include "boot.h"
#include "util.h"
/* boot_update.c: the boot loader's entry for an update, at the fixed address 0x3F5000. Both ways
   into an update come here: boot_main when there is no valid application, and state4_update
   (op_states.c) in OP_UPDATE, with interrupts off, after the ARM's run command 4.

   Entry point: boot_update. Data: none.
   Neighbours: boot_csm.c (boot_copy_run), boot_main.asm, app/state/op_states.c.
   name: descriptive (map/filenames.tsv), name kept.
   at: boot_update 0x3F5000 */

/* Runs the update routine from RAM (boot_copy_run); it does not return. */
void boot_update(void)
{
    boot_copy_run();
}
