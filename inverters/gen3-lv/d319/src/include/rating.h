/* The rating task (task 1): the RMS values and powers of the AC side, the rated power.
   ac_measure.c is task 1; ac (0xD3C0) also holds the battery current maxima and the PV limit PIs
   that power_limits.c uses. */
#ifndef RATING_H
#define RATING_H

#include "F2806x_Device.h"
#include "util.h"

/* the sums for the RMS values (grid_rms_accum.c adds into ac_sums every control cycle; at the end of a
 * grid period they are handed over in ac_sums_isr) */
typedef struct ac_acc_t {
    float inv_n_grid, inv_n_eps, inv_v_sq, grid_i_sq, leak_i_sq, grid_v_sq, acc_0C, acc_0E,
        eps_v_sq, eps_i_sq, acc_14, eps_p, acc_18, grid_p, acc_1C, dc_sum;
    Uint16 n_eps, n_grid;
} ac_acc_t;

/* 0xD3C0: the AC measurements. batt_max_charge_current (0xD400) is HR309 x 100 (0.01 A, 10 .. 80 A,
   default 60 A); current_limit_tapers sets batt_max_discharge_current (0xD402) to it + 15.00 A.
   pvp_pi and pvi_pi are power_limits.c's PV power and PV current PIs. */
struct ac_meas_t {
    float f00, f02, f04, f06, grid_v_raw, grid_v_cal, grid_v, inv_v, grid_i, i2, f14, f16, f18, f1A,
        f1C, f1E, f20, f22, unused_24, unused_26, eps_v, eps_v_filt, eps_i, eps_s, eps_p, eps_dc,
        eps_v_ref, grid_pi_out, f38, dc_component, apparent_p, apparent_p_filt,
        batt_max_charge_current, batt_max_discharge_current, f44, grid_freq_f, grid_freq_filt,
        active_p_raw, active_p, active_p_filt, reactive_p, f52, arm_power;
    PI_BLK eps_pi;
    PI_BLK grid_pi;
    float f7A, f7C, f7E, f80, f82, f84, f86, f88, f8A;
    PI_BLK pvp_pi;
    float f9E, fA0, fA2, fA4, fA6, fA8, fAA, fAC, fAE;
    PI_BLK pvi_pi;
    int pf_x1000, pf_code; /* 0xD482, 0xD483 */
    unsigned int uC4, u_d487, uC6, uC7;
    float i2_k, aux_f16_k, fCC;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "control.h"

/* ac_measure.c */
extern float active_power;
extern float apparent_power_est;
void arm_power_check(void);
extern int arm_power_fast_count;
extern int arm_power_slow_count;
extern unsigned long arm_power_trip_timer;
extern unsigned int arm_power_trips;
void bus_voltage_ref_update(void);
extern unsigned int cf14b12_clear_count;
extern unsigned int cf19b7_clear_count;
extern unsigned int disable_count;
extern unsigned long ecap_period_hist[3];
extern unsigned long ecap_period_med;
extern float eps_dc_filt;
void eps_dc_offset_trim(void);
extern float eps_dc_trim;
extern unsigned int eps_dc_trim_delay;
extern unsigned int eps_fault_clear_count;
extern unsigned long eps_lockout_timer;
void eps_output_check(void);
extern unsigned int eps_ov_clear_count;
extern unsigned int eps_ov_count;
extern unsigned int eps_ov_fast_count;
extern unsigned int eps_overload_count;
extern unsigned int eps_overload_fast_count;
extern unsigned int eps_short_count;
extern unsigned int eps_trip_count;
extern unsigned int eps_uv2_count;
extern unsigned int eps_uv_count;
extern float eps_v_ramp;
extern unsigned int eps_vctl_delay;
void eps_voltage_control(void);
extern unsigned int freq50_count;
extern unsigned int freq60_count;
extern unsigned int freq_hist_idx;
extern unsigned int grid_freq;
void grid_freq_detect(unsigned int f);
extern unsigned int grid_freq_detected;
void grid_loss_watchdog(unsigned int lim);
extern unsigned int grid_pi_delay;
void grid_power_filter(void);
void grid_power_limit_pi(void);
void grid_power_pf_calc(void);
extern float pf_denom;
extern unsigned int relay_b4_count;
extern unsigned int relay_b5_count;
extern unsigned int relay_fail_count;
extern unsigned long relay_lockout_timer;
extern unsigned int relay_lockouts;
void relay_test_check(void);
extern float relay_v_diff;
void relays_off_when_disabled(void);
void select_state(void);
extern unsigned int watchdog_kick_count;

/* eps_rms_accum.c */
void eps_rms_accum(void);

/* grid_rms_accum.c */
void grid_rms_accum(void);

/* defined by no source: the linker places it at its map address */
extern struct ac_meas_t ac;
extern struct ac_acc_t ac_sums;
extern struct ac_acc_t ac_sums_isr;
extern float eps_dc_trim_isr;
extern unsigned int eps_vctl_enable;
extern float eps_vctl_out;
extern unsigned int grid_code;
extern unsigned int grid_cycle_watchdog;
extern unsigned int load_percent;
extern float out_power_limit;
extern float q_current_ref;
extern float q_setpoint;
extern float rated_ac_power;
extern struct relay_cmd_t relay_cmd;
/* ---- end of declarations ---- */

#endif
