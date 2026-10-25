// clang-format off
/* The five rating presets (picked through a table of pointers): grid limits, PLL and
   protection settings in the block at 0xD880. The block runs on past 0xD8C0 (0xD8C2 is a member
   of the same object here: with dsp_settings as another object the scheduler orders the stores
   differently). at: preset_5kw 0x3EF826, preset_4k6w 0x3EF8C2, preset_3k6w 0x3EF95E, preset_3kw
   0x3EF9FA, preset_2kw 0x3EFA96; init_defaults 0x3EFB33; rating_presets 0x3F3EE2

   Job: each preset sets the AC rating of one model (grid power 5.0, 4.6, 3.6, 3.0 or 2.0 kW, in
   0.1 W), its grid current, the PV rating and the temperature and bus limits; init_defaults sets
   the boot values of the rating block and the battery settings (HR55 capacity 120, HR111/HR112 50
   %, HR97/HR98 42.0/56.0 V, HR110 4 %, the charge target 100 %, battery rating 3.6 kW) before the
   ARM's first frames replace them. Entry points: init_defaults (app_init), rating_presets[]
   (rating_apply, settings.c). Data: none of its own; it writes rating_params (0xD880 .. 0xD8BF) and
   dsp_settings (0xD8C0 ..). Units: power 0.1 W, voltage 0.1 V, temperature 0.1 degree, frequency
   0.01 Hz. Neighbours: settings.c, arm_page_write.c (HR308 replaces the battery rating),
   grid_support.c. name: descriptive (map/filenames.tsv: src/presets.c). */
// clang-format on
#define OWN_rating_params /* match: this file's own view of 0xD880 (the presets store into its fields); the header has another */
#include "F2806x_Device.h"
#include "settings.h"
#include "faults.h"
#include "control.h"
#include "state.h"
#include "mppt.h"
#include "util.h"

extern struct {
    float rated_power;
    float grid_v_nominal;
    float grid_f_nominal;
    float grid_omega_step;
    float rated_current;
    float grid_power_max;
    float grid_power_max_pv;
    float grid_v_ref;
    float grid_current_max;
    float batt_chg_power_max;
    float batt_dis_power_max;
    float xD896;
    float xD898;
    float xD89A;
    float pv_power_rated;
    float pv_power_limit;
    float spare_v340;
    float pv_derate_voltage;
    float grid_v_87pct;
    float grid_v_90pct;
    float pv_ov_limit;
    float batt_temp_limit;
    float batt_temp_clear;
    float charger_temp_limit;
    float charger_temp_derate;
    float heatsink_temp_limit;
    float heatsink_temp_clear;
    float vw_v_ref;
    float xD8B8, xD8BA, xD8BC, xD8BE;
    struct dsp_settings_t settings;
} rating_params;

/* 5.0 kW grid rating (3.6 kW rated power, 7.5 kW PV); every preset sets the same temperature
   limits: heatsink and charger 93.0 (clear or derate from 87.0), battery 100.0 (clear 95.0). */
void preset_5kw(void) {
    rating_params.grid_power_max = 50000.0f;
    rating_params.grid_power_max_pv = 50000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 36000.0f;
    rating_params.rated_current = rating_params.rated_power * 10.0f / 2300.0f;
    rating_params.grid_current_max = 500000.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.pv_power_rated = 7500000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
}

/* 4.6 kW grid rating (3.6 kW rated power, 6.9 kW PV). */
void preset_4k6w(void) {
    rating_params.grid_power_max = 46000.0f;
    rating_params.grid_power_max_pv = 46000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 36000.0f;
    rating_params.rated_current = rating_params.rated_power * 10.0f / 2300.0f;
    rating_params.grid_current_max = 460000.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.pv_power_rated = 6900000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
}

/* 3.6 kW grid rating (3.6 kW rated power, 5.4 kW PV). */
void preset_3k6w(void) {
    rating_params.grid_power_max = 36000.0f;
    rating_params.grid_power_max_pv = 36000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 36000.0f;
    rating_params.rated_current = rating_params.rated_power * 10.0f / 2300.0f;
    rating_params.grid_current_max = 360000.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.pv_power_rated = 5400000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
}

/* 3.0 kW grid rating (2.5 kW rated power, 4.6 kW PV). */
void preset_3kw(void) {
    rating_params.grid_power_max = 30000.0f;
    rating_params.grid_power_max_pv = 30000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 25000.0f;
    rating_params.rated_current = rating_params.rated_power * 10.0f / 2300.0f;
    rating_params.grid_current_max = 300000.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.pv_power_rated = 4600000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
}

/* 2.0 kW grid rating (2.0 kW rated power, 3.0 kW PV); also what an unknown model (index 0) gets. */
void preset_2kw(void) {
    rating_params.grid_power_max = 20000.0f;
    rating_params.grid_power_max_pv = 20000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 20000.0f;
    rating_params.rated_current = rating_params.rated_power * 10.0f / 2300.0f;
    rating_params.grid_current_max = 200000.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.pv_power_rated = 3000000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
}

/* Boot defaults of the rating block and the battery settings, called once by app_init: 5.0 kW
   grid, 230.0 V 50.00 Hz, HR55 capacity 120, HR111 and HR112 50 %, HR97 42.0 V and HR98 56.0 V,
   HR110 floor 4 %, charge target 100 %, battery charge and discharge rating 3.6 kW, mode 0. */
void init_defaults(void) {
    rating_params.grid_power_max = 50000.0f;
    rating_params.grid_power_max_pv = 50000.0f;
    rating_params.grid_v_ref = 2300.0f;
    rating_params.rated_power = 36000.0f;
    rating_params.grid_f_nominal = 5000.0f;
    rating_params.grid_omega_step = 6.2831855f * rating_params.grid_f_nominal / 19200.0f / 100.0f;
    rating_params.grid_v_nominal = 2300.0f;
    rating_params.grid_v_90pct = 0.9f * rating_params.grid_v_nominal;
    rating_params.grid_v_87pct = 0.87f * rating_params.grid_v_nominal;
    rating_params.rated_current = rating_params.rated_power * 10.0f / rating_params.grid_v_nominal;
    rating_params.pv_power_rated = 7500000.0f;
    rating_params.pv_power_limit = rating_params.pv_power_rated;
    rating_params.spare_v340 = 3400.0f;
    rating_params.pv_ov_limit = 5450.0f;
    rating_params.pv_derate_voltage = 4800.0f;
    rating_params.settings.bus_ov_limit = 5850.0f;
    rating_params.grid_current_max =
        rating_params.grid_power_max * 10.0f / rating_params.grid_v_ref;
    pv_phase[0].i_max = 1500.0f;
    pv_phase[1].i_max = 1500.0f;
    rating_params.heatsink_temp_limit = 930.0f;
    rating_params.heatsink_temp_clear = 870.0f;
    rating_params.charger_temp_limit = 930.0f;
    rating_params.charger_temp_derate = 870.0f;
    rating_params.batt_temp_limit = 1000.0f;
    rating_params.batt_temp_clear = 950.0f;
    rating_params.vw_v_ref = 2200.0f;
    rating_params.settings.grid_60hz = 0;
    rating_params.settings.batt_capacity_ah = 120;
    rating_params.settings.charge_target_soc_now = 100;
    rating_params.settings.batt_min_voltage = 420;
    rating_params.settings.discharge_min_soc = 4;
    rating_params.settings.batt_discharge_limit_pct = 50;
    rating_params.settings.batt_charge_limit_pct = 50;
    rating_params.batt_chg_power_max = 36000.0f;
    rating_params.settings.batt_max_voltage = 560;
    rating_params.settings.cfg_d8c4 = 10;
    eps_cfg_bits.eps_cfg.bit.b9 = 1;
    rating_params.batt_dis_power_max = 36000.0f;
    rating_params.settings.batt_force_mode = 0;
    grid_freq_setup(rating_params.settings.grid_60hz);
}

/* the presets by the setting at 0xD8F1 (rating_apply in settings.c calls through this table; it
 * limits the setting to 1 .. 6, so 0 and 7 .. 8 are never used and 6 does nothing) */

void (*const rating_presets[9])(void) = {preset_none, preset_2kw,  preset_3kw,
                                         preset_3k6w, preset_4k6w, preset_5kw,
                                         preset_none, preset_none, preset_none};
