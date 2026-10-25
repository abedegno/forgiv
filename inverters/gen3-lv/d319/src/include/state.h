/* The operating state machine (task 0) and the application start-up.
   op_states.c holds the states and the fault queries, state_task.c the task, app_init.c the
   start-up; the rating block rating_params (0xD880) is declared here because the states use it. */
#ifndef STATE_H
#define STATE_H

#include "F2806x_Device.h"
#include "util.h"

/* 0xCF1A: precharge_step (pv_boost_precharge), grid_start_done, lim_soft_start (0xCF1C, the
   soft-start power ramp of grid_support.c, 0.1 W) and the grid code's other power ceilings */
struct power_limits_t {
    unsigned int precharge_step, grid_start_done;
    float lim_soft_start, plim_1E, plim_20, plim_22, plim_24, plim_26, plim_28, plim_2A,
        out_power_limit, plim_2E, plim_30, plim_32;
};

/* The operating states (op_state, 0xCF0D): task0_state_machine runs one function per state */
#define OP_IDLE 0      /* state0_idle (and any unknown state) */
#define OP_START 1     /* state1_start: the start-up checks */
#define OP_FAULT 3     /* state3_fault */
#define OP_UPDATE 4    /* state4_update: waiting for the boot loader's update */
#define OP_GRID_PV 5   /* on grid with PV */
#define OP_GRID_BATT 6 /* on grid without PV */
#define OP_EPS_PV 7    /* EPS (off grid) with PV */
#define OP_EPS_BATT 8  /* EPS without PV */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_Device.h"
#include "util.h"

/* app_init.c */
void app_init(void);
void app_vars_init(void);
void arm_run_mode_set(Uint16 v);
void inv_off(void);
void preset_none(void);

/* op_states.c */
extern unsigned int board_id;
void converters_stop(void);
unsigned int grid_in_range(void);
unsigned int has_eps_fault(void);
unsigned int has_stop_fault(void);
extern unsigned int op_state;
#ifndef OWN_power_limits
extern struct power_limits_t power_limits;
#endif
extern unsigned int precharge_duty;
void pv_boost_precharge(void);
void relay_test_abort(void);
void set_grid_fault_flag(unsigned int on);
void set_pv_fault_flag(unsigned int on);
unsigned int soft_start(void);
extern unsigned int soft_start_duty;
extern unsigned int soft_start_state;
extern unsigned int soft_start_step;
extern unsigned int soft_start_timeout;
extern unsigned int standby_count;
unsigned int start_eps_batt(void);
unsigned int start_eps_pv(void);
unsigned int start_grid_batt(void);
unsigned int start_grid_pv(void);
extern unsigned int start_wait_count;
void state0_idle(void);
void state1_start(void);
void state3_fault(void);
void state4_update(void);
void state5_grid_pv(void);
void state6_grid_batt(void);
void state7_eps_pv(void);
void state8_eps_batt(void);
void state_init(void);

/* defined by no source: the linker places it at its map address */
extern union bits16_u bms_status2; /* 0xD502: never written */
extern float bus_v_ref_prev;
extern struct cfg_d800_t {
    unsigned int w0;
    unsigned int w1;
    unsigned int w2;
    unsigned int pv_start_voltage;
    unsigned int pv_mode_cfg;
    unsigned int pv_bus_mode;
} cfg_d800;
extern Uint16 dcdc_soft_duty;
extern unsigned int eps_start_ok; /* 0xC269: the current controller's start_ok */
extern float eps_v_peak;
extern struct grid_limits_t {
    unsigned int f_ac_high_out;
    unsigned int f_ac_low_out;
    unsigned int f_ac_high_in;
    unsigned int f_ac_low_in;
    unsigned int v_ac_high_c;
    unsigned int v_ac_low_c;
    unsigned int f_ac_high_c;
    unsigned int f_ac_low_c;
} grid_limits;
extern float grid_power_max_pv;
extern struct os_tcb0_t {
    unsigned int vDA80;
    unsigned int vDA81;
    unsigned int period;
    unsigned int ticks;
    union bits16_u flags;
} os_tcb0;
extern unsigned int pll_locked;
#ifndef OWN_rating_params
extern struct rating_params_t {
    float rated_power; /* 0xD880: 0.1 W */
    float grid_v_nominal;
    float grid_f_nominal;
    float grid_omega_step;
    float rated_current;
    float rated_ac_power;
    float grid_power_max_pv;
    float grid_v_ref;
    float grid_current_max;
    float batt_chg_power_max;  /* 0xD892: HR308 battery rating, 0.1 W */
    float batt_dis_power_max;  /* 0xD894: HR308 + 300 W */
    float force_charge_pct;    /* 0xD896: HR313 % */
    float force_discharge_pct; /* 0xD898: HR314 % */
    float force_chg_soc_f;
    float pv_power_rated;
    float pv_power_limit;
    float cfg_d8a0;
    float pv_derate_voltage;
    float grid_voltage_87_percent;
    float cfg_d8a6;
    float pv_ov_limit;
    float batt_temp_limit; /* 0xD8AA: 100.0 degrees; only an unread counter uses it */
    float batt_temp_clear;
    float charger_temp_limit;
    float charger_temp_derate; /* 0xD8B0: 87.0 degrees, 5 % per degree above */
    float heatsink_temp_limit;
    float heatsink_temp_clear;
    float grid_low_voltage_derate_threshold;
    float arm_batt_setpoint; /* 0xD8B8: the ARM's battery power setpoint, 0.1 W */
    float grid_port_max_power;
    float export_target_unfiltered;
    float export_target_filtered;
} rating_params;
#endif
extern unsigned int run_mode_req;
/* ---- end of declarations ---- */

#endif
