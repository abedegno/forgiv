/* The settings the DSP keeps in RAM: the settings page 0xD8C0 .. 0xD8FF (dsp_settings), which
   the ARM link pages and the BMS link write. The code reaches several of its words with one
   data page load from several files, so it is one object.
   The HR numbers are the inverter's holding registers that the ARM copies into its frames
   (docs/names/B-notes.md); "BMS HRn" is a battery register read by bms_link.c. */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "F2806x_Device.h"
#include "util.h"

struct dsp_settings_t {
    float inv_power_abs;             /* 0xD8C0 */
    float bus_ov_limit;              /* 0xD8C2 */
    Uint16 cfg_d8c4;                 /* 0xD8C4: set to 10 by the presets */
    Uint16 batt_charge_limit_pct;    /* 0xD8C5: HR111 battery_charge_limit */
    Uint16 batt_discharge_limit_pct; /* 0xD8C6: HR112 battery_discharge_limit */
    Uint16 charge_target_soc_now;    /* 0xD8C7: forced charge stops here (ARM page 7, 20..100) */
    Uint16 soc_reserve;       /* 0xD8C8: HR114, the SoC floor; raised to HR110 in modes 5/6 */
    Uint16 discharge_min_soc; /* 0xD8C9: SoC floor, HR110 (1..100, default 4) */
    Uint16 grid_import_amps;  /* 0xD8CA */
    Uint16 connect_countdown; /* 0xD8CB */
    Uint16 cfg_changed_flags; /* 0xD8CC */
    Uint16 inverter_enable;   /* 0xD8CD */
    Uint16 batt_force_mode;   /* 0xD8CE: 0 ARM setpoint, 1 forced charge, 2 forced discharge */
    Uint16 start_fault_now;   /* 0xD8CF */
    Uint16 start_fault_entry; /* 0xD8D0 */
    Uint16 charge_active_setting;
    Uint16 discharge_active_setting;
    Uint16 batt_power_mode; /* 0xD8D3: HR27 battery_power_mode; 0 = no power in mode 0 */
    Uint16 cfg_d8d4;
    Uint16 iso_check_enable; /* 0xD8D5 */
    Uint16 rcmu_enable;      /* 0xD8D6 */
    Uint16 cfg_d8d7;
    Uint16 module_bit2;         /* 0xD8D8 */
    Uint16 relay_test_enable;   /* 0xD8D9 */
    Uint16 rcmu_step_check_off; /* 0xD8DA */
    Uint16 cfg_d8db;
    Uint16 cfg_d8dc;
    Uint16 cfg_d8dd;
    Uint16 batt_max_voltage; /* 0xD8DE: battery maximum 0.1 V (HR98) */
    Uint16 batt_min_voltage; /* 0xD8DF: battery minimum 0.1 V (HR97) */
    Uint16 batt_capacity_ah; /* 0xD8E0: HR55 capacity, Ah (BMS HR11 after 50 replies) */
    Uint16 battery_type;     /* 0xD8E1: HR54; 0 = no BMS, else BMS mode */
    Uint16
        bms_full_poll; /* 0xD8E2: full (1) or short BMS poll; HR109 enable_bms_read via the ARM link */
    Uint16 bms_hr0;    /* 0xD8E3: BMS HR0 (full poll) */
    Uint16 bms_fw_version; /* 0xD8E4: BMS HR13; above 3010: HR26/HR27 limits */
    Uint16
        bms_hr14; /* 0xD8E5: BMS HR14; 1 lets the ARM start the battery firmware relay (arm_link.c bms_relay_request) */
    Uint16 cfg_d8e6;
    Uint16 bms_capacity;      /* 0xD8E7: BMS HR11, at most 10000 */
    Uint16 bms_soc_raw;       /* 0xD8E8: BMS HR21 low byte, unclamped */
    Uint16 pv_input_mode;     /* 0xD8E9 */
    Uint16 run_mode_req;      /* 0xD8EA: the requested run mode 0..8 (op_states.c) */
    Uint16 arm_run_mode;      /* 0xD8EB */
    Uint16 arm_page_no;       /* 0xD8EC */
    Uint16 grid_60hz;         /* 0xD8ED */
    Uint16 relay_test_pass;   /* 0xD8EE */
    Uint16 aging_test_enable; /* 0xD8EF: HR332 */
    Uint16 aging_test_mode;   /* 0xD8F0: 1 charge, 2 discharge (aging_test_update) */
    Uint16 preset_index;      /* 0xD8F1 */
    float batt_power_target; /* D8F2: 319 battery power target used by import limiting. */
    Uint16 cfg_d8f4[12];      /* 0xD8F2 .. 0xD8FF */
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_Device.h"

/* rating_presets.c */
void init_defaults(void);
extern void (*const rating_presets[9])(void);

/* settings.c */
extern Uint16 cal_batt_v;
extern Uint16 cal_bus_v;
extern Uint16 cal_d819;
extern Uint16 cal_grid_v;
extern Uint16 cal_power;
extern Uint16 ofr_delay;
extern Uint16 ofr_recover_delay;
extern Uint16
    pv_bus_mode; /* name: only ever set to 0; non-zero would run the PV boosts on the bus voltage (converter_ctrl.c) */
extern Uint16 pv_mode_cfg;
extern Uint16 pv_start_voltage;
extern Uint16 q_mode;
void rating_apply(void);
void settings_defaults(void);
extern Uint16 ufr_delay;

/* defined by no source: the linker places it at its map address */
extern Uint16 active_power_rate;
extern Uint16 auto_test_state;
#ifndef OWN_dsp_settings
extern struct dsp_settings_t dsp_settings;
#endif
extern Uint16 ofr_it_param;
extern Uint16 pf_curve_p1;
extern Uint16 pf_curve_p2;
extern Uint16 pf_curve_p3;
extern Uint16 pf_curve_p4;
extern Uint16 pf_curve_pf1;
extern Uint16 pf_curve_pf2;
extern Uint16 pf_curve_pf3;
extern Uint16 pf_curve_pf4;
extern Uint16 qu_v_high1;
extern Uint16 qu_v_high2;
extern Uint16 qu_v_low1;
extern Uint16 qu_v_low2;
extern Uint16 reactive_power_rate;
/* ---- end of declarations ---- */

#endif
