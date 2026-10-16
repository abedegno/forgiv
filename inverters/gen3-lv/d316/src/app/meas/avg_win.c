// clang-format off
/* A running mean of |in| over a quarter of a grid cycle, with a window check: the alternative fast
   grid voltage check (grid_v_check, with 0xD7C0 bit 10). Run from RAM, once per control
   interrupt.
   Entry point: avg_win_step(&grid_v_win) (block at 0xC300, AVG_WIN in control.h).
   Data: this file's own start-up state at 0xCF3A.. (below): the window length, the clear index,
   the restart flag, the previous mode and the settle count.
   Neighbours: converter_ctrl.c (grid_v_check sets the limits 91..345 V and reads ok).
   name: descriptive, name kept (avg_win.c; map/filenames.tsv).
   at: avg_win_step 0x3F1F20 (load address; it runs at 0x8369) */
// clang-format on
#include "F2806x_Device.h"
#include "util.h"
#include "meas.h"
#include "control.h"
#include "mppt.h"

// clang-format off
// ebss-begin
Uint16 win_len = 0x60;
Uint16 win_clear_idx = 0;
Uint16 win_restart = 1;
Uint16 win_mode_prev = 0;
Uint16 win_settle = 0;
// ebss-end
// clang-format on

#pragma CODE_SECTION(avg_win_step, "ramfuncs");
/* One step. A restart (at boot, after a 50/60 Hz mode change or after a failed check) zeroes the
   mean and clears one word of the 100-word buffer per call. Running: the sum of |in| over the last
   80 samples (mode 1, 60 Hz) or 96 (50 Hz), a quarter cycle at 19.2 kHz, times 1/80 or 1/96
   (0.0104, a rounded 1/96). After 38400 samples (2 s) of settling, a mean outside min..max clears
   ok and restarts; otherwise ok is 1. For a sine the mean of |v| is 0.9 x its RMS, so 91..345 V
   is about 101..383 V RMS. */
void avg_win_step(AVG_WIN *p) {
    if (p->mode != win_mode_prev)
        win_restart = 1;
    win_mode_prev = p->mode;
    if (win_restart == 1) {
        p->idx = 0;
        p->sum = 0;
        p->avg = 0;
        p->buf[win_clear_idx] = 0;
        win_clear_idx++;
        if (win_clear_idx >= 100) {
            win_clear_idx = 0;
            win_restart = 0;
        }
    } else {
        p->abs_in = fabs(p->in);
        p->sum = p->abs_in - p->buf[p->idx] + p->sum;
        p->buf[p->idx] = p->abs_in;
        if (p->mode == 1) {
            win_len = 80;
            p->avg = p->sum * 0.0125f;
        } else {
            win_len = 96;
            p->avg = p->sum * 0.0104f;
        }
        p->idx++;
        if (p->idx >= win_len)
            p->idx = 0;
        if (win_settle < 0x9600) {
            win_settle++;
        } else if (p->avg > p->max || p->avg < p->min) {
            p->ok = 0;
            win_restart = 1;
            win_settle = 0;
        } else {
            p->ok = 1;
        }
    }
}
