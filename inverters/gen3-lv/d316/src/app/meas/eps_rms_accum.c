// clang-format off
/* The EPS output's RMS sums, run from RAM every control interrupt: at the end of each turn of the
   EPS angle (cycle_wrap from grid_angle_update) they are handed to task 1.
   Entry point: eps_rms_accum, called at the end of control_isr.
   Data: none of its own; ac_sums_isr (0xD380) and ac_sums (0xD350).
   Neighbours: ac_measure.c (task 1: eps_voltage_rms and the rest), converter_ctrl.c.
   name: descriptive (was rms_hand.c; map/filenames.tsv).
   at: eps_rms_accum 0x3F2734 (load address; it runs at 0x8B7D) */
// clang-format on
#include "F2806x_Device.h"
#include "os.h"
#include "faults.h"
#include "control.h"
#include "state.h"
#include "mppt.h"
#include "rating.h"
#include "util.h"

/* eps_cycle (0xCC80..): the EPS angle's cycle length, index and wrap flag, right after the angle
 * itself (eps_vctl_tail.theta, 0xCC7E) */

#pragma CODE_SECTION(eps_rms_accum, "ramfuncs");
/* Sums the inverter current squared, the EPS voltage squared and their product (EPS power) every
   sample. At a wrap of the EPS angle it copies the sums and the count to ac_sums, clears them and
   the wrap flag, and posts task 1's EPS event. */
void eps_rms_accum(void) {
    ac_sums_isr.eps_i_sq = ac_sums_isr.eps_i_sq + meas_fast.inv_i * meas_fast.inv_i;
    ac_sums_isr.eps_v_sq = ac_sums_isr.eps_v_sq + meas_fast.eps_v * meas_fast.eps_v;
    ac_sums_isr.eps_p = meas_fast.eps_v * meas_fast.inv_i + ac_sums_isr.eps_p;
    ac_sums_isr.n_eps++;
    if (eps_cycle.cycle_wrap == 1) {
        ac_sums.eps_i_sq = ac_sums_isr.eps_i_sq;
        ac_sums.eps_v_sq = ac_sums_isr.eps_v_sq;
        ac_sums.eps_p = ac_sums_isr.eps_p;
        ac_sums.n_eps = ac_sums_isr.n_eps;
        ac_sums_isr.eps_p = 0;
        ac_sums_isr.eps_v_sq = 0;
        ac_sums_isr.eps_i_sq = 0;
        eps_cycle.cycle_wrap = 0;
        ac_sums_isr.n_eps = 0;
        os_post_isr(TASK_AC_MEAS, EV1_RMS_SUMS);
    }
}
