// clang-format off
/* The PI controllers on float state blocks (PI_BLK, 18 words), run from RAM. Every converter
   loop uses pi_step; the PLL uses pi_inc_step.
   Entry points: pi_step (positional PI, used everywhere: grid bus PI, battery converter, PV
   loops, task 1 and task 4 PIs), pi_inc_step (incremental PI, the PLL), pi_reset and pi_reset_b
   (clear e1, i and u; neither is called in the image).
   Data: none of its own.
   Neighbours: pid_inc.c and pid_pos.c hold two more variants (both unused). pll.c calls
   pi_inc_step.
   name: descriptive (was ctrl_pi.c; map/filenames.tsv).
   at: pi_step 0x3F1F08, pi_reset 0x3F1FAE, pi_inc_step 0x3F2056, pi_reset_b 0x3F20D4 (load
   addresses; pid_inc_step is in pid_inc.c) */
// clang-format on
#include "F2806x_Device.h"

/* match: the PI functions see the output u as volatile (the image stores it and reads it back
 * for the saturation and the return); everywhere else PI_BLK is control.h's */
#define OWN_PI_BLK
typedef struct {
    float e;          /* 0x0: error input */
    float kp;         /* 0x2 */
    float ki;         /* 0x4 */
    float kd;         /* 0x6 */
    float umax;       /* 0x8 */
    float umin;       /* 0xA */
    float e1;         /* 0xC: previous error */
    float i;          /* 0xE: integrator */
    volatile float u; /* 0x10: output */
} PI_BLK;
#include "control.h"
#include "util.h"

/* clamp v to [min, max] through a copy of v */
#define SATURATE(v, max, min)                                                                      \
    {                                                                                              \
        float t_ = (v);                                                                            \
        (v) = (t_ > (max)) ? (max) : __fmax(t_, (min));                                            \
    }

#pragma CODE_SECTION(pi_step, "ramfuncs");
/* Positional PI: i += ki e, clamped to umin..umax; u = kp e + i, clamped to umin..umax; returns u.
 * The integrator uses the output limits, so it cannot wind up beyond them. The caller writes e
 * first; e1 is not used. match: the first clamp is an if/else: the image reads i back before
 * umax there, and umax first in the ternary */
float pi_step(PI_BLK *p) {
    p->i += p->e * p->ki;
    {
        float t_ = p->i;
        if (t_ > p->umax)
            p->i = p->umax;
        else
            p->i = __fmax(t_, p->umin);
    }
    p->u = p->e * p->kp + p->i;
    SATURATE(p->u, p->umax, p->umin);
    return p->u;
}

#pragma CODE_SECTION(pi_reset, "ramfuncs");
/* Clears e1, the integrator and the output. Not called in the image (the callers clear the
   fields inline). */
void pi_reset(PI_BLK *p) {
    p->e1 = 0;
    p->i = 0;
    p->u = 0;
}

#pragma CODE_SECTION(pi_inc_step, "ramfuncs");
/* Incremental PI: u += (kp + ki) e - kp e1, clamped to umin..umax; e1 = e; returns u. The PLL's
 * frequency loop (pll.c). */
float pi_inc_step(PI_BLK *p) {
    float e = p->e;
    float d = (p->kp + p->ki) * e - p->kp * p->e1;
    p->e1 = e;
    p->u += d;
    SATURATE(p->u, p->umax, p->umin);
    return p->u;
}

#pragma CODE_SECTION(pi_reset_b, "ramfuncs");
/* The same body as pi_reset, a second copy. Not called in the image. */
void pi_reset_b(PI_BLK *p) {
    p->e1 = 0;
    p->i = 0;
    p->u = 0;
}
