// clang-format off
/* Glue between the control interrupt and two controller blocks, run from RAM at 19.2 kHz:
   scale the input, run the block, copy its outputs.
   Entry points: grid_pll_run and q_loop_run, both called by control_isr (converter_ctrl.c).
   Data: none of its own; it reads meas_fast and writes the PLL block (grid_pll, 0xC100), the
   reactive loop block (q_loop, 0xC180) and ctrl_globals.pll_sin/pll_cos (0xC028/0xC02A).
   Neighbours: pll.c (pll_step), island_detect.c (q_loop_step), converter_ctrl.c.
   name: descriptive, name kept (ctrl_glue.c; map/filenames.tsv).
   at: grid_pll_run 0x3F22CA, q_loop_run 0x3F22E5 (load addresses; they run at 0x8713, 0x872E) */
// clang-format on
#include "F2806x_Device.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

#pragma CODE_SECTION(grid_pll_run, "ramfuncs");
/* Runs the grid PLL once per sample: the input is the grid voltage in V (meas_fast.grid_v is
   in 0.1 V), and the PLL's sin and cos (with its 2.8 degree lead) become the references
   grid_ictl and q_loop use. */
void grid_pll_run(void) {
    grid_pll.in = 0.1f * meas_fast.grid_v;
    pll_step((PLL *)&grid_pll);
    ctrl_globals.pll_sin = grid_pll.s;
    ctrl_globals.pll_cos = grid_pll.c;
}

#pragma CODE_SECTION(q_loop_run, "ramfuncs");
/* Feeds the cycle-averaged reactive current loop: the reference is task 4's reactive current
   q_cmd (0xC408, 0.1 A peak), the measurement the inverter current times the PLL cosine, and the
   PLL's sample index n tells the loop where in the cycle it is. */
void q_loop_run(void) {
    q_loop.in = grid_ictl_blk.q_cmd;
    q_loop.i_in = meas_fast.inv_i;
    q_loop.cos_in = ctrl_globals.pll_cos;
    q_loop.n = grid_pll.n;
    q_loop_step((q_loop_t *)&q_loop);
}
