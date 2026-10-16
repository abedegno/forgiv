// clang-format off
/* The AC-side controllers of the bridge (EPWM3/EPWM4), run from RAM inside control_isr at
   19.2 kHz: the grid current controller (grid_ictl, block 0xC400, used in op_states 5 and 6), the
   EPS voltage controller (eps_vctl, block 0xC840, op_states 7 and 8) and the repetitive filter both
   of them share (rep_filter_step).
   Entry points: grid_ictl and eps_vctl, called by grid_ictl_run and eps_vctl_run in
   converter_ctrl.c, which fill the blocks from the measurements first. The blocks belong to
   converter_ctrl.c (grid_ctrl_init, eps_vctl_init set them up); this file owns no data.
   Units: currents in 0.1 A (meas_fast.inv_i), voltages in 0.1 V (meas_fast.grid_v, inv_v). The
   output (v_cmd or duty) is a bridge voltage that converter_ctrl.c scales by mod_gain
   (pwm_period / bus voltage) into the compare value.
   Neighbours: pi_ctrl.c (pi_step), converter_ctrl.c (callers, inv_pwm_update), pll.c (sin and cos).
   name: descriptive (was vctl.c; map/filenames.tsv). The two block types were named the wrong way
   round before the readability pass: VCTL is grid_ictl_t and ICTL is eps_vctl_t (docs/names/C-notes.md).
   at: rep_filter_step 0x3F2118, grid_ictl 0x3F3153, eps_vctl 0x3F3229 (load addresses; they run at
   0x8561, 0x959C and 0x9672).
   match: compiled with 16.9.6 (docs/notes.md): 15.12.7 gives eps_vctl's res.r pointer
   XAR4, the image and 16.9.5/16.9.6 XAR7; the other two match with either. */
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
#pragma CODE_SECTION(rep_filter_step, "ramfuncs");
/* The repetitive (resonant) filter step: y follows in with weight a, the delay ring buf holds
   b * old + y at the write index, out is the gain times the sample at the read index, limited to
   max/min. Grid controller: a 0.8, b 0.923, gain 1.0, +-700, ring of one grid period (up to 430).
   EPS controller: a 0.5, b 0.78, gain 0.16, +-30, ring of up to 500. One call per sample.
   The indices are volatile (the callers set them just before the call). */
void rep_filter_step(rep_filter_t *p) {
    float *pw, *pr, *buf;
    Uint16 w, r;

    buf = p->buf;
    p->b = __fmin(p->b, 1.0f);
    p->a = __fmin(p->a, 1.0f);
    w = p->w;
    pw = &buf[w];
    r = p->r;
    pr = &buf[r];
    p->y = p->y * p->a + p->in * (1.0f - p->a);
    *pw = *pw * p->b + p->y;
    p->out = *pr * p->gain;
    SATURATE(p->out, p->max, p->min);
}

#pragma CODE_SECTION(grid_ictl, "ramfuncs");
/* The grid-tied current controller, once per control interrupt in op_states 5 and 6.
   - Outer loop (mode 0, on ctrl_slot 0 only, 2.4 kHz): a bus voltage PI on bus_v_ref - bus_v
     (error clamped to +-200 = 20.0 V; kp 0.6, ki 0.008) gives the active current amplitude
     amp_cmd (0.1 A peak), negated, inside amp_min..amp_max. amp_max/amp_min are the peak current
     bounds task 4 writes (i_ref_max, i_ref_min at 0xC412/0xC414).
   - Mode 1 (pv_bus_mode, never set in this image) ramps the amplitude by 3 per cycle instead.
   - Active reference amp x sin, reactive reference (q_cmd + island_inj + q_corr) x cos, from the
     PLL: q_cmd is task 4's reactive current (0xC408), island_inj the islanding detector's
     injection and q_corr the cycle-averaged reactive loop (q_loop).
   - Inner loop: the current error times kp_i (4.5) plus the repetitive filter on the same error
     (delay line of len samples, one grid period, read at n and written rep_lead (5) samples
     ahead) plus the grid voltage feed-forward v_ff (with the adc_scale lead term) and v_ff2
     (0xC42A, task 4's DC injection correction) is the bridge voltage v_cmd. */
void grid_ictl(grid_ictl_t *p) {
    if (p->mode == 0) {
        if (p->slot == 0) {
            p->pi_bus.e = p->bus_v_ref - p->bus_v;
            {
                float t_ = p->pi_bus.e;
                p->pi_bus.e = (t_ >= 200.0f) ? 200.0f : __fmax(t_, -200.0f);
            }
            p->amp_cmd = -pi_step(&p->pi_bus);
            {
                float t_ = p->amp_cmd;
                p->amp_cmd = (t_ >= p->amp_max) ? p->amp_max : __fmax(t_, p->amp_min);
            }
        }
        p->i_ref_p = p->amp_cmd * p->sin_ref;
    } else {
        if (p->n == 0) {
            if (p->amp_ramp < p->amp_max)
                p->amp_ramp += 3.0f;
            else
                p->amp_ramp = p->amp_max;
        }
        p->i_ref_p = p->amp_ramp * p->sin_ref;
    }
    p->i_ref_q = (p->q_cmd + p->island_inj + p->q_corr) * p->cos_ref;
    p->i_ref = p->i_ref_p + p->i_ref_q;
    p->i_err = p->i_ref - p->i_meas;
    p->res.in = p->i_err;
    if (p->n > 499)
        p->n = 0;
    p->res.r = p->n;
    p->res.w = p->n + p->len - p->rep_lead;
    if (p->res.w >= p->len)
        p->res.w -= p->len;
    rep_filter_step(&p->res);
    p->v_cmd = p->i_err * p->kp_i + p->v_ff + p->v_ff2 + p->res.out;
}

#pragma CODE_SECTION(eps_vctl, "ramfuncs");
/* The EPS (off-grid) voltage controller, once per control interrupt in op_states 7 and 8.
   mode is 0xD7C0 bit 9 (EPS enabled); with mode 0 the duty is 0.
   - The amplitude v_peak rises 2.5 (0.25 V) per sample to v_amp_ref (sqrt 2 x eps_v_ref), and
     ramp_done is set when it gets there. amp_trim (0xCC76, task 1's slow RMS loop,
     eps_voltage_control) is then added every sample, so the peak is v_amp_ref + amp_trim.
   - v_ref = v_peak sin(theta) + dc_trim (0xCC74, always 0: eps_dc_offset_trim writes 0).
   - Voltage loop: pi_v on v_ref - v_meas (kp 0.4, ki 0 from eps_vctl_init, so proportional
     only), plus the repetitive filter on the same error once the ramp is done (read index n,
     write index n + len - 6), plus i_filt (the filtered inverter current, a load feed-forward).
     The sum is the current reference, limited to +-i_limit (370 = 37.0 A).
   - Current loop: pi_i on i_ref - i_meas (kp 1.9, ki 0), plus v_ff (the measured inverter
     voltage) is the duty.
   match: res.r is stored through a pointer (the image's XAR7 = p + 23). */
void eps_vctl(eps_vctl_t *p) {
    if (p->mode == 1) {
        p->v_ref = p->v_amp_ref;
        p->v_peak += 2.5f;
        if (p->v_peak >= p->v_amp_ref) {
            p->ramp_done = 1;
            p->v_peak = p->v_amp_ref;
        }
        p->v_peak += p->amp_trim;
        p->v_ref = p->v_peak * p->sin_ref + p->dc_trim;
        p->pi_v.e = p->v_ref - p->v_meas;
        pi_step(&p->pi_v);
        if (p->ramp_done == 1) {
            p->res.in = p->pi_v.e;
            {
                Uint16 *pr = (Uint16 *)&p->res.r;
                *pr = p->n;
            }
            p->res.w = p->n + p->len - p->rep_lead;
            if (p->res.w >= p->len)
                p->res.w -= p->len;
            rep_filter_step(&p->res);
        } else {
            p->res.out = 0;
        }
        p->i_ref = p->pi_v.u + p->res.out + p->i_filt;
        if (p->i_ref > p->i_limit)
            p->i_ref = p->i_limit;
        else
            p->i_ref = __fmax(p->i_ref, -p->i_limit);
        p->pi_i.e = p->i_ref - p->i_meas;
        pi_step(&p->pi_i);
        p->duty = p->pi_i.u + p->v_ff;
    } else {
        p->duty = 0;
    }
}
