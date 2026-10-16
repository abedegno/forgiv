// clang-format off
/* The residual current monitor (RCMU): its three state blocks and the steps task 4 runs every
   20 ms. (Older comments called this an offset calibration; it is the RCMU, docs/names/C-notes.md
   finding 1.) The trip levels are 0.9 x the VDE 0126-1-1 levels: 27 mA, 54 mA and 135 mA sudden
   changes and a 0.3 A continuous limit (rcmu_watch.c, rcmu_fast_check.c).
   Inputs: rcmu_i[0..2] (0xD488..0xD48C) in A: [0] the RMS of the residual current over a grid cycle
   (task 1, current2_rms), [1] its cycle mean (task 3, meas_aux_f16), [2] a fast mean of 10 samples
   (control_isr, fast_fault_check). The sensor is ADC channel c12 (meas_fast.leak_i, mA).
   Entry points: rcmu_init (app_init.c, at start-up), rcmu_step (task 4, grid_support.c).
   Data: rcmu (0xDB40 .. 0xDB85): a, the zero offset; b, the self test; c, the watch.
   Switches: dsp_settings.rcmu_enable (0xD8D6, HR1 bit 1 inverted) turns all three on;
   rcmu_step_check_off (0xD8DA, HR1 bit 4 inverted) turns off the sudden-change checks.
   Effects: the watch's trips set CF18 bit 6 (rcmu_fast_check); the self test's result is
   run_status bit 7, which the state machine needs before it leaves the idle state when the RCMU
   is enabled (op_states.c).
   Neighbours: rcmu_selftest.c, rcmu_watch.c, rcmu_fast_check.c, grid_support.c (task 4).
   name: descriptive (was offset_cal.c; map/filenames.tsv). Header rcmu.h.
   at: rcmu_init 0x3F00E9 .. rcmu_zero_cal 0x3F01A1; rcmu 0xDB40 .. 0xDB85 */
// clang-format on
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#include <string.h>
#include <math.h>
#include "F2806x_Device.h"
#include "util.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "meas.h"
#include "grid.h"
#include "state.h"
#include "rcmu.h"
#include "control.h"
#include "power_limits.h"
extern struct fault_words_t fault_words; /* match: own form, see OWN_fault_words */

// clang-format off
/* ebss-begin */
struct rcmu_t rcmu;
/* ebss-end */
// clang-format on

/* Clears the three blocks, points them at rcmu_i and sets the watch's limit to 0.86 (rcmu_watch_step
   overwrites it with 0.3 on every run). */
void rcmu_init(void) {
    memset(&rcmu, 0, sizeof(rcmu));
    rcmu.c.limit = 0.86f;
    rcmu.a.src = rcmu_i;
    rcmu.c.src = rcmu_i;
    rcmu.b.src = rcmu_i;
}

/* Task 4's RCMU step (every 20 ms): zero offset, self test, watch. */
void rcmu_step(void) {
    rcmu_zero_step();
    rcmu_selftest_step();
    rcmu_watch_step();
}

/* The zero offset: enabled with the RCMU on and the self test not failed, armed after 500 task 4
   runs (10 s); the result goes to rcmu_offset (0xD412), which nothing reads. Its only effect is
   done, which arms the self test. */
void rcmu_zero_step(void) {
    Uint16 t, u;
    t = 0;
    if (dsp_settings.rcmu_enable != 0 && rcmu.b.failed != 1)
        t = 1;
    rcmu.a.en = t;
    u = 0;
    if (ctl_uptime_ticks >= 500)
        u = 1;
    rcmu.a.arm = u;
    rcmu.a.src = rcmu_i;
    rcmu_zero_cal(&rcmu.a);
    if (rcmu.a.done == 1)
        rcmu_offset = rcmu.a.out;
    else
        rcmu_offset = 0;
}

/* Averages src[1] (the mean residual current) over runs 6..13 after arming, 8 samples; a result
   above 0.06 A is taken as 0. Disabled, it clears everything including done. */
void rcmu_zero_cal(struct rcmu_zero_t *p) {
    float *src = p->src;
    if (p->en == 1) {
        if (p->arm == 1 && p->done == 0) {
            p->cnt++;
            if (p->cnt > 5) {
                p->sum += src[1];
                if (p->cnt >= 13) {
                    p->out = p->sum * 0.125f;
                    if (fabsf(p->out) > 0.06f)
                        p->out = 0;
                    p->cnt = 0;
                    p->done = 1;
                    p->sum = 0;
                }
            }
        } else {
            p->cnt = 0;
            p->sum = 0;
        }
    } else {
        p->cnt = 0;
        p->sum = 0;
        p->done = 0;
        p->out = 0;
    }
}

/* The self test: enabled with the RCMU on and not failed, armed after 10 s once the zero offset
   is done; skipped while CF18 bit 2 is set. Its passed flag is run_status bit 7 (0xD7C1). */
void rcmu_selftest_step(void) {
    Uint16 t;
    t = 0;
    if (dsp_settings.rcmu_enable != 0 && rcmu.b.failed != 1)
        t = 1;
    rcmu.b.en = t;
    if (ctl_uptime_ticks < 500)
        rcmu.b.arm = 0;
    else
        rcmu.b.arm = rcmu.a.done;
    rcmu.b.src = rcmu_i;
    if (!(fault_words.fault_stop2.bit.b2))
        rcmu_selftest(&rcmu.b);
    eps_cfg_bits.run_status.bit.b7 = rcmu.b.passed;
}

/* The watch: enabled with the RCMU on and the self test not failed, armed in op_states 5/6 by
   the self test's passed flag (it keeps its last value in other states), limit 0.3 A, the
   sudden-change checks on unless HR1 bit 4 says otherwise, and the self test's two open-relay
   means as the base for its clear test. */
void rcmu_watch_step(void) {
    Uint16 t, u;
    t = 0;
    if (dsp_settings.rcmu_enable != 0 && rcmu.b.failed != 1)
        t = 1;
    rcmu.c.en = t;
    if (ctl_uptime_ticks >= 500) {
        if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT)
            rcmu.c.arm = rcmu.b.passed;
    } else
        rcmu.c.arm = 0;
    u = 0;
    if (dsp_settings.rcmu_step_check_off == 0)
        u = 1;
    rcmu.c.jump_check_on = u;
    rcmu.c.src = rcmu_i;
    rcmu.c.limit = 0.3f;
    rcmu.c.base0 = rcmu.b.avg0;
    rcmu.c.base1 = rcmu.b.avg1;
    rcmu_watch(&rcmu.c);
}
