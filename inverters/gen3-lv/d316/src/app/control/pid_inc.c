// clang-format off
/* Incremental PID on a float state block, run from RAM. Not called in the image (dead code).
   Entry point: pid_inc_step.
   Data: none.
   Neighbours: pi_ctrl.c (the PI variants in use).
   name: descriptive, name kept (pid_inc.c; map/filenames.tsv).
   at: pid_inc_step 0x3F1FEF (load address; it runs at 0x8438)
   match: the block is pi_ctrl.c's with the upper limit volatile: the image reads u back after the
   store before it reads umax, and reads umax again for the clamp. */
// clang-format on
#include "F2806x_Device.h"

/* match: the incremental PID sees umax and u as volatile and calls the word at 0xE e2 (the error before e1); everywhere else PI_BLK is control.h's */
#define OWN_PI_BLK
typedef struct {
    float e;             /* 0x0: error input */
    float kp;            /* 0x2 */
    float ki;            /* 0x4 */
    float kd;            /* 0x6 */
    volatile float umax; /* 0x8 */
    float umin;          /* 0xA */
    float e1;            /* 0xC: previous error */
    float e2;            /* 0xE: the error before that */
    volatile float u;    /* 0x10: output */
} PI_BLK;
#include "control.h"
#include "util.h"

#pragma CODE_SECTION(pid_inc_step, "ramfuncs");
/* Incremental PID: u += (kp + ki + kd) e - (kp + 2 kd) e1 + kd e2, clamped to umin..umax;
 * shifts e1 into e2 and e into e1; returns u. Unused. */
float pid_inc_step(PI_BLK *p) {
    float d = (p->kp + p->ki + p->kd) * p->e - (p->kp + 2 * p->kd) * p->e1 + p->kd * p->e2;
    p->e2 = p->e1;
    p->e1 = p->e;
    p->u += d;
    {
        float t_ = p->u;
        p->u = (t_ <= p->umax) ? __fmax(t_, p->umin) : p->umax;
    }
    return p->u;
}
