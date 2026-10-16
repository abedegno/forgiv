// clang-format off
/* Sets up the EPS voltage controller and its repetitive delay line (blocks 0xC840 and 0xCC40;
   the 500 floats at 0xC868). An older comment here called it the grid PLL set-up; the PLL is set up
   by grid_ctrl_init (converter_ctrl.c), docs/names/C-notes.md finding 3.
   Entry point: eps_vctl_init, called by the state machine (op_states.c) on entry to the idle, fault
   and update states, before pv_ctrl_init, dcdc_ctrl_init and grid_ctrl_init.
   Data: none of its own.
   Neighbours: ac_ctrl.c (eps_vctl), eps_vctl_clear.c, converter_ctrl.c (eps_vctl_run).
   name: descriptive (was pll_init.c; map/filenames.tsv).
   at: eps_vctl_init 0x3EFBF2 */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "state.h"
#include "mppt.h"
#include "util.h"

/* The EPS controller's gains and limits: voltage PI kp 0.4, ki 0, +-370 (so proportional only);
   current PI kp 1.9, ki 0, +-700; current limit 370 (37.0 A); repetitive filter a 0.5, b 0.78,
   gain 0.16, +-30. The free-running angle's cycle length is 320 samples at 60 Hz (rated nominal
   6000 = 60.00 Hz) and 384 at 50 Hz, both 19.2 kHz / f. The 500-word delay line is cleared. */
void eps_vctl_init(void) {
    Uint16 i;

    eps_vctl_tail.pi_v_e = 0;
    eps_vctl_tail.pi_v_u = 0;
    eps_vctl_tail.pi_v_i = 0;
    eps_vctl_tail.pi_v_e1 = 0;
    eps_vctl_tail.pi_i_u = 0;
    eps_vctl_tail.pi_i_i = 0;
    eps_vctl_tail.pi_i_e1 = 0;
    eps_vctl_tail.duty = 0;
    eps_vctl_tail.pi_v_kp = 0.4f;
    eps_vctl_tail.i_ref = 0;
    eps_vctl_blk.v_peak = 0;
    eps_vctl_tail.mod_gain = 0;
    eps_vctl_tail.theta = 0;
    eps_vctl_tail.pi_v_ki = 0;
    eps_vctl_tail.pi_v_kd = 0;
    eps_vctl_blk.rep_b = 0.779999971f;
    eps_vctl_blk.rep_gain = 0.159999996f;
    eps_vctl_tail.pi_i_kp = 1.9f;
    eps_vctl_tail.pi_i_ki = 0;
    eps_vctl_blk.rep_a = 0.5f;
    eps_vctl_tail.pi_i_kd = 0;
    eps_vctl_blk.rep_max = 30.0f;
    eps_vctl_tail.pi_v_umax = 370.0f;
    eps_vctl_blk.rep_min = -30.0f;
    eps_vctl_tail.pi_v_umin = -370.0f;
    eps_vctl_blk.i_limit = 370.0f;
    eps_vctl_tail.pi_i_umax = 700.0f;
    eps_vctl_blk.ramp_done = 0;
    eps_vctl_tail.pi_i_umin = -700.0f;
    if (rating_params.grid_f_nominal == 6000.0f)
        eps_cycle.cycle_len = 0x140;
    else
        eps_cycle.cycle_len = 0x180;
    eps_cycle.cycle_idx = 0;
    for (i = 0; i < 500; i++)
        eps_rep_buf[i] = 0;
}
