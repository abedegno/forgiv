/* The PV inputs: the MPPT and the boost converters' phase blocks.
   mppt.c (mppt_pv1/2, the perturb and observe steps) and mppt_step.c (pv_mppt_step, the dispatch);
   the MPPT state block mppt (0xD1C0) is declared in meas.h. */
#ifndef MPPT_H
#define MPPT_H

#include "F2806x_Device.h"
#include "util.h"
#include "control.h"

/* 0xD240: two blocks of 0x56 words, one per phase (EPWM1 CMPA and CMPB): the inputs, the
 * compare value waiting to be loaded and its flag, and four PI blocks */
typedef struct pv_phase_t {
    float i_ceiling;
    float i_limit; /* ramped up by pv_mppt_step to i_max */
    float i_max;   /* 1500 in every rating preset */
    float v_loop_out;
    float i_ref;
    float duty;
    Uint16 cmp;                  /* compare value */
    volatile Uint16 cmp_pending; /* 1: x0C waits to be loaded */
    PI_BLK pi_bus;
    PI_BLK pi_precharge;
    PI_BLK pi_v;
    PI_BLK pi_i;
} pv_phase_t;

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* mppt.c */
void mppt_po_pv1(void);
void mppt_pv1(void);
void mppt_pv2(void);
extern unsigned int pv_both_low_count;
extern unsigned int voc_wait_count;

/* mppt_step.c */
void pv_mppt_step(void);

/* defined by no source: the linker places it at its map address */
extern float pv2_i_limit_alias;
extern float pv2_phase_acc;
#ifndef OWN_pv_phase
extern pv_phase_t pv_phase[2];
#endif
/* ---- end of declarations ---- */

#endif
