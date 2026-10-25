// clang-format off
/* The RCMU watch: the residual current trip checks at 0.9 x the VDE 0126-1-1 levels.
   VDE 0126-1-1 (as I read the standard; the code only has the numbers) asks for a trip on a
   continuous residual current over 300 mA within 0.3 s, and on sudden changes of 30 mA within
   0.3 s, 60 mA within 0.15 s and 150 mA within 0.04 s. Here (runs of 20 ms):
     continuous: src[0] or |src[1]| over limit (0.3 A) for more than 10 runs (220 ms);
     30 mA: after a step of 30 mA or more, the reading stays more than 27 mA from its filtered
            value for 10 runs (about 200 ms);
     60 mA: the same with 54 mA for 3 runs (about 60 ms);
     150 mA: src[2] or src[0] more than 135 mA from its filtered value, at once; the same test
            on src[2] runs every 0.5 ms in rcmu_fast_check.
   Entry point: rcmu_watch(&rcmu.c), from rcmu_watch_step (rcmu.c, task 4).
   Data: none of its own; the block rcmu.c (0xDB64..).
   Neighbours: rcmu.c, rcmu_fast_check.c (sets fast_trip and the fault bit CF18 bit 6).
   name: descriptive (was meas_watch.c; map/filenames.tsv).
   at: rcmu_watch 0x3F037A */
// clang-format on
#include "F2806x_Device.h"
#include "rcmu.h"
#include "util.h"

/* trip: clear the counters, set trip and keep the value it tripped on */
#define TRIP(v)                                                                                    \
    {                                                                                              \
        p->over_count = 0;                                                                         \
        p->jump30_count = 0;                                                                       \
        p->jump60_count = 0;                                                                       \
        p->clear_count = 0;                                                                        \
        p->trip = 1;                                                                               \
        p->trip_value = (v);                                                                       \
    }

/* One run of the watch while enabled and armed. filt0 and filt1 follow src[0] and src[1] slowly
   (0.99/0.01, a time constant of about 2 s). The first 1100 runs (22 s) only settle the filters.
   Then the checks in the header. The step detector compares each reading with its value two runs
   back (40 ms), and only while a reading is more than 6.1 mA from the base; the second half of
   that gate compares src[0] with base1 where base0 looks meant (docs/findings-control.md).
   A trip clears after 250 runs (5 s) with no check counting and both readings within 16.8 mA
   above the self test's open-circuit means. Disabled or unarmed, everything is cleared,
   including the trips. */
void rcmu_watch(rcmu_watch_t *p) {
    float *samples = p->src;
    float abs_leak, step1, step0, deviation1, deviation0;

    if (p->en == 1 && p->arm == 1) {
        p->filt0 = 0.99f * p->filt0 + 0.01f * samples[0];
        p->filt1 = 0.01f * samples[1] + 0.99f * p->filt1;
        p->settle++;
        if (p->settle < 1100) {
            p->over_count = 0;
            p->jump30_count = 0;
            p->jump60_count = 0;
            p->clear_count = 0;
            return;
        }
        p->settle = 1100;
        abs_leak = fabs(samples[1]);
        if (samples[0] > p->limit || abs_leak > p->limit) {
            p->over_count++;
            if (p->over_count > 10) {
                TRIP((abs_leak < 0.07f) ? samples[0] : abs_leak);
            }
        } else {
            p->over_count = 0;
        }
        if (p->jump_check_on == 1) {
            if (fabs(samples[2] - p->filt1) > 0.135f || fabs(samples[0] - p->filt0) > 0.135f) {
                TRIP(abs_leak);
            }
            if (fabs(samples[1] - p->base1) > 0.0061f || fabs(samples[0] - p->base1) > 0.0061f) {
                step1 = fabs(samples[1] - p->prev1);
                step0 = fabs(samples[0] - p->prev0);
            } else {
                step1 = 0;
                step0 = step1;
            }
            p->prev1 = p->last1;
            p->last1 = samples[1];
            p->prev0 = p->last0;
            p->last0 = samples[0];
            deviation1 = fabs(samples[1] - p->filt1);
            deviation0 = fabs(samples[0] - p->filt0);
            if (p->jump60_count == 0) {
                if (step1 >= 0.03f || step0 >= 0.03f)
                    p->jump60_count++;
            } else {
                if (deviation1 > 0.054f || deviation0 > 0.054f)
                    p->jump60_count++;
                else
                    p->jump60_count = 0;
                if (p->jump60_count >= 3)
                    TRIP(abs_leak);
            }
            if (p->jump30_count == 0) {
                if (step1 >= 0.03f || step0 >= 0.03f)
                    p->jump30_count++;
            } else {
                if (deviation1 > 0.027f || deviation0 > 0.027f)
                    p->jump30_count++;
                else
                    p->jump30_count = 0;
                if (p->jump30_count >= 10)
                    TRIP(abs_leak);
            }
        }
        if ((p->trip == 1 || p->fast_trip == 1) &&
            (p->over_count | p->jump60_count | p->jump30_count) == 0) {
            if (samples[0] < p->base0 + 0.01677f && samples[1] < p->base1 + 0.01677f) {
                if (p->clear_count < 250) {
                    p->clear_count++;
                } else {
                    p->clear_count = 0;
                    p->trip = 0;
                    p->fast_trip = 0;
                }
                return;
            }
        }
        p->clear_count = 0;
    } else {
        p->over_count = 0;
        p->jump30_count = 0;
        p->jump60_count = 0;
        p->clear_count = 0;
        p->settle = 0;
        p->trip = 0;
        p->fast_trip = 0;
    }
}
