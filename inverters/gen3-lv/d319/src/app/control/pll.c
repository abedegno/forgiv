// clang-format off
/* The grid phase-locked loop: a single-phase PLL on a float state block, run from RAM once per
   control interrupt (19.2 kHz).
   Entry point: pll_step(&grid_pll), called by grid_pll_run (ctrl_glue.c).
   Data: none of its own; the block is grid_pll (0xC100), set up by grid_ctrl_init
   (converter_ctrl.c): filter gain k 0.0098, phase lead 2.8 degrees, PI kp 7e-5, ki 5e-8, step
   limits 0.00818..0.02618 rad per sample (25 Hz .. 80 Hz at 19.2 kHz).
   Users of the outputs: grid_ictl (sin and cos references), q_loop, island_detect_run (the angle
   step w), relay_drive (sample index n), grid_angle_update (theta, period, wrap), task 3
   (pll_locked before it starts the grid inverter).
   name: descriptive, name kept (pll.c; map/filenames.tsv).
   at: pll_step 0x3F1E30 (load address; it runs at 0x81DC) */
// clang-format on
#include "F2806x_Device.h"

/* match: the PLL's PI block has a volatile output here (pi_inc_step stores it and the image
 * reads it back); control.h's PI_BLK, which the PLL struct embeds, is this one in this file */
#define OWN_PI_BLK
typedef struct {
    float e, kp, ki, kd, umax, umin, e1, i;
    volatile float u;
} PI_BLK;
#include "control.h"
#include "util.h"
#include "meas.h"

#define TWO_PI 6.28318548f

#pragma CODE_SECTION(pll_step, "ramfuncs");
/* One PLL step. The phase detector is in x cos(theta) (in is the grid voltage in V), smoothed by
   two first-order low-pass stages (gain k each). Its output is the error of an incremental PI
   (pi_inc_step) whose output w is the angle step per sample; theta += w. When theta passes 2 pi:
   wrap is set for this sample, period takes the sample count of the cycle and n restarts, and
   the summed phase error of the cycle decides the lock: |sum| < 20 x period counts a good cycle,
   otherwise a bad one; 20 good cycles set locked (and clear bad), 10 bad cycles clear it. The
   outputs s and c are sin and cos of theta plus shift degrees (2.8, a lead for the sampling and
   filter delay; inference). */
void pll_step(PLL *p) {
    float shifted_angle;

    p->wrap = 0;
    p->n++;
    p->pd_filt1 += p->k * (p->in * cos(p->theta) - p->pd_filt1);
    p->pd_filt2 += p->k * (p->pd_filt1 - p->pd_filt2);
    p->sum += p->pd_filt2;
    p->pi.e = p->pd_filt2;
    p->w = pi_inc_step(&p->pi);
    p->theta += p->w;
    if (p->theta >= TWO_PI) {
        p->theta -= TWO_PI;
        p->wrap = 1;
        p->last = p->sum;
        p->sum = 0;
        p->period = p->n;
        p->n = 0;
        if (fabs(p->last) < p->period * 20.0f) {
            if (++p->good >= 20) {
                p->good = 20;
                p->bad = 0;
                p->locked = 1;
            }
        } else {
            if (++p->bad >= 10) {
                p->good = 0;
                p->bad = 10;
                p->locked = 0;
            }
        }
    }
    shifted_angle = p->theta + p->shift * 0.0174532924f;
    if (shifted_angle > TWO_PI)
        shifted_angle -= TWO_PI;
    p->s = sin(shifted_angle);
    p->c = cos(shifted_angle);
}
