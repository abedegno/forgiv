// clang-format off
/* The RCMU's fast 135 mA check, run from RAM inside the control interrupt every 10 samples (about
   0.5 ms), and the RCMU fault bit.
   Entry point: rcmu_fast_check, called by fast_fault_check (converter_ctrl.c) after it updates
   rcmu_i[2] (the mean of the last 10 residual current samples, in A).
   Data: none of its own; the watch block rcmu.c (0xDB64..).
   Neighbours: rcmu.c, rcmu_watch.c (the slow checks and the clearing).
   name: descriptive (was cal_fault.c; map/filenames.tsv).
   at: rcmu_fast_check 0x3F32EF (load address; it runs at 0x9738)
   match: compiled with 16.9.6 (docs/notes.md): 15.12.7 schedules the frame's SUBB SP
   before the last OR, the image and 16.6.0 and later after it. */
// clang-format on
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#include <math.h>
#include "F2806x_Device.h"
#include "util.h"
#include "faults.h"
#include "bms.h"
#include "meas.h"
#include "rcmu.h"
#include "control.h"
extern struct fault_words_t fault_words; /* match: own form, see OWN_fault_words */

#pragma CODE_SECTION(rcmu_fast_check, "ramfuncs");
/* With the watch enabled, armed, its sudden-change checks on and settled (1100 runs, 22 s): the
   fast mean more than 0.135 A away from the filtered mean (filt1) sets fast_trip. CF18 bit 6 is
   the OR of the watch's trip and fast_trip, written every call; rcmu_watch clears both. */
void rcmu_fast_check(void) {
    if (rcmu.c.en == 1 && rcmu.c.jump_check_on == 1 && rcmu.c.arm == 1 && rcmu.c.settle >= 1100) {
        if (fabsf(rcmu.c.src[2] - rcmu.c.filt1) > 0.135f)
            rcmu.c.fast_trip = 1;
    }
    fault_words.fault_stop2.bit.b6 = rcmu.c.trip | rcmu.c.fast_trip;
}
