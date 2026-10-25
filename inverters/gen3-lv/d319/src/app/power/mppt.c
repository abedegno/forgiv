// clang-format off
/* The MPPT of the two PV inputs (perturb and observe on the 0xD1C0 block) and its start-up.
   at: mppt_po_pv1 0x3EF02E; mppt_po_pv2 0x3EF173; mppt_pv1 0x3EF2B8; mppt_pv2 0x3EF42D
   This file defines 0xD12C..0xD13D (cinit records #354..#371). pv_mppt_step (agent C's range)
   uses 0xD136/0xD137 and is probably part of the same original file.
   Any change to this file can move its variables: rerun
       python3 tools/ebss_solve.py src/app/power/mppt.c

   Job: per PV input, a start-up sweep and then perturb and observe on the averaged PV power. The
   reference (mppt.pv1_ref/pv2_ref, 0.1 V) goes to the boost converters' interrupt code
   (pv1_ref_isr/pv2_ref_isr) and is held to 120 V .. pv_ref_max (Voc + 2 V, dc_measure.c).
   Entry points: mppt_pv1 and mppt_pv2, called by pv_mppt_step (mppt_step.c) once per slow average
   (one grid period, EV3_SLOW_AVG) by PV mode: 1 PV1, 2 PV2, 4 parallel (PV2 follows PV1), 5 both.
   Data it owns: the counters 0xD12C..0xD13D below; it works on mppt (0xD1C0, meas.h), whose
   v_step (5.0 V) and p_hyst (60000) dc_measure.c sets.
   Neighbours: mppt_step.c (dispatch and Voc), dc_measure.c (pv powers, pv_ref_max, the rescan),
   power_limits.c (aging mode 1 fixes both references at 250 V).
   Units: voltages 0.1 V; the power sums are 50 samples of dc_meas.pv1_p/pv2_p, so 0.02 x sum is the
   mean in the units of dc_meas (pv_power_rated 7500000 is the 5 kW preset's PV rating).
   name: descriptive (map/filenames.tsv: src/mppt_b.c). */
// clang-format on
#define OWN_pv_phase /* match: the first word of the phase block 0xD240 as a float here; the header has the two phase blocks */
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "meas.h"
#include "control.h"
#include "state.h"
#include "mppt.h"

extern float
    pv_phase; /* match: the first word of the phase block 0xD240 as a float, see OWN_pv_phase */

#include <math.h>

// clang-format off
/* .ebss begin */
unsigned int pv1_po_prev = 0;
unsigned int pv1_low_power_count = 0;  /* sums under the low-power mean, 161 = rescan */
unsigned int pv2_hold_count = 0;
unsigned int pv1_hold_count = 0;
unsigned int pv2_low_power_count = 0;
unsigned int pv2_ref_high_count = 0;  /* reference over PV2 + 20.0 V */
unsigned int voc_wait_count = 0;  /* 0xD136: pv_mppt_step waits 25 calls for Voc */
unsigned int pv2_ref_low_count = 0;
unsigned int pv2_po_prev = 0;
unsigned int pv2_po_state = 0;
unsigned int pv1_plim_step_count = 0;  /* calls between 0.1 V steps while power limited */
unsigned int pv1_po_state = 0;  /* perturb and observe state 0..3 */
unsigned int pv2_start_count = 0;
unsigned int pv2_plim_step_count = 0;
unsigned int pv1_start_count = 0;
unsigned int pv_both_low_count = 0;  /* 0xD137: pv_mppt_step, both inputs under 120 V */
unsigned int pv1_ref_low_count = 0;
unsigned int pv1_ref_high_count = 0;  /* reference over PV1 + 30.0 V */
/* .ebss end */
// clang-format on

/* One perturb and observe step for PV1, after each 50-sample power sum. The sum against the last
   one (+- p_hyst) picks a state: rose -> 0 (or 1 after a fall), fell -> 2, or 3 if it fell by more
   than 90000 or was already falling, the same for 5 calls -> 0, for over 50 -> 3. Then:
   state 3: a full step (v_step, 5.0 V) back the other way; state 0 just entered: a full step the
   same way as the last; state 0 again: half a step back the other way (dither at the peak); states
   1 and 2 hold. The reference stays within 120 V .. pv1_ref_max. */
void mppt_po_pv1(void) {
    unsigned int f;
    if (mppt.pv1_p_sum > mppt.pv1_p_prev + mppt.p_hyst) {
        if (pv1_po_state == 3 || pv1_po_state == 2) {
            pv1_po_prev = pv1_po_state;
            pv1_po_state = 1;
        } else {
            pv1_po_prev = pv1_po_state;
            pv1_po_state = 0;
            mppt.pv1_p_prev = mppt.pv1_p_sum;
        }
        pv1_hold_count = 0;
    } else if (mppt.pv1_p_sum + mppt.p_hyst < mppt.pv1_p_prev) {
        f = (mppt.pv1_p_sum + 90000.0f < mppt.pv1_p_prev);
        if (f || pv1_po_state == 3 || pv1_po_state == 2) {
            pv1_po_prev = pv1_po_state;
            pv1_po_state = 3;
        } else if (pv1_po_state == 0 || pv1_po_state == 1) {
            pv1_po_prev = pv1_po_state;
            pv1_po_state = 2;
        }
        if (pv1_po_state == 3)
            mppt.pv1_p_prev = mppt.pv1_p_sum;
        pv1_hold_count = 0;
    } else if (++pv1_hold_count == 5) {
        pv1_po_prev = pv1_po_state;
        pv1_po_state = 0;
        mppt.pv1_p_prev = mppt.pv1_p_sum;
    } else if (pv1_hold_count > 50) {
        pv1_hold_count = 0;
        pv1_po_prev = pv1_po_state;
        pv1_po_state = 3;
        mppt.pv1_p_prev = mppt.pv1_p_sum;
    }
    if (pv1_po_state == 3) {
        if (mppt.pv1_ref <= mppt.pv1_ref_prev) {
            mppt.pv1_ref_prev = mppt.pv1_ref;
            if (mppt.pv1_ref < pv1_ref_max - mppt.v_step)
                mppt.pv1_ref = mppt.pv1_ref + mppt.v_step;
            else
                mppt.pv1_ref = pv1_ref_max;
        } else {
            mppt.pv1_ref_prev = mppt.pv1_ref;
            if (mppt.pv1_ref > mppt.v_step + 1200.0f)
                mppt.pv1_ref = mppt.pv1_ref - mppt.v_step;
            else
                mppt.pv1_ref = mppt.pv1_ref + 0.5f * mppt.v_step;
        }
    } else if (pv1_po_state == 0) {
        if (pv1_po_state == pv1_po_prev) {
            if (mppt.pv1_ref < mppt.pv1_ref_prev) {
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (mppt.pv1_ref < pv1_ref_max - 0.5f * mppt.v_step)
                    mppt.pv1_ref = 0.5f * mppt.v_step + mppt.pv1_ref;
                else
                    mppt.pv1_ref = pv1_ref_max;
            } else {
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (mppt.pv1_ref > 0.5f * mppt.v_step + 1200.0f)
                    mppt.pv1_ref = mppt.pv1_ref - 0.5f * mppt.v_step;
                else
                    mppt.pv1_ref = mppt.pv1_ref + 0.5f * mppt.v_step;
            }
        } else {
            if (mppt.pv1_ref <= mppt.pv1_ref_prev) {
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (mppt.pv1_ref > mppt.v_step + 1200.0f)
                    mppt.pv1_ref = mppt.pv1_ref - mppt.v_step;
                else
                    mppt.pv1_ref = mppt.pv1_ref + 0.5f * mppt.v_step;
            } else {
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (mppt.pv1_ref < pv1_ref_max - mppt.v_step)
                    mppt.pv1_ref = mppt.pv1_ref + mppt.v_step;
                else
                    mppt.pv1_ref = pv1_ref_max;
            }
        }
    }
}

/* mppt_po_pv1 for PV2, the same code on the PV2 words. */
void mppt_po_pv2(void) {
    unsigned int f;
    if (mppt.pv2_p_sum > mppt.pv2_p_prev + mppt.p_hyst) {
        if (pv2_po_state == 3 || pv2_po_state == 2) {
            pv2_po_prev = pv2_po_state;
            pv2_po_state = 1;
        } else {
            pv2_po_prev = pv2_po_state;
            pv2_po_state = 0;
            mppt.pv2_p_prev = mppt.pv2_p_sum;
        }
        pv2_hold_count = 0;
    } else if (mppt.pv2_p_sum + mppt.p_hyst < mppt.pv2_p_prev) {
        f = (mppt.pv2_p_sum + 90000.0f < mppt.pv2_p_prev);
        if (f || pv2_po_state == 3 || pv2_po_state == 2) {
            pv2_po_prev = pv2_po_state;
            pv2_po_state = 3;
        } else if (pv2_po_state == 0 || pv2_po_state == 1) {
            pv2_po_prev = pv2_po_state;
            pv2_po_state = 2;
        }
        if (pv2_po_state == 3)
            mppt.pv2_p_prev = mppt.pv2_p_sum;
        pv2_hold_count = 0;
    } else if (++pv2_hold_count == 5) {
        pv2_po_prev = pv2_po_state;
        pv2_po_state = 0;
        mppt.pv2_p_prev = mppt.pv2_p_sum;
    } else if (pv2_hold_count > 50) {
        pv2_hold_count = 0;
        pv2_po_prev = pv2_po_state;
        pv2_po_state = 3;
        mppt.pv2_p_prev = mppt.pv2_p_sum;
    }
    if (pv2_po_state == 3) {
        if (mppt.pv2_ref <= mppt.pv2_ref_prev) {
            mppt.pv2_ref_prev = mppt.pv2_ref;
            if (mppt.pv2_ref < pv2_ref_max - mppt.v_step)
                mppt.pv2_ref = mppt.pv2_ref + mppt.v_step;
            else
                mppt.pv2_ref = pv2_ref_max;
        } else {
            mppt.pv2_ref_prev = mppt.pv2_ref;
            if (mppt.pv2_ref > mppt.v_step + 1200.0f)
                mppt.pv2_ref = mppt.pv2_ref - mppt.v_step;
            else
                mppt.pv2_ref = mppt.pv2_ref + 0.5f * mppt.v_step;
        }
    } else if (pv2_po_state == 0) {
        if (pv2_po_state == pv2_po_prev) {
            if (mppt.pv2_ref < mppt.pv2_ref_prev) {
                mppt.pv2_ref_prev = mppt.pv2_ref;
                if (mppt.pv2_ref < pv2_ref_max - 0.5f * mppt.v_step)
                    mppt.pv2_ref = 0.5f * mppt.v_step + mppt.pv2_ref;
                else
                    mppt.pv2_ref = pv2_ref_max;
            } else {
                mppt.pv2_ref_prev = mppt.pv2_ref;
                if (mppt.pv2_ref > 0.5f * mppt.v_step + 1200.0f)
                    mppt.pv2_ref = mppt.pv2_ref - 0.5f * mppt.v_step;
                else
                    mppt.pv2_ref = mppt.pv2_ref + 0.5f * mppt.v_step;
            }
        } else {
            if (mppt.pv2_ref <= mppt.pv2_ref_prev) {
                mppt.pv2_ref_prev = mppt.pv2_ref;
                if (mppt.pv2_ref > mppt.v_step + 1200.0f)
                    mppt.pv2_ref = mppt.pv2_ref - mppt.v_step;
                else
                    mppt.pv2_ref = mppt.pv2_ref + 0.5f * mppt.v_step;
            } else {
                mppt.pv2_ref_prev = mppt.pv2_ref;
                if (mppt.pv2_ref < pv2_ref_max - mppt.v_step)
                    mppt.pv2_ref = mppt.pv2_ref + mppt.v_step;
                else
                    mppt.pv2_ref = pv2_ref_max;
            }
        }
    }
}

/* PV1's MPPT, once per slow average, after PV detection (pv_detect_done 1 or 2).
   - Start (mppt_flags bit 4 clear): in states 5 or 7 (or bit 14), after 32 calls start a sweep at
     the measured voltage - 2.0 V, 50 steps.
   - Run: unless PV1 is power limited (bit 0, then the reference rises 0.1 V every 6th call),
     discard 50 samples and sum the next 50; then a mean under 60000 in dc_meas units (120000 in
     parallel mode) for 161 sums asks for a rescan; the sweep steps down 2.0 V per sum until 0.83 x
     Voc, 120 V or 50 steps (bit 8), then mppt_po_pv1 runs.
   - Resync: the reference more than 30.0 V from the measured voltage for over 500 calls sets bit 3,
     which pulls it to the measured voltage - 2.5 V at 2.5 V per call.
   Aging mode 1 fixes it at 250.0 V. pv1_ref_isr gets the reference before the 120 V .. max
   clamp. */
void mppt_pv1(void) {
    if (pv_detect_done == 1 || pv_detect_done == 2) {
        if (!mppt_flags.bit.b4) {
            if (op_state == OP_GRID_PV || op_state == OP_EPS_PV || mppt_flags.bit.b14) {
                if (pv1_start_count++ > 30) {
                    mppt_flags.bit.b4 = 1;
                    mppt.pv1_p_prev = 0.0f;
                    mppt.pv1_p_sum = 0.0f;
                    pv1_start_count = 0;
                    mppt_flags.bit.b8 = 1;
                    mppt.pv1_p_count = 0;
                    pv_phase = 0.0f;
                    mppt.pv1_ref_prev = mppt.pv1_ref;
                    mppt.pv1_ref = dc_meas.pv1_v - 20.0f;
                    mppt.pv1_sweep_steps = 50;
                }
            }
        } else {
            if (!mppt_flags.bit.b0) {
                pv1_plim_step_count = 0;
                if (mppt.pv1_p_count < 100) {
                    if (++mppt.pv1_p_count > 50) {
                        mppt.pv1_p_sum =
                            ((dsp_settings.pv_input_mode == 4) ? dc_meas.pv_p : dc_meas.pv1_p) +
                            mppt.pv1_p_sum;
                    }
                } else {
                    float p = 0.02f * mppt.pv1_p_sum;
                    if (p < ((dsp_settings.pv_input_mode == 4) ? 120000.0f : 60000.0f)) {
                        if (++pv1_low_power_count > 160) {
                            mppt.rescan = 1;
                            pv1_low_power_count = 0;
                        }
                    } else {
                        pv1_low_power_count = 0;
                    }
                    if (mppt_flags.bit.b8) {
                        mppt.pv1_ref_prev = mppt.pv1_ref;
                        mppt.pv1_ref = mppt.pv1_ref_prev - 20.0f;
                        if (mppt.pv1_sweep_steps != 0)
                            mppt.pv1_sweep_steps--;
                        if (mppt.pv1_ref < mppt.pv1_sweep_min || mppt.pv1_ref <= 1200.0f ||
                            mppt.pv1_sweep_steps == 0)
                            mppt_flags.bit.b8 = 0;
                    } else {
                        mppt_po_pv1();
                    }
                    mppt.pv1_p_count = 0;
                    mppt.pv1_p_sum = 0.0f;
                }
            } else {
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (++pv1_plim_step_count > 5) {
                    pv1_plim_step_count = 0;
                    mppt.pv1_ref = mppt.pv1_ref_prev + 1.0f;
                }
            }
            if (mppt.pv1_ref > dc_meas.pv1_v + 300.0f) {
                if (++pv1_ref_high_count > 500) {
                    mppt_flags.bit.b3 = 1;
                    pv1_ref_high_count = 250;
                }
            } else {
                pv1_ref_high_count = 0;
            }
            if (mppt.pv1_ref < dc_meas.pv1_v - 300.0f) {
                if (++pv1_ref_low_count > 500) {
                    pv1_ref_low_count = 250;
                    mppt_flags.bit.b3 = 1;
                }
            } else {
                pv1_ref_low_count = 0;
            }
            if (mppt_flags.bit.b3) {
                mppt.pv1_ref = dc_meas.pv1_v - 25.0f;
                rate_limit(&mppt.pv1_ref, mppt.pv1_ref_prev, 25.0f, -25.0f);
                mppt.pv1_ref_prev = mppt.pv1_ref;
                if (fabs(mppt.pv1_ref - dc_meas.pv1_v) <= 25.0f)
                    mppt_flags.bit.b3 = 0;
            }
        }
    }
    if (dsp_settings.aging_test_mode == 1)
        mppt.pv1_ref = 2500.0f;
    pv1_ref_isr = mppt.pv1_ref;
    limitf(&mppt.pv1_ref, pv1_ref_max, 1200.0f);
}

/* PV2's MPPT, like mppt_pv1 with its own words, except: starts after 31 calls, the low-power
   mean is 60000 only, the resync window is 20.0 V for over 250 calls, and in parallel mode (4) the
   reference copies PV1's. */
void mppt_pv2(void) {
    if (pv_detect_done == 1 || pv_detect_done == 2) {
        if (!mppt_flags.bit.b5) {
            if (op_state == OP_GRID_PV || op_state == OP_EPS_PV || mppt_flags.bit.b14) {
                if (++pv2_start_count > 30) {
                    mppt_flags.bit.b5 = 1;
                    mppt.pv2_p_prev = 0.0f;
                    mppt.pv2_p_sum = 0.0f;
                    pv2_start_count = 0;
                    mppt_flags.bit.b9 = 1;
                    mppt.pv2_p_count = 0;
                    pv2_phase_acc = 0.0f;
                    mppt.pv2_ref_prev = mppt.pv2_ref;
                    mppt.pv2_ref = dc_meas.pv2_v - 20.0f;
                    mppt.pv2_sweep_steps = 50;
                }
            }
        } else {
            if (dsp_settings.pv_input_mode != 4) {
                if (!mppt_flags.bit.b1) {
                    pv2_plim_step_count = 0;
                    if (mppt.pv2_p_count < 100) {
                        if (++mppt.pv2_p_count > 50) {
                            mppt.pv2_p_sum = mppt.pv2_p_sum + dc_meas.pv2_p;
                        }
                    } else {
                        float p = 0.02f * mppt.pv2_p_sum;
                        if (p < 60000.0f) {
                            if (++pv2_low_power_count > 160) {
                                mppt.rescan = 1;
                                pv2_low_power_count = 0;
                            }
                        } else {
                            pv2_low_power_count = 0;
                        }
                        if (mppt_flags.bit.b9) {
                            mppt.pv2_ref_prev = mppt.pv2_ref;
                            mppt.pv2_ref = mppt.pv2_ref_prev - 20.0f;
                            if (mppt.pv2_sweep_steps != 0)
                                mppt.pv2_sweep_steps--;
                            if (mppt.pv2_ref < mppt.pv2_sweep_min || mppt.pv2_ref <= 1200.0f ||
                                mppt.pv2_sweep_steps == 0)
                                mppt_flags.bit.b9 = 0;
                        } else {
                            mppt_po_pv2();
                        }
                        mppt.pv2_p_count = 0;
                        mppt.pv2_p_sum = 0.0f;
                    }
                } else {
                    mppt.pv2_ref_prev = mppt.pv2_ref;
                    if (++pv2_plim_step_count > 5) {
                        pv2_plim_step_count = 0;
                        mppt.pv2_ref = mppt.pv2_ref_prev + 1.0f;
                    }
                }
            } else {
                mppt.pv2_ref_prev = mppt.pv2_ref;
                mppt.pv2_ref = mppt.pv1_ref;
            }
            if (mppt.pv2_ref > dc_meas.pv2_v + 200.0f) {
                if (++pv2_ref_high_count > 250) {
                    pv2_ref_high_count = 250;
                    mppt_flags.bit.b7 = 1;
                }
            } else {
                pv2_ref_high_count = 0;
            }
            if (mppt.pv2_ref < dc_meas.pv2_v - 200.0f) {
                if (++pv2_ref_low_count > 250) {
                    pv2_ref_low_count = 250;
                    mppt_flags.bit.b7 = 1;
                }
            } else {
                pv2_ref_low_count = 0;
            }
            if (mppt_flags.bit.b7) {
                mppt.pv2_ref = dc_meas.pv2_v - 25.0f;
                rate_limit(&mppt.pv2_ref, mppt.pv2_ref_prev, 25.0f, -25.0f);
                mppt.pv2_ref_prev = mppt.pv2_ref;
                if (fabs(mppt.pv2_ref - dc_meas.pv2_v) <= 25.0f)
                    mppt_flags.bit.b7 = 0;
            }
        }
    }
    if (dsp_settings.aging_test_mode == 1)
        mppt.pv2_ref = 2500.0f;
    pv2_ref_isr = mppt.pv2_ref;
    limitf(&mppt.pv2_ref, pv2_ref_max, 1200.0f);
}
