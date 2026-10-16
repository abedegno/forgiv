// clang-format off
/* Eight-sample averages of the DC measurements and the bus voltage notch filter, run from RAM
   every control interrupt. It also steps ctrl_slot (0xC83A), the eight-slot schedule of the slow
   loops (docs/subsystems/control.md).
   Entry point: meas_avg8, called by control_isr after adc_scale.
   Data: none of its own; meas_avg (0xC140) and ctrl_slot.
   Neighbours: converter_ctrl.c, current_ctrl.c (duty_ff), adc_scale.c.
   name: descriptive (was avg8.c; map/filenames.tsv).
   at: meas_avg8 0x3F2C31 (load address; it runs at 0x907A) */
// clang-format on
#include "F2806x_Device.h"
#include "settings.h"
#include "meas.h"
#include "control.h"
#include "mppt.h"
#include "util.h"

#pragma CODE_SECTION(meas_avg8, "ramfuncs");
/* Every sample: adds PV1/PV2 voltage, bus, high side, battery and PV currents to the sums and
   steps ctrl_slot. At slot 8 (every 8th sample, 2.4 kHz) ctrl_slot returns to 0 and the means are
   taken (PV and bus voltages with their gain words pv1_v_k, pv2_v_k, bus_v_k), and the bus mean
   goes through a second-order notch at twice the grid frequency (b 0.795, -1.537, 0.795 and a
   1.537, -0.591 at 50 Hz, a 99 Hz notch at 2.4 kHz; 119 Hz with the 60 Hz set), so bus_v_filt
   has the 100/120 Hz ripple removed. Finally the battery converter's feed-forward duty_ff =
   pwm_period x hv_v / bus_v_filt. */
void meas_avg8(void) {
    float t;

    meas_avg.pv1_v_acc = meas_avg.pv1_v_acc + meas_fast.pv1_v;
    meas_avg.pv2_v_acc = meas_avg.pv2_v_acc + meas_fast.pv2_v;
    meas_avg.bus_v_acc = meas_avg.bus_v_acc + meas_fast.bus_v;
    meas_avg.hv_v_acc = meas_avg.hv_v_acc + meas_fast.hv_v;
    meas_avg.batt_v_acc = meas_avg.batt_v_acc + meas_fast.batt_v;
    meas_avg.pv1_i_acc = meas_avg.pv1_i_acc + meas_fast.pv1_i;
    meas_avg.pv2_i_acc = meas_avg.pv2_i_acc + meas_fast.pv2_i;
    if (++grid_rep_tail.ctrl_slot >= 8) {
        meas_avg.bus_v_z2 = meas_avg.bus_v_z1;
        meas_avg.bus_v_z1 = meas_avg.bus_v;
        meas_avg.pv1_v = meas_avg.pv1_v_acc * 0.125f * meas_avg.pv1_v_k;
        meas_avg.pv2_v = meas_avg.pv2_v_acc * 0.125f * meas_avg.pv2_v_k;
        meas_avg.bus_v = meas_avg.bus_v_acc * 0.125f * meas_avg.bus_v_k;
        grid_rep_tail.ctrl_slot = 0;
        meas_avg.hv_v = meas_avg.hv_v_acc * 0.125f;
        meas_avg.pv1_i = meas_avg.pv1_i_acc * 0.125f;
        meas_avg.pv2_i = meas_avg.pv2_i_acc * 0.125f;
        meas_avg.batt_v = meas_avg.batt_v_acc * 0.125f;
        meas_avg.pv1_v_acc = 0;
        meas_avg.pv2_v_acc = 0;
        meas_avg.bus_v_acc = 0;
        meas_avg.hv_v_acc = 0;
        meas_avg.pv1_i_acc = 0;
        meas_avg.pv2_i_acc = 0;
        meas_avg.batt_v_acc = 0;
        if (dsp_settings.grid_60hz == 1)
            meas_avg.bus_v_filt =
                meas_avg.bus_v_filt * 1.45703125f - meas_avg.bus_v_filt_z1 * 0.53076171875f +
                meas_avg.bus_v * 0.765380859375f - meas_avg.bus_v_z1 * 1.45703125f +
                meas_avg.bus_v_z2 * 0.765380859375f;
        else
            meas_avg.bus_v_filt =
                meas_avg.bus_v_filt * 1.537109375f - meas_avg.bus_v_filt_z1 * 0.590576171875f +
                meas_avg.bus_v * 0.79541015625f - meas_avg.bus_v_z1 * 1.537109375f +
                meas_avg.bus_v_z2 * 0.79541015625f;
        meas_avg.bus_v_filt_z2 = meas_avg.bus_v_filt_z1;
        meas_avg.bus_v_filt_z1 = meas_avg.bus_v_filt;
        if (meas_avg.bus_v_filt > 0.0f)
            dcdc_ref.duty_ff = pwm_period * meas_avg.hv_v / meas_avg.bus_v_filt;
    }
}
