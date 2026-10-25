// clang-format off
/* The PV input start-up ramp (phase blocks 0xD240/0xD296) and the MPPT dispatch.
   at: pv_mppt_step 0x3EF598
   It uses 0xD136/0xD137, which src/app/power/mppt.c defines; it is probably the end of the same
   original file, kept apart here because mppt.c is range B's.

   Job: once per slow average (one grid period; task3_measure on EV3_SLOW_AVG) ramp the PV boost
   current limits up, take the open-circuit voltages, and call mppt_pv1/mppt_pv2 by PV mode.
   Entry point: pv_mppt_step (meas_task.c). Data: none of its own; it writes pv_phase[].i_limit,
   the mppt block (meas.h) and state_flags bits 12/13.
   Neighbours: mppt.c, dc_measure.c (PV mode, voltages), op_states.c (op_state, state_flags).
   Units: voltages 0.1 V, i_limit in the units of pv_phase[].i_max (1500 in every preset).
   name: descriptive (map/filenames.tsv: src/mppt_c.c). */
// clang-format on
#include "F2806x_Device.h"
#include "settings.h"
#include "faults.h"
#include "meas.h"
#include "control.h"
#include "state.h"
#include "mppt.h"
#include "util.h"

/* The second phase block's f02 (0xD298) under a name of its own: the ramp and the lower clamp
   go through it (DP), the compare and the upper clamp through pv_phase[1] (base register). */

/* the ramp step per call: 50 while pv_detect_done is 1, else 5 */
#define PV_RAMP_STEP                                                                               \
    if (pv_detect_done != 1)                                                                       \
        step = 5.0f;                                                                               \
    else                                                                                           \
        step = 50.0f
/* In states 5 and 7 (or mppt_flags bit 14):
   - running (mppt_flags bit 6 clear): each input's current limit ramps up by the step while it
     reads 110.0 V or more, to i_max; under 110 V it is 0;
   - a start or rescan (bit 6 set by start_grid_pv, start_eps_pv or meas_task.c): clear bits 4
     and 5 (the MPPTs start again), wait 25 calls, then set both
     references to the measured voltages and the sweep floors to 0.83 x those, set state_flags bit 13
     (state 5) or 12 (state 7) and clear bit 6;
   - keep the highest voltage seen as Voc, run the MPPT by PV mode (1 PV1, 2 PV2, 4 and 5 both), and
     ask for a rescan after 51 calls with both inputs under 120.0 V.
   In states 0, 3 and 8 the references, Voc and the sweep floors follow the measured voltages. */
void pv_mppt_step(void) {
    float step;

    if (op_state == OP_GRID_PV || op_state == OP_EPS_PV || mppt_flags.bit.b14) {
        if (!mppt_flags.bit.b6) {
            if (dc_meas.pv1_v < 1100.0f) {
                pv_phase[0].i_limit = 0;
            } else {
                PV_RAMP_STEP;
                pv_phase[0].i_limit = step + pv_phase[0].i_limit;
                {
                    float t_ = pv_phase[0].i_limit;
                    pv_phase[0].i_limit =
                        t_ > pv_phase[0].i_max ? pv_phase[0].i_max : __fmax(t_, 0.0f);
                }
            }
            if (dc_meas.pv2_v >= 1100.0f) {
                PV_RAMP_STEP;
                pv2_i_limit_alias = step + pv2_i_limit_alias;
                if (pv_phase[1].i_limit > pv_phase[1].i_max)
                    pv_phase[1].i_limit = pv_phase[1].i_max;
                else
                    pv2_i_limit_alias = __fmax(pv2_i_limit_alias, 0.0f);
            } else {
                pv_phase[1].i_limit = 0;
            }
        } else {
            mppt_flags.all &= 0xFFCF;
            if (voc_wait_count >= 25) {
                mppt.pv1_ref = dc_meas.pv1_v;
                mppt.pv2_ref = dc_meas.pv2_v;
                mppt.pv1_sweep_min = 0.83f * dc_meas.pv1_v;
                mppt.pv2_sweep_min = 0.83f * dc_meas.pv2_v;
                voc_wait_count = 0;
                if (op_state == OP_GRID_PV) {
                    DINT;
                    state_flags.all |= 0x2000;
                    EINT;
                }
                if (op_state == OP_EPS_PV) {
                    DINT;
                    state_flags.all |= 0x1000;
                    EINT;
                }
                mppt_flags.all &= 0xFFBF;
            } else {
                voc_wait_count++;
            }
        }
        mppt.pv1_voc = __fmax(mppt.pv1_voc, dc_meas.pv1_v);
        mppt.pv2_voc = __fmax(mppt.pv2_voc, dc_meas.pv2_v);
        if (dsp_settings.pv_input_mode == 1) {
            mppt_pv1();
        } else if (dsp_settings.pv_input_mode == 2) {
            mppt_pv2();
        } else if (dsp_settings.pv_input_mode == 5 || dsp_settings.pv_input_mode == 4) {
            mppt_pv1();
            mppt_pv2();
        }
        if (dc_meas.pv1_v >= 1200.0f || dc_meas.pv2_v >= 1200.0f) {
            pv_both_low_count = 0;
        } else {
            pv_both_low_count++;
            if (pv_both_low_count > 50) {
                mppt.rescan = 1;
                pv_both_low_count = 0;
            }
        }
    } else if (op_state == OP_IDLE || op_state == OP_FAULT || op_state == OP_EPS_BATT) {
        mppt.pv1_voc = dc_meas.pv1_v;
        mppt.pv2_voc = dc_meas.pv2_v;
        mppt.pv1_ref = mppt.pv1_voc;
        mppt.pv2_ref = mppt.pv2_voc;
        mppt.pv1_sweep_min = 0.83f * mppt.pv1_voc;
        mppt.pv2_sweep_min = 0.83f * mppt.pv2_voc;
    }
}
