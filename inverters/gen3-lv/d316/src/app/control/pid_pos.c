// clang-format off
/* Positional PID on a float state block, run from RAM. Not called in the image (dead code).
   Entry point: pid_pos_step.
   Data: none.
   Neighbours: pi_ctrl.c (the PI variants in use).
   name: descriptive, name kept (pid_pos.c; map/filenames.tsv).
   at: pid_pos_step 0x3F1EB9 (load address; it runs at 0x8302) */
// clang-format on
#include "F2806x_Device.h"

/* match: the positional PID sees the integrator i and the output u as volatile; everywhere else PI_BLK is control.h's */
#define OWN_PI_BLK
typedef struct {
    float e;
    float kp;
    float ki;
    float kd;
    float umax;
    float umin;
    float e1;
    volatile float i;
    volatile float u;
} PI_BLK;
#include "control.h"
#include "util.h"

/* clamp v to [min, max] through a copy of v */
#define SATURATE(v, max, min)                                                                      \
    {                                                                                              \
        float t_ = (v);                                                                            \
        (v) = (t_ > (max)) ? (max) : __fmax(t_, (min));                                            \
    }

#pragma CODE_SECTION(pid_pos_step, "ramfuncs");
/* Positional PID: i += ki e (clamped), u = i + kp e + kd (e - e1) (clamped), e1 = e; returns u.
   Unused. */
float pid_pos_step(PI_BLK *p) {
    float d;

    d = p->kp * p->e + p->kd * (p->e - p->e1);
    p->i += p->ki * p->e;
    SATURATE(p->i, p->umax, p->umin);
    p->e1 = p->e;
    p->u = p->i + d;
    SATURATE(p->u, p->umax, p->umin);
    return p->u;
}
