#include "util.h"
// clang-format off
// cflags: -Ooff
/* The rate limiter shared by the tasks, run from RAM.
 * Entry point: rate_limit, called by tasks 1, 2 and 4 and the MPPT (ac_measure.c, power_limits.c,
 * grid_support.c, mppt.c) for their slow references and power limits.
 * Data: none.
 * name: descriptive, name kept (rate_limit.c; map/filenames.tsv).
 * match: compiled without optimisation (-Ooff, the cflags line above), like util.c.
 * at: rate_limit 0x3F3320 (load address; it runs at 0x9769) */
// clang-format on
#pragma CODE_SECTION(rate_limit, "ramfuncs");
/* Rate limit: *x becomes ref + (*x - ref) limited to lo..up, so a new value may move at most up
 * above or -lo below the previous one (ref). The callers store the result as the next ref. */
void rate_limit(float *x, float ref, float up, float lo) {
    float delta;
    delta = *x - ref;
    if (delta > up)
        delta = up;
    if (delta < lo)
        delta = lo;
    *x = ref + delta;
}
