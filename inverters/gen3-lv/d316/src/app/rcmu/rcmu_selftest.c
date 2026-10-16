// clang-format off
/* The RCMU self test: closes and opens a test circuit through GPIO52 and checks that the residual
   current reading moves by a plausible amount. (Older comments called it a relay test; the
   "relay" on GPIO52 is the RCMU's test circuit, docs/names/C-notes.md finding 1.)
   Entry point: rcmu_selftest(&rcmu.b), from rcmu_selftest_step (rcmu.c, task 4, every 20 ms).
   Data: none of its own; the block rcmu.b (0xDB4C..).
   Neighbours: rcmu.c, rcmu_watch.c (uses avg0 and avg1 as its base).
   name: descriptive (was relay_test.c; map/filenames.tsv).
   at: rcmu_selftest 0x3F01EF */
// clang-format on
#include "F2806x_Device.h"
#include "F2806x_Gpio.h"
#include "rcmu.h"
#include "util.h"

/* One step of the self test, one per 20 ms run (about 5 s in all):
   - steps 1..14: test circuit open, sums cleared; steps 16..34: sum src[1] (the mean residual
     current); step 36: open_avg = that mean;
   - step 40: GPIO52 set (test circuit closed); steps 46..89: src[1] must differ from open_avg by
     0.02..0.15 A; more than 5 misses in a row restart the test, and the second restart sets
     failed (fail_value keeps the reading). failed disables the whole RCMU (rcmu.c), so the
     inverter cannot leave idle while the RCMU is enabled;
   - step 115: GPIO52 cleared; steps 151..249: sum src[0] and src[1]; step 250: avg0 and avg1, the
     open-circuit RMS and mean, and done;
   - after done, passed is set (step 250). Unarmed or disabled: circuit open, passed and step
     cleared (and done, when disabled). */
void rcmu_selftest(rcmu_selftest_t *p) {
    float *m = p->src;
    Uint16 s;

    if (p->en == 1) {
        if (p->arm == 1) {
            if (p->done == 0) {
                p->passed = 0;
                if (p->step > 250)
                    p->step = 0;
                else
                    p->step++;
                s = p->step;
                if (s < 251) {
                    if (s < 15) {
                        GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
                        p->n = 0;
                        p->sum1 = 0;
                        p->sum0 = 0;
                        p->open_avg = 0;
                        p->avg0 = 0;
                        p->avg1 = 0;
                    } else if (s > 15) {
                        if (s < 35) {
                            p->sum1 += m[1];
                            p->n++;
                        } else if (s == 36) {
                            if (p->n != 0) {
                                p->open_avg = p->sum1 / (float)p->n;
                                p->sum1 = 0;
                                p->n = 0;
                            }
                        } else if (s == 40) {
                            GpioDataRegs.GPBSET.bit.GPIO52 = 1;
                            p->n = 0;
                            p->sum1 = 0;
                            p->sum0 = 0;
                            p->err = 0;
                        } else if (s > 45) {
                            if (s < 90) {
                                float d = fabs(m[1] - p->open_avg);
                                if (d > 0.15f || d < 0.02f) {
                                    if (p->err < 5) {
                                        p->err++;
                                    } else {
                                        p->step = 0;
                                        p->retries++;
                                        if (p->retries >= 2) {
                                            p->fail_value = m[1];
                                            p->failed = 1;
                                        }
                                    }
                                } else {
                                    p->err = 0;
                                }
                            } else if (s == 115) {
                                GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
                                p->n = 0;
                                p->sum0 = 0;
                                p->sum1 = 0;
                            } else if (s > 150) {
                                if (s < 250) {
                                    p->sum0 += m[0];
                                    p->sum1 += m[1];
                                    p->n++;
                                } else if (s == 250) {
                                    if (p->n != 0) {
                                        p->avg0 = p->sum0 / (float)p->n;
                                        p->avg1 = p->sum1 / (float)p->n;
                                    }
                                    p->n = 0;
                                    p->sum0 = 0;
                                    p->sum1 = 0;
                                    p->err = 0;
                                    p->done = 1;
                                }
                            }
                        }
                    }
                }
            } else {
                GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
                p->err = 0;
                if (p->step < 250)
                    p->step = 0;
                else
                    p->passed = 1;
            }
        } else {
            GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
            p->passed = 0;
            p->step = 0;
        }
    } else {
        GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;
        p->passed = 0;
        p->step = 0;
        p->done = 0;
    }
}
