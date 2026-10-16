// clang-format off
/* Two small loops of the grid-tied inverter, run from RAM: the frequency-drift islanding detector
   (island_detect) and the cycle-averaged reactive current loop (q_loop_step).
   Entry points: island_detect(&island_det) from island_detect_run (converter_ctrl.c, once per grid
   cycle at PLL sample 100) and q_loop_step(&q_loop) from q_loop_run (ctrl_glue.c, every sample).
   Data: none of its own; the blocks are island_det (0xC04E) and q_loop (0xC180), set up by
   grid_ctrl_init.
   Neighbours: converter_ctrl.c, ctrl_glue.c; ac_ctrl.c's grid_ictl adds both outputs to the
   reactive current reference.
   name: descriptive (was detect.c; map/filenames.tsv).
   at: island_detect 0x3F2046, q_loop_step 0x3F20AF (load addresses; they run at 0x848F, 0x84F8) */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "util.h"

/* clamp v to [min, max] through a copy of v */
#define SATURATE(v, max, min)                                                                      \
    {                                                                                              \
        float t_ = (v);                                                                            \
        (v) = (t_ > (max)) ? (max) : __fmax(t_, (min));                                            \
    }

#pragma CODE_SECTION(island_detect, "ramfuncs");
/* Anti-islanding by frequency drift, once per grid cycle. The input is 1000 x the PLL's angle
   step (rad per sample; 1 unit is about 3.06 Hz at 19.2 kHz). d = 3 x the sample three cycles ago
   less the three newest samples, so d is minus the summed drift over the last three cycles.
   out = gain (50) x d, clamped to +-15 (1.5 A), goes into the reactive current reference: a
   positive feedback that pushes the frequency further once the grid is gone. cnt counts cycles
   with |d| > thr (0.97) and counts down below thr / 2; above limit (60) flag is set, and
   island_detect_run then stops the bridge (inv_off) and asks for an MPPT rescan. */
void island_detect(island_det_t *p) {
    float frequency_drift;

    p->hist3 = p->hist2;
    p->hist2 = p->hist1;
    p->hist1 = p->in;
    frequency_drift = p->hist3 * 3.0f - p->hist2 - p->hist1 - p->in;
    p->out = p->gain * frequency_drift;
    SATURATE(p->out, p->max, p->min);
    if (fabs(frequency_drift) > p->thr)
        p->cnt++;
    else if (fabs(frequency_drift) < p->thr * 0.5f) {
        if (p->cnt > 1)
            p->cnt--;
        else
            p->cnt = 0;
    }
    if (p->cnt > p->limit) {
        p->flag = 1;
        p->cnt = p->limit;
    } else
        p->flag = 0;
}

#pragma CODE_SECTION(q_loop_step, "ramfuncs");
/* The reactive current loop: every sample it sums half the reference and the measured current
   times the PLL cosine; once a cycle (at PLL sample 20) it takes the difference of the two sums
   as the error and steps the output u += kp e - ki e1 (kp 0.00061, ki 0.00049), clamped to +-51
   (5.1 A). u is q_corr in the grid controller's reactive reference. */
void q_loop_step(q_loop_t *p) {
    p->ref_sum += p->in * 0.5f;
    p->meas_sum += p->cos_in * p->i_in;
    if (p->n == 20) {
        p->e1 = p->e;
        p->e = p->ref_sum - p->meas_sum;
        p->ref_sum = 0;
        p->meas_sum = 0;
        p->u += p->e * p->kp - p->e1 * p->ki;
        SATURATE(p->u, p->max, p->min);
    }
}
