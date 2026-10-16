// clang-format off
/* The ADC readings, run from RAM at the start of every control interrupt (19.2 kHz).
   Entry point: adc_scale, called by control_isr (converter_ctrl.c) after ADCINT1.
   Data: none of its own. It reads the DMA buffer adc_dma_buf (0xDB86: 16 channels, two
   conversions each) and the offsets measured at start-up (eps_cycle page: pv1/pv2/inv/dci/hv
   current offsets), and writes the raw channels (adc_ch, 0xC1C0), the scaled values (meas_fast,
   0xC0C0) and the grid voltage history (grid_v_hist, 0xC080).
   Scales (per ADC count): AC voltages (grid, inverter, EPS) 2.214 -> 0.1 V about mid-scale 2048;
   inverter current 0.2096 -> 0.1 A; DC voltages (PV, bus, high side) 1.480 -> 0.1 V; battery
   0.1792 -> 0.1 V; PV currents 0.8076 -> 0.01 A; high-side current 1.177 -> 0.01 A (inference from
   its use); residual current 0.484 -> mA about 2075; DC injection 0.889; insulation 0.186. The
   current units are read from the limits the code applies (docs/subsystems/meas.md).
   Neighbours: meas_avg8.c (the next step), dc_avg.c, grid_rms_accum.c, converter_ctrl.c.
   name: descriptive, name kept (adc_scale.c; map/filenames.tsv).
   at: adc_scale 0x3F2584 (load address; it runs at 0x89CD) */
// clang-format on
#include "F2806x_Device.h"
#include "meas.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

#pragma CODE_SECTION(adc_scale, "ramfuncs");
/* Each channel is the mean of its two conversions less its offset, scaled to a float in
   meas_fast. Then the grid voltage lead term: lead = (v0 + v2) x 1.5625 - v1 x 3.125 (a second
   difference of the last three samples), filtered 0.6/0.4 and clamped to +-100 (10.0 V);
   grid_ictl_run adds it to the voltage feed-forward. Zero or negative PV currents read as 1. */
void adc_scale(void) {
    float lead_sample;

    adc_ch.pv1_v_raw.v = (Uint16)(adc_dma_buf[0] + adc_dma_buf[16]) >> 1;
    adc_ch.pv2_v_raw.v = (Uint16)(adc_dma_buf[1] + adc_dma_buf[17]) >> 1;
    adc_ch.pv1_i_raw.v = ((Uint16)(adc_dma_buf[2] + adc_dma_buf[18]) >> 1) - eps_cycle.pv1_i_ofs;
    adc_ch.pv2_i_raw.v = ((Uint16)(adc_dma_buf[3] + adc_dma_buf[19]) >> 1) - eps_cycle.pv2_i_ofs;
    adc_ch.iso_raw.v = (Uint16)(adc_dma_buf[4] + adc_dma_buf[20]) >> 1;
    adc_ch.bus_v_raw.v = (Uint16)(adc_dma_buf[5] + adc_dma_buf[21]) >> 1;
    adc_ch.inv_v_raw.v = ((Uint16)(adc_dma_buf[6] + adc_dma_buf[22]) >> 1) - 2048;
    adc_ch.inv_i_raw.v = ((Uint16)(adc_dma_buf[7] + adc_dma_buf[23]) >> 1) - eps_cycle.inv_i_ofs;
    adc_ch.dci_i_raw.v = ((Uint16)(adc_dma_buf[8] + adc_dma_buf[24]) >> 1) - eps_cycle.dci_ofs;
    adc_ch.leak_i_raw.v = ((Uint16)(adc_dma_buf[9] + adc_dma_buf[25]) >> 1) - 2075;
    adc_ch.grid_v_raw.v = ((Uint16)(adc_dma_buf[10] + adc_dma_buf[26]) >> 1) - 2048;
    adc_ch.eps_v_raw.v = ((Uint16)(adc_dma_buf[11] + adc_dma_buf[27]) >> 1) - 2048;
    adc_ch.hv_v_raw.v = (Uint16)(adc_dma_buf[12] + adc_dma_buf[28]) >> 1;
    adc_ch.spare_ac_raw.v = ((Uint16)(adc_dma_buf[13] + adc_dma_buf[29]) >> 1) - 2048;
    adc_ch.batt_v_raw.v = (Uint16)(adc_dma_buf[14] + adc_dma_buf[30]) >> 1;
    adc_ch.hv_i_raw.v = ((Uint16)(adc_dma_buf[15] + adc_dma_buf[31]) >> 1) - eps_cycle.hv_i_ofs;

    meas_fast.grid_v = adc_ch.grid_v_raw.v * 2.2138671875f;
    meas_fast.inv_v = adc_ch.inv_v_raw.v * 2.2138671875f;
    meas_fast.eps_v = adc_ch.eps_v_raw.v * 2.2138671875f;
    meas_fast.inv_i = adc_ch.inv_i_raw.v * 0.209570318f;
    meas_fast.dci_i = adc_ch.dci_i_raw.v * 0.888598979f;
    meas_fast.leak_i = adc_ch.leak_i_raw.v * 0.484375f;
    meas_fast.pv1_v = adc_ch.pv1_v_raw.v * 1.48046875f;
    meas_fast.pv2_v = adc_ch.pv2_v_raw.v * 1.48046875f;
    meas_fast.bus_v = adc_ch.bus_v_raw.v * 1.48046875f;
    meas_fast.hv_v = adc_ch.hv_v_raw.v * 1.48046875f;
    meas_fast.batt_v = adc_ch.batt_v_raw.v * 0.17919921875f;
    meas_fast.pv1_i = adc_ch.pv1_i_raw.v * 0.8076171875f;
    meas_fast.pv2_i = adc_ch.pv2_i_raw.v * 0.8076171875f;
    meas_fast.hv_i = adc_ch.hv_i_raw.v * 1.1767578125f;
    meas_fast.leak_i_slow = adc_ch.leak_i_raw.v * 0.484375f;
    meas_fast.iso_v = adc_ch.iso_raw.v * 0.185794801f;
    meas_fast.aux17 = (Uint16)adc_ch.aux17_raw.v;
    meas_fast.aux18 = (Uint16)adc_ch.aux18_raw.v;

    grid_v_hist.v2 = grid_v_hist.v1;
    grid_v_hist.v1 = grid_v_hist.v0;
    grid_v_hist.v0 = meas_fast.grid_v;
    grid_v_hist.lead_prev = grid_v_hist.lead;
    grid_v_hist.lead = (grid_v_hist.v0 + grid_v_hist.v2) * 1.5625f - grid_v_hist.v1 * 3.125f;
    grid_v_hist.lead = grid_v_hist.lead_prev * 0.6f + grid_v_hist.lead * 0.4f;
    {
        lead_sample = grid_v_hist.lead;
        grid_v_hist.lead = lead_sample > 100.0f ? 100.0f : __fmax(lead_sample, -100.0f);
    }
    if (meas_fast.pv1_i <= 0.0f)
        meas_fast.pv1_i = 1.0f;
    if (meas_fast.pv2_i <= 0.0f)
        meas_fast.pv2_i = 1.0f;
}
