/* The grid code: voltage and frequency protection, RoCoF, reactive power, power responses and
   limits, the CEI 0-21 auto test; task 4 (grid_support.c). Most of its RAM is reached through
   ram_view.h's macros; this header holds the structured types and the declarations.
   docs/subsystems/grid.md describes the rules. */
#ifndef GRID_H
#define GRID_H

#include "F2806x_Device.h"
#include "util.h"

/* the RoCoF detector's state, as rocof_detect updates it through a pointer (the earlier name, an
 * oscillation detector, was wrong: it sums the frequency change over a window) */
struct rocof_det_t {
    float x;          /* the new sample */
    float n;          /* samples per window, less one (25) */
    int en;           /* 1: running */
    unsigned int thr; /* threshold on the summed change (110 = 1.10 Hz) */
    int flag;         /* 1: the last window's change was over thr */
    unsigned int pad;
    float sum; /* the last window's summed change */
};

/* 0xCDD4..: a count word (ram_view.h's fast_trip_hold) and the RoCoF detector */
struct rocof_blk_t {
    unsigned int cnt;
    unsigned int pad;
    struct rocof_det_t osc;
};

/* a one-word struct (not used: 0xDAD0 is arm_link.c's array bms_serial[10], the last of its .ebss) */
struct a_word {
    unsigned int w;
};

/* 0xCDE0: the CEI 0-21 auto test's state (grid_support.c's autotest_run). ram_view.h names the
 * words: auto_test_state (CDE0), the test value, result, phase, elapsed and trip times, the
 * expected time and limit; the ARM reads them on D frame page 4. */
struct autotest_t {
    unsigned int v_CDE0;
    unsigned int v_CDE1;
    unsigned int v_CDE2;
    unsigned int v_CDE3;
    unsigned int v_CDE4;
    unsigned int v_CDE5;
    unsigned int v_CDE6;
    unsigned int v_CDE7;
    unsigned int v_CDE8;
    unsigned int v_CDE9;
    unsigned int v_CDEA;
    unsigned int v_CDEB;
};

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "power_limits.h"

/* grid_support.c */
float active_power_rate_limit(void);
extern struct autotest_t autotest;
extern unsigned int autotest_expect_ms;
void autotest_run(void);
extern int autotest_step;
extern unsigned int autotest_time_tol;
extern unsigned int autotest_wait_cnt;
extern unsigned int cosphi_p_active;
extern float cosphi_p_pct;
extern float cosphi_p_pf;
void ctl_timers_tick(void);
extern unsigned int ctl_uptime_ticks;
extern float dc_pi_out_max;
extern float dc_power_limit;
extern float dc_power_limit_prev;
float dc_side_power_limit(void);
void dci_control(void);
extern float dci_filt;
extern float dci_offset_cmd;
extern unsigned int dci_settle_cnt;
extern unsigned int dci_tick;
extern unsigned int dci_trip_cnt;
extern unsigned int dci_trip_cnt_it;
extern unsigned int exp_fault_clear_cnt;
extern float exp_limit_check;
extern unsigned int exp_over_cnt1;
extern unsigned int exp_over_cnt2;
extern float exp_p_default;
extern float exp_p_limit;
extern unsigned int exp_ramp_active;
extern float exp_ramp_power;
void export_overload_check(void);
float export_power_limit(void);
extern unsigned int fast_trip_active;
extern unsigned int fast_trip_release_cnt;
extern volatile float fast_trip_v_high;
extern volatile float fast_trip_v_low;
extern struct freq_resp_t freq_resp;
void freq_response_it(float f);
extern unsigned int ft_clear_cnt;
extern unsigned int ft_high1_cnt;
extern unsigned int ft_high2_cnt;
extern unsigned int ft_low1_cnt;
extern unsigned int ft_low2_cnt;
extern unsigned int ft_test_freq;
void grid_freq_protect(void);
float grid_import_current_limit(void);
void grid_reconnect_check(void);
void grid_support_update(void);
extern unsigned int grid_v_10min;
void grid_v_10min_update(void);
void grid_v_fast_trip(void);
void grid_volt_protect(void);
extern unsigned int hw_fault_clear_cnt;
extern float imp_i_scale;
extern float imp_p_filt;
extern float imp_p_limit;
extern float imp_p_target;
extern unsigned int it_ofr_delay_cnt;
extern float it_ofr_f_full;
extern float it_ofr_f_start;
extern float it_ofr_power;
extern float it_ofr_power_prev;
extern unsigned int it_ofr_recover_cnt;
extern unsigned int it_ovr_batt_state;
extern unsigned int it_ovr_period;
extern float it_ovr_power;
extern float it_ovr_power_min;
extern float it_ovr_step;
extern unsigned int it_ovr_tick;
extern float it_p_base;
extern unsigned int it_ufr_delay_cnt;
extern float it_ufr_f_full;
extern float it_ufr_f_start;
extern float it_ufr_power;
extern float it_ufr_power_prev;
extern unsigned int it_ufr_recover_cnt;
extern unsigned int ofr_active;
extern unsigned int ofr_curve_type;
extern unsigned int ofr_delay_cnt;
extern unsigned int ofr_first_recover;
extern unsigned int ofr_first_recover_s6;
extern unsigned int ofr_it_band;
extern unsigned int ofr_it_band3_cnt;
extern unsigned int ofr_it_band_prev;
extern unsigned int ofr_it_delay_ms;
extern float ofr_p_base;
extern float ofr_p_calc;
extern float ofr_p_limit;
extern float ofr_p_min;
float ofr_power_limit(void);
extern unsigned int ofr_recover_cnt;
extern unsigned int ofr_s6_delay_cnt;
extern float ofr_s6_droop;
extern unsigned int ofr_s6_enabled;
extern float ofr_s6_p_base;
extern float ofr_s6_power;
extern float ofr_s6_power_prev;
extern unsigned int ofr_s6_recover_cnt;
void ofr_storage_response(void);
extern unsigned int ovr_batt_state;
extern float ovr_p_base;
extern unsigned int ovr_period;
extern float ovr_power;
extern float ovr_power_min;
extern float ovr_step;
void ovr_storage_response(void);
extern unsigned int ovr_tick;
extern float ovr_v_start;
float p_limit_by_q(void);
extern float p_limit_q;
extern float p_rate_limit;
extern unsigned int pf_band_cap;
extern unsigned int pf_band_hi;
extern unsigned int pf_band_lo;
extern float pf_cosphi;
float pf_curve_interp(float x, float x0, float y0, float x1, float y1);
extern unsigned int pf_flag_unused;
float pf_of_u_adjust(float x, float y);
extern unsigned int pfp_lock;
extern float pfp_pf;
extern float pfu_v_filt;
void power_limits_update(void);
extern unsigned int prot_high1;
extern unsigned int prot_high2;
extern unsigned int prot_low1;
extern unsigned int prot_low2;
extern float pv_i_raw_max;
float q_cosphi_p(void);
extern float q_cosphi_p_out;
extern float q_current_ref_prev;
extern float q_fixed_pct;
float q_fixed_percent(void);
float q_fixed_pf(void);
extern float q_fixed_pf_out;
extern float q_mode_out;
float q_of_u_curve(float x);
float q_of_u_fixed(float x);
float q_of_u_lockin(float x);
float q_pf_curve(void);
extern float q_pf_curve_out;
extern float q_pt1_dir;
extern float q_pt1_k;
extern float q_pt1_last;
extern float q_pt1_min_step;
float q_pt1_response(float x);
extern float q_pt1_step;
extern unsigned int q_pt1_step_pending;
extern float q_pt1_target;
extern unsigned int q_pt1_tick;
extern float q_pt1_time;
extern float q_qu;
extern unsigned int q_rec_active;
extern unsigned int q_rec_cnt;
extern float q_rec_k;
extern float q_rec_start;
extern float q_rec_target;
extern float q_rec_time;
float q_restore_response(float x);
extern unsigned int qu_active;
extern float qu_fixed_v_filt;
extern float qu_k;
extern unsigned int qu_lockout_cnt;
extern float qu_v_filt;
void reactive_power_update(void);
extern unsigned int reconnect_ok_cnt;
extern struct rocof_blk_t rocof_blk;
void rocof_detect(struct rocof_det_t *p);
extern float rocof_last;
extern float rocof_prev;
extern unsigned int rocof_sample_cnt;
extern float rocof_sum_acc;
extern float soft_start_power;
float soft_start_ramp(void);
extern unsigned int soft_start_tick;
float temp_derate_limit(void);
extern unsigned int ufr_active;
extern unsigned int ufr_delay_cnt;
extern float ufr_droop_k;
extern float ufr_p_base;
extern float ufr_p_limit;
extern float ufr_p_prev;
float ufr_power_limit(float x);
extern unsigned int ufr_recover_cnt;
extern unsigned int ufr_recover_delay;
extern unsigned int ufr_recover_freq;
extern float ufr_recover_step;
extern unsigned int ufr_s6_au_curve;
extern unsigned int ufr_s6_delay_cnt;
extern float ufr_s6_droop;
extern unsigned int ufr_s6_enabled;
extern float ufr_s6_p_base;
extern float ufr_s6_power;
extern float ufr_s6_power_prev;
extern unsigned int ufr_s6_recover_cnt;
void ufr_storage_response(void);
extern unsigned int uvr_batt_state;
extern float uvr_p_base;
extern unsigned int uvr_period;
extern float uvr_power;
extern float uvr_power_target;
void uvr_storage_response(void);
extern unsigned int uvr_tick;
extern float uvr_v_full;
extern float uvr_v_start;
extern unsigned int v10_index;
extern unsigned int v10_sample_cnt;
extern unsigned int v10_slots[200];
extern unsigned long v10_sum;
extern volatile unsigned int v10_window;
float volt_watt_limit(void);
extern unsigned int vt_10min_cnt;
extern unsigned int vt_clear_cnt;
extern unsigned int vt_high1_cnt;
extern unsigned int vt_high2_cnt;
extern unsigned int vt_inst_ov_cnt;
extern unsigned int vt_it_10min_cnt;
extern unsigned int vt_it_high_cnt;
extern unsigned int vt_it_low_cnt;
extern unsigned int vt_low1_cnt;
extern unsigned int vt_low2_cnt;
extern unsigned int vt_test_voltage;
extern unsigned int vw_latched;
extern float vw_p_limit;
extern unsigned int vw_tick;
extern float vw_v_filt;
extern float vw_v_filt2;

/* defined by no source: the linker places it at its map address */
extern unsigned int arm_rx_frame[45];
extern volatile unsigned int arm_tx_frame[45];
extern unsigned int bms_fc4_buf[100];
extern unsigned int slow_ramp_req;
/* ---- end of declarations ---- */

#endif
