// clang-format off
/* Clears the EPS voltage controller's state, run from RAM. (An older comment here called it the
   grid PLL; the PLL is pll.c's block at 0xC100, docs/names/C-notes.md finding 3.)
   Entry point: eps_vctl_clear, called by control_isr every sample while the bridge is off or in a
   grid state, so the EPS loop always starts from zero.
   Data: none of its own; it writes eps_vctl_blk (0xC840) and eps_vctl_tail (0xCC40).
   Neighbours: eps_vctl_init.c (the full set-up), ac_ctrl.c (eps_vctl).
   name: descriptive (was pll_clear.c; map/filenames.tsv).
   at: eps_vctl_clear 0x3F293C (load address; it runs at 0x8CE8) */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

#pragma CODE_SECTION(eps_vctl_clear, "ramfuncs");
/* Zeroes the two PI states (error, integrator, output), the duty, the current reference, the
   amplitude ramp v_peak and ramp_done. Gains and limits are left as eps_vctl_init set them. */
void eps_vctl_clear(void) {
    eps_vctl_tail.pi_v_e = 0;
    eps_vctl_tail.pi_v_u = 0;
    eps_vctl_tail.pi_v_i = 0;
    eps_vctl_tail.pi_v_e1 = 0;
    eps_vctl_tail.pi_i_u = 0;
    eps_vctl_tail.pi_i_i = 0;
    eps_vctl_tail.pi_i_e1 = 0;
    eps_vctl_tail.duty = 0;
    eps_vctl_blk.v_peak = 0;
    eps_vctl_blk.ramp_done = 0;
    eps_vctl_tail.i_ref = 0;
}
