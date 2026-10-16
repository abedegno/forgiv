// clang-format off
/* The grid-side RMS sums, run from RAM every control interrupt: squares and products of the
   measured currents and voltages added into the block at 0xD380 (ac_sums_isr). grid_period_isr
   hands them to task 1 once per grid period.
   Entry point: grid_rms_accum, called by control_isr.
   Data: none of its own.
   Neighbours: converter_ctrl.c (grid_period_isr), ac_measure.c (task 1).
   name: descriptive (was rms_sum.c; map/filenames.tsv).
   at: grid_rms_accum 0x3F2783 (load address; it runs at 0x8BCC) */
// clang-format on
#include "F2806x_Device.h"
#include "faults.h"
#include "control.h"
#include "mppt.h"
#include "rating.h"
#include "util.h"

#pragma CODE_SECTION(grid_rms_accum, "ramfuncs");
/* While the grid is present (state_flags bit 8 clear): adds the inverter voltage squared, the
   inverter current squared, grid v x i (active power), the grid voltage squared, the residual
   current squared and the DC injection sample, and counts the sample. With the grid lost the sums
   are cleared (the DC sum is kept). */
void grid_rms_accum(void) {
    if (!state_flags.bit.b8) {
        ac_sums_isr.inv_v_sq = meas_fast.inv_v * meas_fast.inv_v + ac_sums_isr.inv_v_sq;
        ac_sums_isr.grid_i_sq = meas_fast.inv_i * meas_fast.inv_i + ac_sums_isr.grid_i_sq;
        ac_sums_isr.grid_p = meas_fast.grid_v * meas_fast.inv_i + ac_sums_isr.grid_p;
        ac_sums_isr.grid_v_sq = meas_fast.grid_v * meas_fast.grid_v + ac_sums_isr.grid_v_sq;
        ac_sums_isr.leak_i_sq = meas_fast.leak_i * meas_fast.leak_i + ac_sums_isr.leak_i_sq;
        ac_sums_isr.dc_sum = ac_sums_isr.dc_sum + meas_fast.dci_i;
        ac_sums_isr.n_grid++;
    } else {
        ac_sums_isr.grid_v_sq = 0;
        ac_sums_isr.grid_p = 0;
        ac_sums_isr.inv_v_sq = 0;
        ac_sums_isr.n_grid = 0;
        ac_sums_isr.leak_i_sq = 0;
    }
}
