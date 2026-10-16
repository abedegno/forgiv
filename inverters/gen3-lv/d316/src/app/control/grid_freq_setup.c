// clang-format off
/* Sets the per-cycle constants of the control interrupt for a 50 Hz or 60 Hz grid.
   Entry point: grid_freq_setup(mode), called by grid_freq_detect (ac_measure.c, task 1) once the
   frequency has been seen in the 46..54 Hz or 56..64 Hz band for 25 periods.
   Data: none of its own; it writes the converter module's globals through the ctrl_globals view:
   grid_cycle_samples and the two relay sync points relay_sync_a/b.
   Neighbours: converter_ctrl.c (relay_drive uses the sync points, dc_acc_step the cycle length).
   name: descriptive (was ctrl_mode.c; map/filenames.tsv).
   at: grid_freq_setup 0x3EFBD7
   match: converter control code that reaches the control state (0xC000 ..) as the control
   module's own variables (one MOVW DP per page): written against the page structs of control.h. */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

/* mode 0 (50 Hz): 384 samples per cycle (19.2 kHz / 50), relay close points at samples 24 and
   216, half a cycle apart. mode 1 (60 Hz): 320 samples, close points at 320 and 160. The stores are
   made with interrupts off, so the interrupt never sees a mixed set. */
void grid_freq_setup(Uint16 mode) {
    switch (mode) {
    case 0:
        DINT;
        ctrl_globals.grid_cycle_samples = 0x0180;
        ctrl_globals.relay_sync_a = 0x0018;
        ctrl_globals.relay_sync_b = 0x00D8;
        EINT;
        break;
    case 1:
        DINT;
        ctrl_globals.grid_cycle_samples = 0x0140;
        ctrl_globals.relay_sync_a = 0x0140;
        ctrl_globals.relay_sync_b = 0x00A0;
        EINT;
        break;
    }
}
