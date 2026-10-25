// clang-format off
/* Settings with their defaults (settings_defaults) and the rating preset and start delay
   (rating_apply): the words at
   0xD801 .. 0xD83F are this file's own variables (one data page; 0xD834 and 0xD838 are not used
   anywhere); 0xD840 .. are other files'. at: settings_defaults 0x3EF706, rating_apply 0x3EF7E2

   Job: the boot defaults of the grid protection, calibration gain and power quality settings that
   the ARM later overwrites (arm_link.c, arm_page_write.c), and rating_apply, which picks the rating
   preset and the start delay. Entry points: settings_defaults (app_init), rating_apply
   (state0_idle, every pass with a run mode).
   Data it owns: 0xD801 .. 0xD83F below. Units: grid voltages 0.1 V, frequencies 0.01 Hz,
   calibration gains 1024 = 1.0, times in the units the grid code uses (see grid_support.c).
   Neighbours: rating_presets.c, arm_link.c, grid_support.c, dc_measure.c (the gains).
   name: descriptive, kept from the matching (map/filenames.tsv: src/settings.c). */
// clang-format on
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#include "F2806x_Device.h"
#include "util.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
extern struct fault_words_t fault_words; /* match: own form, see OWN_fault_words */

// clang-format off
// ebss-begin
Uint16 ofr_recover_delay;
Uint16 cfg_unused_b;
Uint16 ofr_droop;
Uint16 ofr_delay;
Uint16 ufr_droop;
Uint16 ufr_delay;
Uint16 ofr_recover_freq;
Uint16 ufr_start_freq;
Uint16 ofr_start_freq;
Uint16 f_ac_low_out_time;
Uint16 f_ac_high_in_time;
Uint16 f_ac_high_out_time;
Uint16 v_ac_high_in_time;
Uint16 v_ac_low_in_time;
Uint16 dci_trip_time;
Uint16 dci_trip_time_it;
Uint16 cfg_unused_a;
Uint16 f_ac_low_in_time;
Uint16 cfg_spare_150;
Uint16 cal_spare_16;
Uint16 cal_pv2_p;
Uint16 cal_grid_v;
Uint16 cal_pv1_p;
Uint16 cal_pv2_v;
Uint16 cal_pv2_i;
Uint16 cal_spare_14;
Uint16 cal_spare_15;
Uint16 cal_power;
Uint16 cal_spare_11;
Uint16 cal_spare_12;
Uint16 cal_pv1_i;
Uint16 pv_mode_cfg;
Uint16 pv_bus_mode;     /* name: only ever set to 0; non-zero would run the PV boosts on the bus voltage (converter_ctrl.c) */
Uint16 pv_start_voltage;
Uint16 cfg_flag_a;
Uint16 cfg_flag_b;
Uint16 cal_hv_i;
Uint16 cal_pv1_v;
Uint16 cal_batt_v;
Uint16 qu_q_max;
Uint16 q_mode;
Uint16 v_ac_low_out_time;
Uint16 v_ac_low_c;
Uint16 f_ac_high_c;
Uint16 v_ac_high_c;
Uint16 f_ac_high_in;
Uint16 f_ac_low_in;
Uint16 dci_limit_b;
Uint16 v_ac_high_out_time;
Uint16 dci_limit;
Uint16 f_ac_low_c;
Uint16 v_10_min_protection;
Uint16 f_ac_low_out;
Uint16 inverter_start_time;
Uint16 inverter_restart_delay;
Uint16 cal_d819;
Uint16 cal_spare_17;
Uint16 cal_bus_v;
Uint16 v_ac_low_in;
Uint16 f_ac_high_out;
Uint16 v_ac_high_in;
Uint16 v_ac_high_out;
Uint16 v_ac_low_out;
// ebss-end
// clang-format on

/* Boot defaults: every calibration gain 1024 (1.0), grid limits (for example v_ac_high_out
   2620 = 262.0 V, f_ac_high_out 5151 = 51.51 Hz), PF and Q(U) curves, the DC injection limit
   (1100), the power ramp rates (1000), the PV start voltage (1200 = 120.0 V). */
void settings_defaults(void) {
    pv_start_voltage = 0x4b0;
    ofr_it_param = 0x18;
    cal_batt_v = 0x400;
    active_power_rate = 100;
    cal_hv_i = 0x400;
    reactive_power_rate = 100;
    cal_pv1_v = 0x400;
    pf_curve_p1 = 0xff;
    cal_pv1_i = 0x400;
    pf_curve_pf1 = 20000;
    cal_pv2_v = 0x400;
    cal_pv2_i = 0x400;
    cal_pv1_p = 0x400;
    pf_curve_p2 = 0xff;
    cal_pv2_p = 0x400;
    cal_grid_v = 0x400;
    cal_spare_11 = 0x400;
    pf_curve_pf2 = 20000;
    v_10_min_protection = 0x9d8;
    cal_spare_12 = 0x400;
    cal_power = 0x400;
    pf_curve_p3 = 0xff;
    cal_d819 = 0x400;
    cal_spare_14 = 0x400;
    cal_spare_15 = 0x400;
    pf_curve_pf3 = 20000;
    dci_limit = 0x44c;
    cal_spare_16 = 0x400;
    cal_spare_17 = 0x400;
    pf_curve_p4 = 0xff;
    cal_bus_v = 0x400;
    v_ac_high_out = 0xa3c;
    v_ac_low_out = 0x744;
    pf_curve_pf4 = 20000;
    v_ac_high_in = 0xa50;
    v_ac_low_in = 0x73a;
    f_ac_high_out = 0x141f;
    qu_v_high1 = 0x9b4;
    f_ac_low_out = 0x128e;
    f_ac_high_in = 0x1450;
    f_ac_low_in = 0x1194;
    qu_v_high2 = 0x9e2;
    v_ac_high_out_time = 0x32;
    v_ac_low_out_time = 0x32;
    v_ac_high_in_time = 5;
    qu_v_low1 = 0x844;
    v_ac_low_in_time = 5;
    f_ac_high_out_time = 0x32;
    f_ac_low_out_time = 0x32;
    qu_v_low2 = 0x816;
    f_ac_high_in_time = 5;
    f_ac_low_in_time = 5;
    cfg_spare_150 = 0x96;
    power_ramp_rate_reconnect = 1000;
    dci_trip_time = 0x96;
    power_ramp_rate = 1000;
    pv_mode_cfg = 0;
    fast_trip_v_low_set = 0x47e;
    ofr_delay = 10;
    qu_q_max = 0x1b6;
    ofr_recover_delay = 0;
    fast_trip_v_high_set = 0xac8;
    ofr_recover_freq = 0x1392;
    q_pt1_tau = 0x96;
}

/* Flag an unknown model (preset_index 0: CF17 bit 10, a stop fault), hold the index to 1..6 and
   run its preset; so index 0 runs preset_2kw and 6 runs preset_none. connect_countdown =
   inverter_start_time x 50 (1 s = 50 passes), or inverter_restart_delay x 50 after a grid
   voltage or frequency trip (run_status bit 2) unless pv_bus_mode; 1500 in the auto test; held to
   1500 .. 30000 (30 s .. 10 min). */
void rating_apply(void) {
    if (dsp_settings.preset_index == 0) {
        fault_words.fault_stop1 |= FS1_BAD_PRESET;
    } else {
        fault_words.fault_stop1 &= 0xFBFF; /* ~FS1_BAD_PRESET, as the image spells it */
    }
    limitu(&dsp_settings.preset_index, 6, 1);
    if (!eps_cfg_bits.run_status.bit.b2 || pv_bus_mode != 0) {
        dsp_settings.connect_countdown = inverter_start_time * 50;
    } else {
        dsp_settings.connect_countdown = inverter_restart_delay * 50;
        if (auto_test_state != 0)
            dsp_settings.connect_countdown = 1500;
    }
    limitu(&dsp_settings.connect_countdown, 30000, 1500);
    rating_presets[dsp_settings.preset_index]();
}
