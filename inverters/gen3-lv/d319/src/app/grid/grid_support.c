// clang-format off
/* RTOS task 4 (every 20 ms) and the grid code: grid voltage and frequency protection, RoCoF, the
   residual current monitor's step, DC injection control, the CEI 0-21 auto test, reactive power
   (fixed PF, cos phi(P), PF(P), fixed Q, Q(U)), the active power limits and ramps, and the
   frequency and voltage responses (P(f), and the storage responses in op_state 6).
   Also app_entry, the first function of .text (0x3E8004).
   Entry points: task4 (an RTOS task); app_entry (calls start).
   Data: this file's variables at 0xCCC0 .. 0xCF07 (below), with three blocks: rocof_blk, freq_resp
   (0xCE00, the responses' state and outputs) and autotest (0xCDE0). The settings come from the ARM
   through arm_link.c and arm_page_write.c (the HR numbers are in map/names.tsv); the ram_view.h
   macros name them here.
   Outputs: out_power_limit (0xCF2C, 0.1 W), the grid current bounds i_ref_max/i_ref_min (0xC414/
   0xC414, 0.1 A peak), the reactive current reference q_current_ref (0xC408), the DC injection
   correction dci_offset (0xC42A), the response power freq_resp.grid_resp_power (0xCE1E), and the
   fault bits CF12 bit 12 (voltage), CF12 bit 15 (frequency), CF13 bit 15 (grid lost), CF14 bit 5
   (export overload), CF14 bit 7 (outside the reconnect window), CF17 bit 0 (DC injection).
   Units: grid voltage 0.1 V, frequency 0.01 Hz, power 0.1 W, times in ticks of 20 ms unless the
   setting is stated in ms. op_state 5 is grid with PV, 6 grid with the battery only.
   Grid codes (grid_code, 0xD34A, the high byte of HR2): the code tests the numbers in groups.
   4 is Italy (CEI 0-21 auto test). 3 and 19 share the Australian-style rules (volt-watt 253 to
   260 V for 3, 242 to 250 V for 19; the ARM maps codes 20 and 21 to 3), so 19 is probably New
   Zealand; this is inference from the numbers (docs/subsystems/grid.md). The group
   2, 9, 11, 13, 22..26, 28 shares one set of European-style rules; 7 has PT1 smoothing of Q;
   8, 12, 16, 17 share a fixed over-frequency recovery at 50.2/60.2 Hz.
   Neighbours: rcmu.c (rcmu_step), ac_measure.c (task 1: the measurements), converter_ctrl.c (the
   interrupt that uses the bounds), power_limits.c (task 2), arm_link.c (task 5).
   name: descriptive (was a_ctl.c; map/filenames.tsv). Header grid.h; its RAM view is ram_view.h.
   at: .text 0x3E8004 .. 0x3EAABA (range A's first file; docs/names/A-notes.md has each address) */
// clang-format on
#define OWN_state_flags /* match: this file reads and writes 0xCF0E as a plain word (masks); the header has the bit view union bits16_u, and this file's code is written against the word */
#include "util.h"
#include "os.h"
#include "sci.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "meas.h"
#include "control.h"
#include "grid.h"
#include "state.h"
#include "rating.h"
#include "rcmu.h"
#include "boot.h"
#include "power_limits.h"
#include "ram_view.h"
#include "F2806x_Device.h"
extern unsigned int state_flags; /* match: own form, see OWN_state_flags */

// clang-format off
/* ebss: tools/climb/ebss.py */
unsigned int uvr_batt_state = 0;
unsigned int uvr_tick = 0;
unsigned int exp_fault_clear_cnt = 0;
unsigned int ufr_s6_au_curve = 0;
unsigned int ovr_period = 0;
unsigned int ovr_tick = 0;
unsigned int q_pt1_tick = 0;
unsigned int uvr_period = 0;
unsigned int ufr_s6_recover_cnt = 0;
unsigned int v10_index = 0;
int autotest_step = 0;
unsigned int ufr_s6_delay_cnt = 0;
unsigned int v10_sample_cnt = 0;
unsigned int q_rec_cnt = 0;
unsigned int q_rec_active = 0;
unsigned int prot_high2;
unsigned int prot_high1;
unsigned int pf_flag_unused = 0;
unsigned int prot_low2;
unsigned int grid_v_10min;
unsigned int ofr_s6_enabled = 0;
unsigned int prot_low1;
unsigned int ofr_first_recover_s6 = 0;
unsigned int it_ovr_period = 0;
unsigned int ofr_it_band3_cnt = 0;
unsigned int ufr_recover_delay = 0;
unsigned int it_ufr_recover_cnt = 0;
unsigned int ofr_delay_cnt = 0;
unsigned int it_ovr_batt_state = 0;
unsigned int it_ovr_tick = 0;
unsigned int ufr_recover_freq = 0;
unsigned int ofr_recover_cnt = 0;
unsigned int ofr_s6_recover_cnt = 0;
unsigned int ufr_active = 0;
unsigned int it_ofr_recover_cnt = 0;
unsigned int ufr_delay_cnt = 0;
unsigned int ft_high1_cnt = 0;
unsigned int ft_low2_cnt = 0;
unsigned int ofr_it_band_prev = 0;
unsigned int ufr_recover_cnt = 0;
unsigned int it_ofr_delay_cnt = 0;
unsigned int ft_low1_cnt = 0;
unsigned int ofr_first_recover = 0;
unsigned int ofr_active = 0;
unsigned int vt_test_voltage = 0;
unsigned int vt_inst_ov_cnt = 0;
unsigned int qu_active = 0;
unsigned int fast_trip_active = 0;
unsigned int fast_trip_release_cnt = 0;
unsigned int qu_lockout_cnt = 0;
unsigned int it_ufr_delay_cnt = 0;
unsigned int reconnect_ok_cnt = 0;
unsigned int ofr_s6_delay_cnt = 0;
unsigned int ofr_it_delay_ms = 0;
unsigned int ctl_uptime_ticks = 0;  /* task 4 runs since start, up to 500 (10 s) */
unsigned int vw_latched = 0;
unsigned int vw_tick = 0;
unsigned int ofr_curve_type = 0;
unsigned int vt_low1_cnt = 0;
unsigned int pf_band_hi = 0;
unsigned int pf_band_lo = 0;
unsigned int pf_band_cap = 0;
unsigned int ofr_it_band = 0;
unsigned int exp_ramp_active = 0;
unsigned int hw_fault_clear_cnt = 0;
unsigned int vt_high2_cnt = 0;
unsigned int exp_over_cnt1 = 0;
unsigned int q_pt1_step_pending = 0;
unsigned int vt_low2_cnt = 0;
unsigned int vt_10min_cnt = 0;
unsigned int dci_tick = 0;
unsigned int ft_test_freq = 0;
unsigned int pfp_lock = 0;
unsigned int ft_high2_cnt = 0;
unsigned int cosphi_p_active = 0;
unsigned int dci_trip_cnt = 0;
unsigned int vt_clear_cnt = 0;
unsigned int rocof_sample_cnt = 0;
unsigned int soft_start_tick = 0;
unsigned int vt_it_high_cnt = 0;
unsigned int autotest_wait_cnt = 0;
unsigned int autotest_time_tol = 0;
unsigned int autotest_expect_ms = 0;
volatile unsigned int v10_window = 0;
unsigned int vt_it_low_cnt = 0;
unsigned int dci_trip_cnt_it = 0;
unsigned int ft_clear_cnt = 0;
unsigned int dci_settle_cnt = 0;
unsigned int a_ctl_unused_cd1b;
unsigned int vt_high1_cnt = 0;
unsigned int exp_over_cnt2 = 0;
unsigned int ufr_s6_enabled = 0;
unsigned int ovr_batt_state = 0;
unsigned int vt_it_10min_cnt = 0;
float it_ovr_power_min = 0.0;
float it_ovr_step = 0.0;
float it_ovr_power = 0.0;
float dc_power_limit = 0.0;
float it_ofr_power = 0.0;
float it_ufr_power_prev = 0.0;
float dc_power_limit_prev = 0.0;
volatile float fast_trip_v_high;
float it_ufr_f_start = 0.0;
float cosphi_p_pf = 1.0;
volatile float fast_trip_v_low;
float q_fixed_pf_out = 0.0;
float dci_filt = 0.0;
float vw_v_filt2 = 0.0;
float ovr_power = 0.0;
float dci_offset_cmd = 0.0;
float pf_cosphi = 1.0;
float pfp_pf = 1.0;
float it_ofr_f_start = 0.0;
float it_ofr_power_prev = 0.0;
float it_ofr_f_full = 0.0;
float cosphi_p_pct = 0.0;
float q_cosphi_p_out = 0.0;
float it_ufr_f_full = 0.0;
float ufr_p_limit = 0.0;
float ovr_p_base = 0.0;
float ufr_s6_power = 0.0;
float ufr_recover_step = 0.0;
float ufr_s6_p_base = 0.0;
float q_rec_k = 1.0;
float ufr_s6_power_prev = 0.0;
float ufr_p_base = 0.0;
float ovr_v_start = 0.0;
float ofr_s6_p_base = 0.0;
float ovr_step = 0.0;
float uvr_p_base = 0.0;
float ufr_s6_droop = 0.0;
float ofr_s6_power_prev = 0.0;
float ufr_p_prev = 0.0;
float ofr_s6_droop = 0.0;
float uvr_power_target = 0.0;
float q_pt1_step = 0.0;
float q_pt1_k = 1.0;
float q_pt1_dir = 0.0;
float q_pt1_min_step = 6000.0;
float q_pt1_last = 0.0;
float q_pt1_target = 0.0;
float q_rec_time = 0.0;
float q_pt1_time = 0.0;
float rocof_last = 0.0;
float rocof_prev = 0.0;
float uvr_v_full = 0.0;
float pfu_v_filt = 0.0;
float exp_limit_check = 0.0;
float rocof_sum_acc = 0.0;
float q_rec_start = 0.0;
float q_rec_target = 0.0;
float imp_i_scale = 0.0;
float q_mode_out = 0.0;
float vw_p_limit = 0.0;
float imp_p_target = 0.0;
float imp_p_filt = 0.0;
float imp_p_limit = 0.0;
float soft_start_power = 0.0;
float exp_ramp_power = 0.0;
float q_fixed_pct = 0.0;
float pv_i_raw_max = 0.0;
float q_pf_curve_out = 0.0;
float qu_fixed_v_filt = 0.0;
float dc_pi_out_max = 0.0;
float vw_v_filt = 0.0;
float exp_p_default = 0.0;
float exp_p_limit = 0.0;
float q_current_ref_prev = 0.0;
float q_qu = 0.0;
float ofr_s6_power = 0.0;
float ufr_droop_k = 0.0;
float uvr_power = 0.0;
float ovr_power_min = 0.0;
float uvr_v_start = 0.0;
float qu_k = 0.0;
float qu_v_filt = 0.0;
unsigned long v10_sum = 0;
float it_ufr_power = 0.0;
float p_rate_limit = 0.0;
float p_limit_q = 0.0;
float ofr_p_calc = 0.0;
float ofr_p_base = 0.0;
float ofr_p_limit = 0.0;
float it_p_base = 0.0;
float ofr_p_min = 0.0;
struct rocof_blk_t rocof_blk;
struct freq_resp_t freq_resp;
struct autotest_t autotest;
unsigned int v10_slots[200] = {2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300, 2300};  /* 3 s means of the grid voltage, 200 slots = 10 minutes */
/* end ebss */
// clang-format on

/* The first function of .text; calls start (os_main.c). */
void app_entry(void) { start(); }

/* RTOS task 4: on every 20 ms tick, timers, reconnect window, 10-minute voltage mean, voltage and
   frequency protection, the RCMU, DC injection, the fast voltage trip, the auto test, the export
   overload check, reactive power, the power limits and the frequency and voltage responses. */
void task4(void) {
    ctl_uptime_ticks = 0;
    for (;;) {
        while (!(os_wait(0) & EV_TICK_M))
            ;
        ctl_timers_tick();
        grid_reconnect_check();
        grid_v_10min_update();
        grid_volt_protect();
        grid_freq_protect();
        rcmu_step();
        dci_control();
        grid_v_fast_trip();
        autotest_run();
        export_overload_check();
        reactive_power_update();
        power_limits_update();
        grid_support_update();
    }
}

/* CF14 bit 7 while the inverter is not on the grid and the grid voltage (one cycle) or frequency
   is outside the reconnect window HR79..HR80 (V), HR81..HR82 (f); cleared on the grid, during the
   auto test, or after 101 ticks (2 s) inside the window. */
void grid_reconnect_check(void) {
    if (auto_test_state == 0) {
        if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
            fault_grid_eps.bit.b7 = 0;
        } else if (grid_voltage_fast > v_ac_high_c || grid_voltage_fast < v_ac_low_c ||
                   grid_freq > f_ac_high_c || grid_freq < f_ac_low_c) {
            fault_grid_eps.bit.b7 = 1;
            reconnect_ok_cnt = 0;
        } else if (++reconnect_ok_cnt > 100) {
            fault_grid_eps.bit.b7 = 0;
            reconnect_ok_cnt = 0;
        }
    } else {
        fault_grid_eps.bit.b7 = 0;
        reconnect_ok_cnt = 0;
    }
}

/* Counts task 4's uptime to 500 (10 s, the RCMU waits for it), the auto test's elapsed time in
   ms, and the Italian over-frequency delay in ms (done when it reaches HR300, ofr_delay). */
void ctl_timers_tick(void) {
    if (ctl_uptime_ticks < 500)
        ctl_uptime_ticks++;
    else
        ctl_uptime_ticks = 500;
    if (auto_test_state == 2)
        autotest_elapsed_ms += 20;
    if (ofr_it_delay_run == 1) {
        ofr_it_delay_ms += 20;
        if (ofr_it_delay_ms >= ofr_delay)
            ofr_it_delay_done = 1;
    } else {
        ofr_it_delay_done = 0;
        ofr_it_delay_ms = 0;
    }
}

/* The active power setpoint HR50 (% of rated, at most 100) with a slew limit per tick: 50 W for
   codes 3, 8, 12, 19 with 0xD7C2 bit 8; 40 W for the European group with that bit and 0xD7C0
   bit 4 or Q mode 2 or 6; for the European group otherwise, in op_state 6, HR306 x rated / 3,000,000 per
   tick (HR306 per mille of rated per minute) once run_status bit 2 is set (after any grid fault
   since start-up) and HR302 before that, the reverse of soft_start_ramp
   (docs/findings-control.md); in op_state 5 0.01 % of rated per tick; for code 7 the HR306 rate; else 50 W. */
float active_power_rate_limit(void) {
    float lim;
    if (active_power_rate > 100)
        active_power_rate = 100;

    p_rate_limit = active_power_rate * rated_ac_power * 0.01f;
    if ((grid_code == 3 || grid_code == 12 || grid_code == 8 || grid_code == 19) &&
        grid_func_flags.bit.b8 == 1) {
        lim = 500.0f;
    } else if ((grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
                grid_code == 26 || grid_code == 28) &&
               (eps_cfg_bits.bit.b4 == 1 || q_mode == 2 || q_mode == 6) &&
               grid_func_flags.bit.b8 == 1) {
        lim = 400.0f;
    } else if (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
               grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
               grid_code == 26 || grid_code == 28) {
        if (op_state == OP_GRID_BATT) {
            if (run_status_bits.bit.b2 == 1)
                lim = power_ramp_rate * rated_ac_power * 3.333e-7f;
            else
                lim = power_ramp_rate_reconnect * rated_ac_power * 3.333e-7f;
        } else {
            lim = rated_ac_power * 0.0001f;
        }
    } else if (grid_code == 7) {
        lim = power_ramp_rate * rated_ac_power * 3.333e-7f;
    } else {
        lim = 500.0f;
    }
    rate_limit(&p_rate_limit, p_rate_limit_prev, lim, -lim);
    limitf(&p_rate_limit, rated_ac_power, 0.0f);
    p_rate_limit_prev = p_rate_limit;
    return p_rate_limit;
}

/* The active power left beside the reactive setpoint: sqrt(S^2 - Q^2) with S the rated power
   (rated while the Australian-style under-frequency response is latched). */
float p_limit_by_q(void) {
    if (ufr_latched == 1 && (grid_code == 3 || grid_code == 19)) {
        p_limit_q = rated_ac_power;
    } else if (fabs(q_setpoint) < fabs(rated_ac_power)) {
        p_limit_q =
            sqrtf((rated_ac_power + 0.0f) * (rated_ac_power + 0.0f) - q_setpoint * q_setpoint);
    } else {
        p_limit_q = rated_ac_power;
    }
    return p_limit_q;
}

/* P(f) over-frequency power reduction in op_state 5 (grid with PV), with 0xD7C2 bit 3 (HR125).
   Generic codes: between HR321 (start) and HR66 + 0.10 Hz, after HR300 ticks, the base is the
   present inverter power and the limit falls by base x HR123 / 10000 per 0.01 Hz (x rated for
   codes 8, 12, 24, 28); it never rises while active for 3, 19 and the European group with 0xD7C2
   bit 2 (HR179). Below the recovery frequency (HR328 for the European and Australian groups,
   50.20/60.20 Hz for 8, 12, 16, 17, else HR321), after the recovery delay, the limit ramps up by
   3.05e-5 x rated per tick (5.5e-5 for 3 and 19, 3e-5 for the rest) until rated. The function
   writes the recovery delay setting itself for some codes (1500 for 3/19, 0 for 8/12/16/17,
   15150 for the rest; docs/findings-control.md). Italy (code 4) uses bands: above HR321 after
   HR300 ms, with ofr_it_param (0xD854) in 20..50, a fixed 0.77 % of the base per 0.01 Hz;
   recovery only within 49.90..50.10 Hz after 15150 ticks (5 minutes). */
float ofr_power_limit(void) {
    float lim, lo, k, step;
    unsigned int hi, prev;
    lim = ofr_start_freq;
    if (op_state == OP_GRID_PV && grid_func_flags.bit.b3) {
        if (grid_code == 7 || grid_code == 3 || grid_code == 8 || grid_code == 12 ||
            grid_code == 2 || grid_code == 15 || grid_code == 13 || grid_code == 9 ||
            grid_code == 11 || grid_code == 16 || grid_code == 17 || grid_code == 18 ||
            grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
            grid_code == 26 || grid_code == 19 || grid_code == 28) {
            hi = f_ac_high_out + 10;
            if (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
                grid_code == 26 || grid_code == 28) {
                k = ofr_droop * 0.0001f;
                lo = ofr_recover_freq;
                step = 3.05e-05f;
            } else if (grid_code == 7 || grid_code == 18) {
                k = ofr_droop * 0.0001f;
                lo = lim;
                step = 3.05e-05f;
            } else if (grid_code == 3 || grid_code == 19) {
                lo = ofr_recover_freq;
                ofr_recover_delay = 1500;
                k = ofr_droop * 0.0001f;
                step = 5.5e-05f;
            } else if (grid_code == 8 || grid_code == 12 || grid_code == 17 || grid_code == 16) {
                if (grid_is_60hz == 1)
                    lo = 6020.0f;
                else
                    lo = 5020.0f;
                ofr_recover_delay = 0;
                if (grid_code == 17 || grid_code == 16)
                    k = 0.005f;
                else
                    k = 0.002f;
                step = 3e-05f;
            } else {
                ofr_recover_delay = 15150;
                k = ofr_droop * 0.0001f;
                lo = lim;
                step = 3e-05f;
            }
            if (grid_freq_f >= lim && grid_freq_f <= hi) {
                if (ofr_active == 0) {
                    if (++ofr_delay_cnt > ofr_delay) {
                        ofr_p_base = inv_power;
                        if (ofr_p_base > rated_ac_power)
                            ofr_p_base = rated_ac_power;
                        ofr_delay_cnt = 0;
                        ofr_active = 1;
                        ofr_first_recover = 1;
                        ofr_p_min = ofr_p_base;
                        ofr_p_calc = ofr_p_base;
                    } else {
                        ofr_p_calc = out_power_limit;
                        if (ofr_p_calc > rated_ac_power)
                            ofr_p_calc = rated_ac_power;
                        ofr_p_min = ofr_p_calc;
                    }
                } else if (grid_code == 8 || grid_code == 12 || grid_code == 24 ||
                           grid_code == 28) {
                    ofr_p_calc = ofr_p_base - rated_ac_power * k * (grid_freq_f - lim);
                } else {
                    ofr_p_calc = ofr_p_base - ofr_p_base * k * (grid_freq_f - lim);
                }
                if (grid_code == 3 || grid_code == 19 ||
                    ((grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                      grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
                      grid_code == 26 || grid_code == 28) &&
                     grid_func_flags.bit.b2)) {
                    ofr_p_limit = ofr_p_min < ofr_p_calc ? ofr_p_min : ofr_p_calc;
                    ofr_p_min = ofr_p_limit;
                } else {
                    ofr_p_limit = ofr_p_calc;
                }
                ofr_recover_cnt = 0;
            } else if (grid_freq_f >= lo && grid_freq_f < lim) {
                ofr_recover_cnt = 0;
                if (!((grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                       grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
                       grid_code == 26 || grid_code == 28 || grid_code == 3 || grid_code == 19) &&
                      grid_func_flags.bit.b2)) {
                    ofr_active = 0;
                    ofr_first_recover = 0;
                    ofr_p_limit = rated_ac_power;
                }
            } else if (grid_freq_f < lo && grid_freq_f > f_ac_low_out) {
                if (ofr_active != 0 &&
                    (grid_func_flags.bit.b2 ||
                     !(grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                       grid_code == 18 || grid_code == 28 || grid_code == 24 || grid_code == 23 ||
                       grid_code == 22 || grid_code == 25 || grid_code == 26))) {
                    if ((grid_code == 7 || grid_code == 15 || grid_code == 16 || grid_code == 17) &&
                        ofr_first_recover == 1) {
                        ofr_first_recover = 0;
                        ofr_p_limit = ofr_p_min;
                    } else if (++ofr_recover_cnt > ofr_recover_delay) {
                        ofr_recover_cnt = ofr_recover_delay;
                        ofr_p_limit = step * rated_ac_power + ofr_p_limit;
                        if (ofr_p_limit >= ofr_p_base) {
                            if (grid_code == 3 || grid_code == 19)
                                ofr_active = 0;
                            ofr_first_recover = 0;
                        }
                        if (ofr_p_limit > rated_ac_power) {
                            ofr_active = 0;
                            ofr_p_limit = rated_ac_power;
                        }
                    }
                } else {
                    ofr_active = 0;
                    ofr_first_recover = 0;
                    ofr_p_limit = rated_ac_power;
                }
            } else {
                ofr_recover_cnt = 0;
                ofr_active = 0;
                ofr_first_recover = 0;
                ofr_p_limit = rated_ac_power;
                ofr_p_base = rated_ac_power;
                ofr_p_calc = 0.0f;
            }
        } else if (grid_code == 4) {
            ofr_recover_delay = 15150;
            if (grid_freq_f >= ofr_start_freq && grid_freq_f < f_ac_high_in) {
                ofr_it_band3_cnt = 0;
                ofr_recover_cnt = 0;
                prev = ofr_it_band;
                ofr_it_band = 1;
                ofr_it_band_prev = prev;
                if (prev == 1) {
                    ofr_it_delay_run = 1;
                    if (ofr_it_delay_done == 1) {
                        if (ofr_active == 0) {
                            ofr_p_base = inv_power;
                            ofr_active = 1;
                            if (ofr_p_base > rated_ac_power)
                                ofr_p_base = rated_ac_power;
                        } else if (ofr_it_param < 20 || ofr_it_param > 50) {
                            ofr_p_limit = out_power_limit;
                        } else {
                            ofr_p_min = ofr_p_limit;
                            ofr_p_calc = ofr_p_base - 0.0077f * (ofr_p_base * (grid_freq_f - lim));
                            ofr_p_limit = ofr_p_calc < ofr_p_min ? ofr_p_calc : ofr_p_min;
                        }
                    }
                }
            } else if (grid_freq_f > f_ac_low_in && grid_freq_f < ofr_start_freq) {
                prev = ofr_it_band;
                ofr_it_band3_cnt = 0;
                ofr_it_band = 2;
                ofr_it_band_prev = prev;
                if (prev == 2) {
                    ofr_it_delay_run = 0;
                    if (ofr_active != 0) {
                        if (grid_freq_f >= 4990.0f && grid_freq_f <= 5010.0f &&
                            ++ofr_recover_cnt > ofr_recover_delay) {
                            ofr_recover_cnt = ofr_recover_delay;
                            if (ofr_p_limit < ofr_p_base + 500.0f)
                                ofr_p_limit += 6.2e-05f * ofr_p_base;
                            else
                                ofr_p_limit += 6.2e-07f * (rated_ac_power * power_ramp_rate);
                            if (ofr_p_limit > rated_ac_power) {
                                ofr_p_limit = rated_ac_power;
                                ofr_active = 0;
                            }
                        }
                    } else {
                        ofr_p_limit = rated_ac_power;
                    }
                }
            } else {
                prev = ofr_it_band;
                ofr_it_band = 3;
                ofr_it_band_prev = prev;
                ofr_p_limit = rated_ac_power;
                if (prev == 3) {
                    ofr_it_delay_run = 0;
                    if (ofr_it_band3_cnt < 30) {
                        ofr_it_band3_cnt++;
                    } else {
                        ofr_active = 0;
                        ofr_it_band3_cnt = 0;
                        ofr_p_calc = 0.0f;
                        ofr_p_min = 0.0f;
                    }
                }
            }
        } else {
            ofr_p_limit = rated_ac_power;
        }
    } else {
        ofr_it_delay_run = 0;
        ofr_active = 0;
        ofr_it_band = 0;
        ofr_recover_cnt = 0;
        ofr_first_recover = 0;
        ofr_p_base = rated_ac_power;
        ofr_p_limit = rated_ac_power;
    }
    limitf(&ofr_p_limit, rated_ac_power, 0.0f);
    return ofr_p_limit;
}

/* Italy (code 4), op_state 6 (battery only). Under-frequency (0xD7C2 bit 12): below 49.80 Hz
   (charging, batt_state_cmd 1) or 49.60 Hz, after uf_delay ticks, the base is the inverter power +
   200 W and the power rises linearly to rated at 49.10 Hz, never falling while active; recovery
   within 50.00 +-0.20 Hz after 19500 ticks, ramping back over 15500 ticks. Over-frequency (bit 3):
   above HR321 the power falls from the base towards -rated (charging) at 51.50 Hz, never rising;
   recovery the same way after 15000 ticks. Over-voltage: above 253.0 V the power steps down 1 % of
   rated every 70 ticks (1.4 s) to -85 %, and back up below it; a change of batt_state_cmd ends it.
   The output (freq_resp.grid_resp_power) is the over-voltage, else the under-frequency, else the
   over-frequency response, else rated. */
void freq_response_it(float f) {
    if (batt_state_cmd == 1)
        freq_resp.uf_start_freq = 4980.0f;
    else
        freq_resp.uf_start_freq = 4960.0f;
    freq_resp.uf_full_freq = 4910.0f;
    freq_resp.of_start_freq = ofr_start_freq;
    freq_resp.of_full_freq = 5150.0f;
    if (op_state == OP_GRID_BATT && grid_func_flags.bit.b12) {
        if (f < freq_resp.uf_start_freq) {
            if (freq_resp.uf_active == 0) {
                if (++it_ufr_delay_cnt > freq_resp.uf_delay) {
                    it_ufr_delay_cnt = 0;
                    it_p_base = inv_power + 2000.0f;
                    freq_resp.uf_active = 1;
                    it_ufr_power = it_p_base;
                    freq_resp.uf_ramp_step = (rated_ac_power - it_p_base) / 15500.0f;
                    it_ufr_f_start = freq_resp.uf_start_freq;
                    it_ufr_f_full = freq_resp.uf_full_freq;
                }
            } else {
                it_ufr_power_prev = it_ufr_power;
                it_ufr_power =
                    it_p_base + (rated_ac_power - it_p_base) *
                                    ((it_ufr_f_start - f) / (it_ufr_f_start - it_ufr_f_full));
                limitf(&it_ufr_power, rated_ac_power, -rated_ac_power);
                if (it_ufr_power < it_ufr_power_prev)
                    it_ufr_power = it_ufr_power_prev;
            }
        } else if (freq_resp.uf_active == 0) {
            it_ufr_delay_cnt = 0;
            it_ufr_recover_cnt = 0;
            freq_resp.uf_ramp_step = 0.0f;
            it_ufr_power_prev = 0.0f;
        } else if (fabs(f - 5000.0f) < 20.0f) {
            if (++it_ufr_recover_cnt > 19500) {
                it_ufr_recover_cnt = 19500;
                it_ufr_power -= freq_resp.uf_ramp_step;
                if (it_ufr_power <= it_p_base) {
                    freq_resp.uf_active = 0;
                    it_ufr_power = it_p_base;
                }
            }
        } else if (f - 20.0f > 5000.0f) {
            freq_resp.uf_active = 0;
            it_ufr_recover_cnt = 0;
        } else {
            it_ufr_recover_cnt = 0;
        }
    } else {
        freq_resp.uf_active = 0;
        it_ufr_delay_cnt = 0;
        it_ufr_recover_cnt = 0;
        freq_resp.uf_ramp_step = 0.0f;
        it_ufr_power = 0.0f;
        it_ufr_power_prev = 0.0f;
    }
    if (op_state == OP_GRID_BATT && grid_func_flags.bit.b3) {
        if (f > freq_resp.of_start_freq) {
            if (freq_resp.of_active == 0) {
                if (++it_ofr_delay_cnt > freq_resp.of_delay) {
                    it_ofr_delay_cnt = 0;
                    it_p_base = inv_power + 2000.0f;
                    freq_resp.of_active = 1;
                    it_ofr_power = it_p_base;
                    freq_resp.of_ramp_step = (rated_ac_power + it_p_base) / 15500.0f;
                    it_ofr_f_start = freq_resp.of_start_freq;
                    it_ofr_f_full = freq_resp.of_full_freq;
                }
            } else {
                it_ofr_power_prev = it_ofr_power;
                it_ofr_power = it_p_base - (it_p_base + rated_ac_power) * (f - it_ofr_f_start) /
                                               (it_ofr_f_full - it_ofr_f_start);
                limitf(&it_ofr_power, rated_ac_power, -rated_ac_power);
                if (it_ofr_power > it_ofr_power_prev)
                    it_ofr_power = it_ofr_power_prev;
            }
        } else if (freq_resp.of_active == 0) {
            it_ofr_delay_cnt = 0;
            it_ofr_recover_cnt = 0;
            freq_resp.of_ramp_step = 0.0f;
            it_ofr_power_prev = 0.0f;
        } else if (fabs(f - 5000.0f) < 20.0f) {
            if (++it_ofr_recover_cnt > 15000) {
                it_ofr_recover_cnt = 16000;
                it_ofr_power += freq_resp.of_ramp_step;
                if (it_ofr_power >= it_p_base) {
                    freq_resp.of_active = 0;
                    it_ofr_power = it_p_base;
                }
            }
        } else if (f + 20.0f < 5000.0f) {
            freq_resp.of_active = 0;
            it_ofr_power = 0.0f;
        } else {
            it_ofr_recover_cnt = 0;
        }
    } else {
        freq_resp.of_active = 0;
        it_ofr_delay_cnt = 0;
        it_ofr_recover_cnt = 0;
        freq_resp.of_ramp_step = 0.0f;
        it_ofr_power = 0.0f;
        it_ofr_power_prev = 0.0f;
    }
    if (op_state == OP_GRID_BATT) {
        if (grid_voltage > 2530.0f) {
            if (freq_resp.ov_resp_active == 0) {
                freq_resp.ov_resp_active = 1;
                it_ovr_step = rated_ac_power * 0.01f;
                it_ovr_period = 70;
                it_ovr_batt_state = batt_state_cmd;
                it_p_base = inv_power;
                it_ovr_power_min = -rated_ac_power * 0.85f;
                it_ovr_power = inv_power;
            } else {
                if (it_ovr_batt_state == batt_state_cmd) {
                    if (++it_ovr_tick >= it_ovr_period) {
                        it_ovr_tick = 0;
                        it_ovr_power = it_ovr_power - it_ovr_step;
                        if (it_ovr_power < it_ovr_power_min)
                            it_ovr_power = it_ovr_power_min;
                    }
                } else {
                    freq_resp.ov_resp_active = 0;
                }
                it_ovr_batt_state = batt_state_cmd;
            }
        } else if (freq_resp.ov_resp_active == 0) {
            it_ovr_period = 0;
            it_ovr_power = 0.0f;
            it_ovr_power_min = 0.0f;
            it_ovr_step = 0.0f;
        } else if (++it_ovr_tick >= it_ovr_period) {
            it_ovr_tick = 0;
            it_ovr_power = it_ovr_step + it_ovr_power;
            if (it_ovr_power > it_p_base) {
                freq_resp.ov_resp_active = 0;
                it_ovr_power = it_p_base;
            }
        }
    } else {
        freq_resp.ov_resp_active = 0;
        it_ovr_period = 0;
        it_ovr_power = 0.0f;
        it_ovr_power_min = 0.0f;
        it_ovr_step = 0.0f;
    }
    if (freq_resp.ov_resp_active == 1)
        freq_resp.grid_resp_power = it_ovr_power;
    else if (freq_resp.uf_active == 1)
        freq_resp.grid_resp_power = it_ufr_power;
    else if (freq_resp.of_active == 1)
        freq_resp.grid_resp_power = it_ofr_power;
    else
        freq_resp.grid_resp_power = rated_ac_power;
    limitf(&freq_resp.grid_resp_power, rated_ac_power, -rated_ac_power);
}

/* Picks the frequency and voltage response in op_states 5 and 6: Italy's (freq_response_it), or
   for the European and Australian groups and code 7 the four storage responses, with priority
   over-frequency, under-frequency, over-voltage, under-voltage; with none active the output is
   rated + 500 W (no limit). Other states or codes: all off and 0. */
void grid_support_update(void) {
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        if (grid_code == 4) {
            freq_response_it(grid_freq_f);
            return;
        }
        if (grid_code == 2 || grid_code == 7 || grid_code == 13 || grid_code == 9 ||
            grid_code == 11 || grid_code == 28 || grid_code == 24 || grid_code == 23 ||
            grid_code == 22 || grid_code == 25 || grid_code == 26 || grid_code == 3 ||
            grid_code == 19) {
            ofr_storage_response();
            ufr_storage_response();
            ovr_storage_response();
            uvr_storage_response();
            if (freq_resp.of_active == 1)
                freq_resp.grid_resp_power = freq_resp.of_resp_power;
            else if (freq_resp.uf_active == 1)
                freq_resp.grid_resp_power = freq_resp.uf_resp_power;
            else if (freq_resp.ov_resp_active == 1)
                freq_resp.grid_resp_power = freq_resp.ov_resp_power;
            else if (freq_resp.uv_resp_active == 1)
                freq_resp.grid_resp_power = freq_resp.uv_resp_power;
            else
                freq_resp.grid_resp_power = rated_ac_power + 5000.0f;
            limitf(&freq_resp.grid_resp_power, rated_ac_power + 5000.0f,
                   -(rated_ac_power + 5000.0f));
            return;
        }
    }
    freq_resp.uf_active = 0;
    freq_resp.of_active = 0;
    freq_resp.ov_resp_active = 0;
    freq_resp.grid_resp_power = 0.0f;
}

/* A derate on the DC bus voltage: 0.2 % of rated per V above 520.0 V, 0.4 % per V (of the excess
   over 520 V) above 560.0 V, 28 % above 570.0 V, at most 70 %. While charging (batt_state_cmd 1)
   two PIs cap the AC power: the PV power over its rating (kp 0.3125, ki 0.1) and the larger PV
   current over 18.00 A (kp 3.125, ki 1.0); the larger output is taken off rated, slewed 10 W per
   tick. */
float dc_side_power_limit(void) {
    float r = rated_ac_power, k = 0.0f;
    if (bus_voltage > 5700.0f)
        k = 28.0f;
    else if (bus_voltage > 5600.0f)
        k = (bus_voltage - 5200.0f) * 0.1f * 0.4f;
    else if (bus_voltage > 5200.0f)
        k = (bus_voltage - 5200.0f) * 0.1f * 0.2f;
    limitf(&k, 70.0f, 0.0f);
    r = r - k * r * 0.01f;
    if (batt_state_cmd == 1) {
        pv_power_pi_e = (pv_power - pv_power_rated) * 0.01f;
        pv_power_pi_kp = 0.3125f;
        pv_power_pi_ki = 0.1f;
        pv_power_pi_kd = 0.0f;
        pv_power_pi_umax = 11500.0f;
        pv_power_pi_umin = -11500.0f;
        pi_step((PI_BLK *)&pv_power_pi_e); /* the PI block at 0xD43A (a_ram.h names its words) */
        if (pv_power_pi_u > 11500.0f)
            pv_power_pi_u = 11500.0f;
        else if (pv_power_pi_u < -11500.0f)
            pv_power_pi_u = -11500.0f;
        pv_i_raw_max = pv1_current_raw > pv2_current_raw ? pv1_current_raw : pv2_current_raw;
        pv_current_pi_e = pv_i_raw_max + -1800.0f;
        pv_current_pi_kp = 3.125f;
        pv_current_pi_ki = 1.0f;
        pv_current_pi_kd = 0.0f;
        pv_current_pi_umax = 20000.0f;
        pv_current_pi_umin = -20000.0f;
        pi_step((PI_BLK *)&pv_current_pi_e);
        if (pv_current_pi_u > 20000.0f)
            pv_current_pi_u = 20000.0f;
        else if (pv_current_pi_u < -20000.0f)
            pv_current_pi_u = -20000.0f;
        dc_pi_out_max = pv_power_pi_u > pv_current_pi_u ? pv_power_pi_u : pv_current_pi_u;
        dc_power_limit = rated_ac_power - dc_pi_out_max;
    } else {
        pv_power_pi_i = 0.0f;
        dc_power_limit = rated_ac_power;
        pv_power_pi_u = 0.0f;
        pv_current_pi_i = 0.0f;
        pv_current_pi_u = 0.0f;
    }
    rate_limit(&dc_power_limit, dc_power_limit_prev, 100.0f, -100.0f);
    limitf(&dc_power_limit, rated_ac_power, 0.0f);
    dc_power_limit_prev = dc_power_limit;
    if (r > dc_power_limit)
        r = dc_power_limit;
    limitf(&r, rated_ac_power, 0.0f);
    return r;
}

/* A temperature derate: 8 % of rated per degree above the heatsink or charger derate point
   (heatsink_temp_derate, charger_temp_derate), down to a 10 % floor. */
float temp_derate_limit(void) {
    float r = rated_ac_power;
    float a = 0.0f, b = 0.0f, k = 0.0f, t;
    if (heatsink_temp > heatsink_temp_derate)
        a = heatsink_temp - heatsink_temp_derate;
    if (charger_temp > charger_temp_derate)
        b = charger_temp - charger_temp_derate;
    t = max3f(a, b, 0.0f);
    if (t > 0.0f) {
        k = 100.0f - t * 0.1f * 8.0f;
        limitf(&k, 100.0f, 10.0f);
        r = k * 0.01f * r;
    }
    return r;
}

/* Volt-watt: the grid voltage filtered 0.9/0.1. Thresholds: European group 1.10 Vn, step 0.04 % of
   rated per tick; 8, 12, 16, 17 1.13 Vn, 0.029 %; code 3 1.10 Vn (253.0 V) to 260.0 V, 1 %; code
   19 242.0 V to 250.0 V, 1 %; others 1.10 Vn, 0.029 %. Above it, with 0xD7C2 bit 6 (HR176 bit 1):
   codes 3 and 19 slew every 5 ticks to rated x (1 - 0.8 (V - start) / (top - start)), 20 % at the
   top (no limit while the under-frequency response runs); others step down every 5 ticks in
   op_state 5 (from the present limit) and up in op_state 6; 10..100 % of rated. Between 230.0 V
   and the threshold it steps back up. Below grid_v_low_derate it holds the current: P = V x rated
   / grid_v_low_derate. */
float volt_watt_limit(void) {
    float step, thr, top = 0.0f, t = 0.0f;
    vw_v_in = grid_voltage;
    vw_v_in2 = grid_voltage;
    vw_v_filt = vw_v_filt * 0.9f + vw_v_in * 0.1f;
    vw_v_filt2 = vw_v_filt2 * 0.9f + vw_v_in2 * 0.1f;
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        if (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
            grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
            grid_code == 26 || grid_code == 28) {
            thr = grid_voltage_nominal * 1.1f;
            step = rated_ac_power * 4e-4f;
        } else if (grid_code == 12 || grid_code == 8 || grid_code == 16 || grid_code == 17) {
            thr = grid_voltage_nominal * 1.13f;
            step = rated_ac_power * 2.9e-4f;
        } else if (grid_code == 3) {
            top = 2600.0f;
            thr = grid_voltage_nominal * 1.1f;
            step = rated_ac_power * 0.01f;
        } else if (grid_code == 19) {
            top = 2500.0f;
            thr = 2420.0f;
            step = rated_ac_power * 0.01f;
        } else {
            thr = grid_voltage_nominal * 1.1f;
            step = rated_ac_power * 2.9e-4f;
        }
        if (vw_v_filt > thr) {
            if (grid_func_flags.bit.b6 == 1) {
                if (grid_code == 3 || grid_code == 19) {
                    if (++vw_tick >= 5) {
                        vw_tick = 0;
                        if (freq_resp.uf_active == 0) {
                            if (vw_v_filt < top)
                                t = rated_ac_power *
                                    (1.0f - (vw_v_filt - thr) * 0.8f / (top - thr));
                            else
                                t = rated_ac_power * 0.2f;
                        } else {
                            vw_p_limit = rated_ac_power;
                        }
                        if (vw_p_limit > t + step)
                            vw_p_limit = vw_p_limit - step;
                        else if (vw_p_limit < t - step)
                            vw_p_limit = vw_p_limit + step;
                        else
                            vw_p_limit = t;
                    }
                } else if (op_state == OP_GRID_PV) {
                    if (++vw_tick >= 5) {
                        vw_tick = 0;
                        if (vw_latched == 0) {
                            vw_latched = 1;
                            vw_p_limit = out_power_limit;
                        }
                        vw_p_limit = vw_p_limit - step;
                    }
                } else if (++vw_tick >= 5) {
                    vw_tick = 0;
                    vw_latched = 0;
                    vw_p_limit = vw_p_limit + step;
                }
                limitf(&vw_p_limit, rated_ac_power, rated_ac_power * 0.1f);
            } else {
                vw_p_limit = rated_ac_power;
            }
        } else if (vw_v_filt > 2300.0f) {
            if (++vw_tick > 5) {
                vw_p_limit = vw_p_limit + step;
                vw_tick = 0;
                vw_latched = 0;
                if (vw_p_limit > rated_ac_power)
                    vw_p_limit = rated_ac_power;
            }
        } else if (vw_v_filt < grid_v_low_derate) {
            if (grid_v_low_derate != 0.0f)
                vw_p_limit = vw_v_in2 * (rated_ac_power / grid_v_low_derate);
        } else {
            vw_p_limit = rated_ac_power;
        }
    } else {
        vw_p_limit = rated_ac_power;
    }
    if (ufr_latched == 1 && (grid_code == 3 || grid_code == 19))
        return rated_ac_power;
    return vw_p_limit;
}

/* The export limit (0xD7C2 bit 0, HR178) in op_states 5/6 when not charging: the target is the
   inverter power + the ARM's battery setpoint + grid_port_max_power (0xD8BA), clamped to rated and
   filtered 0.03 per tick, and slew-limited at HR306 after a re-enable for codes 3 and 19. While CF19
   bit 2 is set it rises 0.5 % of rated per tick to |grid_port_max_power|. */
float export_power_limit(void) {
    exp_p_default = rated_ac_power;
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        if (grid_func_flags.bit.b0 == 1 && (batt_state_cmd & 0xFFFD) == 0) {
            if (batt_blocks.bit.b2 == 0) {
                inv_power_abs = fabs(inv_power);
                exp_target_raw = inv_power_abs + arm_batt_setpoint + grid_port_max_power;
                limitf(&exp_target_raw, rated_ac_power, 0.0f);
                exp_target_filt = exp_target_filt + (exp_target_raw - exp_target_filt) * 0.03f;
                limitf(&exp_target_filt, rated_ac_power, 0.0f);
                exp_p_limit = exp_target_filt;
                if (exp_ramp_active == 1) {
                    exp_ramp_power =
                        exp_ramp_power + power_ramp_rate * rated_ac_power * 0.2f * 1.6666667e-6f;
                    if (exp_ramp_power > rated_ac_power) {
                        exp_ramp_active = 0;
                        exp_ramp_power = rated_ac_power;
                    }
                    if (exp_p_limit > exp_ramp_power)
                        exp_p_limit = exp_ramp_power;
                }
            } else {
                exp_p_limit = exp_p_limit + rated_ac_power * 0.005f;
                if (exp_p_limit > fabs(grid_port_max_power)) {
                    exp_p_limit = fabs(grid_port_max_power);
                    if (grid_code == 3 || grid_code == 19) {
                        exp_ramp_active = 1;
                        exp_ramp_power = exp_p_limit;
                    }
                }
            }
        } else {
            exp_p_limit = exp_p_default;
        }
    } else {
        exp_p_limit = exp_p_default;
    }
    return exp_p_limit;
}

/* The import limit (0xD7C0 bit 6, HR102 bit 6) in op_states 5/6 outside a calibration: the
   allowed import power is V x HR101 x 0.99 less the inverter power and the battery setpoint, in
   0..1.07 x rated, filtered 0.015 per tick. Returns it as a peak current bound (0.1 A). */
float grid_import_current_limit(void) {
    float k = grid_import_amps;
    if ((op_state == OP_GRID_PV || op_state == OP_GRID_BATT) && (eps_cfg_bits.all & 0x40) &&
        !(batt_blocks.all & 4) && batt_cal_stage == 0) {
        imp_p_target = -((inv_power + batt_power_target) - grid_voltage * k * 0.99f);
        limitf(&imp_p_target, rated_ac_power * 1.07f, 0.0f);
        imp_p_filt = imp_p_filt + (imp_p_target - imp_p_filt) * 0.015f;
        limitf(&imp_p_filt, rated_ac_power * 1.07f, 0.0f);
        imp_p_limit = imp_p_filt;
    } else {
        imp_p_limit = rated_ac_power * 1.07f;
    }
    if (grid_voltage != 0.0f)
        imp_i_scale = 1.4142135f / grid_voltage;
    return imp_p_limit * imp_i_scale * 10.0f;
}

/* The power ceiling after a start, in op_states 5/6: rated / 6000 per tick (2 minutes to rated);
   rated / 1200 in the aging test; with slow_ramp_req, every 5 ticks HR306 (HR302 once run_status
   bit 2 records a grid fault) per mille of rated (of batt_chg_power_max in op_state 6) per minute. At rated it sets
   run_status bit 4 and ends the slow ramp once the PV detection is done. */
float soft_start_ramp(void) {
    float inc;
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        if (aging_test_enable == 1) {
            soft_start_power = soft_start_power + rated_ac_power * 500.0f * 1.6666667e-6f;
        } else if (slow_ramp_req == 1 && (pv_detect_done == 1 || op_state == OP_GRID_BATT)) {
            if (++soft_start_tick >= 5) {
                soft_start_tick = 0;
                if (run_status_bits.bit.b2 == 1) {
                    if (op_state == OP_GRID_BATT)
                        inc = power_ramp_rate_reconnect * batt_chg_power_max * 1.6666667e-6f;
                    else
                        inc = power_ramp_rate_reconnect * rated_ac_power * 1.6666667e-6f;
                } else {
                    if (op_state == OP_GRID_BATT)
                        inc = power_ramp_rate * batt_chg_power_max * 1.6666667e-6f;
                    else
                        inc = power_ramp_rate * rated_ac_power * 1.6666667e-6f;
                }
                soft_start_power = soft_start_power + inc;
            }
        } else {
            soft_start_power = soft_start_power + rated_ac_power * 1.6666667e-4f;
        }
        if (soft_start_power > rated_ac_power) {
            soft_start_power = rated_ac_power;
            run_status_bits.bit.b4 = 1;
            if (pv_detect_done == 1)
                slow_ramp_req = 0;
        }
    } else {
        soft_start_power = 0.0f;
    }
    return soft_start_power;
}

/* The active power limit out_power_limit (0xCF2C): the smallest of the soft start, the P(f)
   limit, the temperature derate, volt-watt, the DC side limit and rated, then of P beside Q and
   the HR50 rate limit (raised by the under-frequency response). The grid current bounds (0.1 A
   peak, positive = export): hi from the export limit when that is lower, else from the power
   limit plus p_limit_trim (0xD3F6, task 1's PI on the power headroom), at least 0.5 A; lo = -hi or the import limit, at most -0.5 A. With the
   charge target on (HR20), the ramp done and SoC at the target (HR116), 0xD7C0 bit 12 is cleared
   and lo is -0.5 A, so the battery is not charged from the grid. The bounds are written with
   interrupts off. */
void power_limits_update(void) {
    float hi, lo;
    lim_soft_start = soft_start_ramp();
    lim_dc_side = dc_side_power_limit();
    lim_temp = temp_derate_limit();
    lim_volt_watt = volt_watt_limit();
    lim_rated = rated_ac_power;
    lim_ofr = ofr_power_limit();
    lim_export = export_power_limit();
    out_power_limit = min7f(lim_soft_start, lim_ofr, lim_temp, lim_volt_watt, lim_dc_side,
                            rated_ac_power, lim_rated);
    lim_by_q = p_limit_by_q();
    lim_rate = active_power_rate_limit();
    lim_rate = ufr_power_limit(lim_rate);
    out_power_limit = min3f(out_power_limit, lim_by_q, lim_rate);
    if (grid_voltage != 0.0f) {
        if (lim_export < out_power_limit) {
            hi = lim_export * 1.4142135f * 10.0f / grid_voltage;
            if (hi < 5.0f)
                hi = 5.0f;
            lo = -((out_power_limit + p_limit_trim) * 1.4142135f * 10.0f) / grid_voltage;
            if (lo > -5.0f)
                lo = -5.0f;
        } else {
            hi = (out_power_limit + p_limit_trim) * 1.4142135f * 10.0f / grid_voltage;
            if (hi < 5.0f)
                hi = 5.0f;
            lo = -hi;
        }
        {
            float t = grid_import_current_limit();
            if (lo < -t)
                lo = -t;
        }
        if (lo > -5.0f)
            lo = -5.0f;
        if (enable_charge_target == 1 && run_status_bits.bit.b4 == 1 &&
            bms_soc >= charge_target_soc)
            eps_cfg_bits.bit.b12 = 0;
        else
            eps_cfg_bits.bit.b12 = 1;
        if (eps_cfg_bits.bit.b12 == 0)
            lo = -5.0f;
        DINT;
        i_ref_max = hi;
        i_ref_min = lo;
        EINT;
    }
}

/* The reactive power. q_max is 60 % of rated (sqrt(S^2 - P^2) for 3/19 under the latched
   under-frequency response). In op_states 5/6, by Q mode (HR122): 1 fixed PF, 2 cos phi(P), 3 PF(P)
   curve, 4/5 fixed Q % (two signs), 6 Q(U) with power lock-in, 8/9 the fixed Q(U) curves, else 0;
   limited to +-q_max, plus the filter capacitor's own reactive power 2 pi f C V^2 with C = 8.4 uF
   (by the constant). The reactive current (0.1 A peak) Q x sqrt 2 x 10 / V, limited to
   +-grid_current_max and slewed 0.1 A per tick, goes to q_current_ref with interrupts off. */
void reactive_power_update(void) {
    float k = 0.0f;
    q_base_power = rated_ac_power;
    if (ufr_latched == 1 && (grid_code == 3 || grid_code == 19)) {
        if (q_base_power <= out_power_limit)
            q_max = 0.0f;
        else
            q_max = sqrtf(q_base_power * q_base_power - out_power_limit * out_power_limit);
    } else {
        q_max = rated_ac_power * 0.6f;
    }
    limitf(&q_max, rated_ac_power * 0.6f, 0.0f);
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        q_cap_comp = grid_freq * 6.2831855f * grid_voltage * grid_voltage * 8.4e-6f * 0.001f;
        switch (q_mode) {
        default:
            q_mode_out = 0.0f;
            break;
        case 8:
        case 9:
            q_mode_out = q_of_u_fixed(grid_voltage);
            break;
        case 6:
            q_mode_out = q_of_u_lockin(grid_voltage);
            break;
        case 4:
        case 5:
            q_mode_out = q_fixed_percent();
            break;
        case 3:
            q_mode_out = q_pf_curve();
            break;
        case 2:
            q_mode_out = q_cosphi_p();
            break;
        case 1:
            q_mode_out = q_fixed_pf();
            break;
        }
        limitf(&q_mode_out, q_max, -q_max);
        q_setpoint = q_cap_comp + q_mode_out;
    } else {
        q_setpoint = 0.0f;
        q_cap_comp = 0.0f;
        q_mode_out = 0.0f;
    }
    if (grid_voltage != 0.0f)
        k = q_setpoint * 1.4142135f * 10.0f / grid_voltage;
    limitf(&k, grid_current_max, -grid_current_max);
    q_current_ref_prev = q_current_ref;
    rate_limit(&k, q_current_ref, 1.0f, -1.0f);
    DINT;
    q_current_ref = k;
    EINT;
}

/* Fixed power factor (Q mode 1): below 4 % load Q is 0. A PF code power_factor_set (HR52) of
   1..2500 gives cos phi = 1 - code / 10000 (1.0 down to 0.75); 17500..20000 gives the same with the
   other sign; other codes give 1. Code 7 with P > 0 adjusts it by voltage (pf_of_u_adjust).
   Q = P tan phi, plus 100 W when |P| is over 300 W in the code's direction. */
float q_fixed_pf(void) {
    float off;
    pf_band_hi = 20000;
    pf_band_lo = 17500;
    pf_band_cap = 2500;
    pf_flag_unused = 0;
    if (load_percent < 4) {
        pf_cosphi = 1.0f;
        q_fixed_pf_out = 0.0f;
    } else {
        if ((power_factor_set != 0 && power_factor_set <= 2500) ||
            (power_factor_set >= pf_band_lo && power_factor_set <= pf_band_hi)) {
            pf_cosphi = (10000.0f - power_factor_set) * 0.0001f;
            if (grid_code == 7 && inv_power > 0.0f)
                pf_cosphi = pf_of_u_adjust(grid_voltage, pf_cosphi);
        } else {
            pf_cosphi = 1.0f;
        }
        if (pf_cosphi == 0.0f) {
            q_fixed_pf_out = 0.0f;
        } else {
            if (power_factor_set > 10000) {
                if (inv_power > 3000.0f)
                    off = 1000.0f;
                else
                    off = 0.0f;
            } else {
                if (inv_power < -3000.0f)
                    off = 1000.0f;
                else
                    off = 0.0f;
            }
            q_fixed_pf_out = inv_power * (sqrtf(1.0f - pf_cosphi * pf_cosphi) / pf_cosphi) + off;
        }
    }
    return q_fixed_pf_out;
}

/* cos phi(P) (Q mode 2) from the load in %: codes 3, 7, 19 PF = 1.05 - load / 1000, 0.95..1
   (PF 1 up to 50 %, 0.95 at full load); code 4 and the European group 1.1 - 2 (load + 1) / 1000,
   0.9..1 (0.9 at full load), and only while the grid voltage is above HR144 (off below HR145). Active from 50 % load. Code 7 smooths the result (q_pt1_response). */
float q_cosphi_p(void) {
    float i;
    cosphi_p_pct = load_percent;
    if (grid_code == 4 || grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
        grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
        grid_code == 26 || grid_code == 28) {
        if (grid_voltage > pfp_lock_in_v && cosphi_p_pct >= 50.0f)
            cosphi_p_active = 1;
        else if (grid_voltage < pfp_lock_out_v || cosphi_p_pct < 50.0f)
            cosphi_p_active = 0;
    } else if (grid_code == 3 || grid_code == 1 || grid_code == 7 || grid_code == 18 ||
               grid_code == 19 || grid_code == 6 || grid_code == 0) {
        if (cosphi_p_pct < 50.0f)
            cosphi_p_active = 0;
        else
            cosphi_p_active = 1;
    }
    if (cosphi_p_active == 1) {
        i = inv_power;
        if (grid_code == 3 || grid_code == 7 || grid_code == 19) {
            cosphi_p_pf = (1050.0f - cosphi_p_pct) * 0.001f;
            limitf(&cosphi_p_pf, 1.0f, 0.95f);
        } else if (grid_code == 4 || grid_code == 2 || grid_code == 13 || grid_code == 9 ||
                   grid_code == 11 || grid_code == 24 || grid_code == 23 || grid_code == 22 ||
                   grid_code == 25 || grid_code == 26 || grid_code == 28) {
            cosphi_p_pf = (1100.0f - (cosphi_p_pct + 1.0f) * 2.0f) * 0.001f;
            limitf(&cosphi_p_pf, 1.0f, 0.9f);
        }
        if (cosphi_p_pf != 0.0f)
            q_cosphi_p_out = sqrtf(1.0f - cosphi_p_pf * cosphi_p_pf) / cosphi_p_pf * i;
    } else {
        cosphi_p_pf = 1.0f;
        q_cosphi_p_out = 0.0f;
    }
    if (grid_code == 7)
        q_cosphi_p_out = q_pt1_response(q_cosphi_p_out);
    return q_cosphi_p_out;
}

/* Interpolates between two PF points in the code's encoding: 0..2000 means 1.0 down to 0.8 on one
   side (turned into 10000 - code), 18000..20000 the other side. Two points on the same side
   interpolate directly; across unity it finds the crossing point xm and goes through 10000 and
   20000. */
float pf_curve_interp(float x, float x0, float y0, float x1, float y1) {
    float xm, m, r;

    r = 0.0f;
    xm = 0.0f;
    m = 0.0f;
    if (y0 >= 0.0f && y0 <= 2000.0f)
        y0 = 10000.0f - y0;
    if (y1 >= 0.0f && y1 <= 2000.0f)
        y1 = 10000.0f - y1;
    if ((y0 <= 10000.0f && y1 <= 10000.0f) || (y0 >= 18000.0f && y1 >= 18000.0f)) {
        r = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
    } else {
        if (y0 <= 10000.0f && y1 >= 18000.0f)
            xm = x1 - (x1 - x0) * (20000.0f - y1) / (30000.0f - y0 - y1);
        else if (y0 >= 18000.0f && y1 <= 10000.0f)
            xm = x1 - (x1 - x0) * (10000.0f - y1) / (30000.0f - y0 - y1);
        if (x >= x0 && x <= xm) {
            if (y0 <= 10000.0f && y1 >= 18000.0f)
                m = 10000.0f;
            else if (y0 >= 18000.0f && y1 <= 10000.0f)
                m = 20000.0f;
            r = y0 + (m - y0) * (x - x0) / (xm - x0);
        } else if (x > xm && x <= x1) {
            if (y0 <= 10000.0f && y1 >= 18000.0f)
                m = 20000.0f;
            else if (y0 >= 18000.0f) {
                if (y1 < 10000.0f)
                    m = 10000.0f;
                else
                    m = y1;
            }
            r = m + (y1 - m) * (x - xm) / (x1 - xm);
        }
    }
    return r;
}

/* PF(P) curve (Q mode 3): four points, power % in D846..D849 and PF codes in D84A..D84D. It acts
   only when the first two power points are 100 or less, and nothing but settings_defaults (255)
   ever writes D846..D849, so this mode always gives Q = 0 (docs/findings-control.md). For code 4
   it also needs the voltage lock-in (HR144/HR145). */
float q_pf_curve(void) {
    if (grid_code == 4) {
        if (grid_voltage > pfp_lock_in_v)
            pfp_lock = 1;
        else if (grid_voltage < pfp_lock_out_v)
            pfp_lock = 0;
    }
    if ((pfp_lock == 1 || grid_code != 4) && pf_curve_p1 <= 100 && pf_curve_p2 <= 100 &&
        load_percent >= 5) {
        if (pf_curve_pf1 > 2000) {
            if (pf_curve_pf1 < 10000)
                pf_curve_pf1 = 2000;
            else if (pf_curve_pf1 > 10000 && pf_curve_pf1 < 18000)
                pf_curve_pf1 = 18000;
        }
        if (pf_curve_pf2 > 2000) {
            if (pf_curve_pf2 < 10000)
                pf_curve_pf2 = 2000;
            else if (pf_curve_pf2 > 10000 && pf_curve_pf2 < 18000)
                pf_curve_pf2 = 18000;
        }
        if (pf_curve_pf3 > 2000) {
            if (pf_curve_pf3 < 10000)
                pf_curve_pf3 = 2000;
            else if (pf_curve_pf3 > 10000 && pf_curve_pf3 < 18000)
                pf_curve_pf3 = 18000;
        }
        if (pf_curve_pf4 > 2000) {
            if (pf_curve_pf4 < 10000)
                pf_curve_pf4 = 2000;
            else if (pf_curve_pf4 > 10000 && pf_curve_pf4 < 18000)
                pf_curve_pf4 = 18000;
        }
        if (load_percent <= pf_curve_p2)
            pfp_pf_code =
                pf_curve_interp(load_percent, pf_curve_p1, pf_curve_pf1, pf_curve_p2, pf_curve_pf2);
        else if (pf_curve_p3 <= 100 && load_percent > pf_curve_p2 && load_percent <= pf_curve_p3)
            pfp_pf_code =
                pf_curve_interp(load_percent, pf_curve_p2, pf_curve_pf2, pf_curve_p3, pf_curve_pf3);
        else if (pf_curve_p4 <= 100 && load_percent > pf_curve_p3)
            pfp_pf_code =
                pf_curve_interp(load_percent, pf_curve_p3, pf_curve_pf3, pf_curve_p4, pf_curve_pf4);
        if (pfp_pf_code < 8000.0f)
            pfp_pf_code = 8000.0f;
        else if (pfp_pf_code > 10000.0f && pfp_pf_code < 18000.0f)
            pfp_pf_code = 18000.0f;
        if (pfp_pf_code >= 8000.0f && pfp_pf_code <= 10000.0f)
            pfp_pf = pfp_pf_code * 0.0001f;
        else if (pfp_pf_code >= 18000.0f && pfp_pf_code <= 20000.0f)
            pfp_pf = (10000.0f - pfp_pf_code) * 0.0001f;
        q_pf_curve_out = inv_power * (sqrtf(1.0f - pfp_pf * pfp_pf) / pfp_pf);
    } else {
        pfp_pf = 1.0f;
        q_pf_curve_out = 0.0f;
    }
    return q_pf_curve_out;
}

/* Fixed Q (modes 4 and 5): HR51 % of rated (1..60; 0 or over 60 gives 0), positive in mode 4,
   negative in mode 5. Code 7 passes it through q_restore_response. */
float q_fixed_percent(void) {
    if (reactive_power_rate == 0) {
        q_fixed_pct = 0.0f;
    } else if (reactive_power_rate > 60) {
        q_fixed_pct = 0.0f;
    } else if (q_mode == 4) {
        q_fixed_pct = q_base_power * reactive_power_rate * 0.01f;
    } else {
        q_fixed_pct = -q_base_power * reactive_power_rate * 0.01f;
    }
    if (grid_code == 7)
        q_fixed_pct = q_restore_response(q_fixed_pct);
    return q_fixed_pct;
}

/* Q modes 8 and 9, European group only, on the voltage filtered 0.9/0.1, with 43.6 % of rated
   (sin phi at PF 0.9): mode 9 gives -43.6 % from 195.5 to 241.5 V, falling to 0 at 253.0 V; mode 8
   gives +43.6 % from 218.5 to 253.0 V, rising from 0 at 207.0 V; 0 elsewhere. */
float q_of_u_fixed(float x) {
    float r = 0.0f, f;
    f = qu_fixed_v_filt * 0.9f + x * 0.1f;
    qu_fixed_v_filt = f;
    if (grid_code == 2 || grid_code == 9 || grid_code == 11 || grid_code == 13 || grid_code == 24 ||
        grid_code == 23 || grid_code == 22 || grid_code == 25 || grid_code == 26 ||
        grid_code == 28) {
        if (q_mode == 9) {
            if (f >= 2415.0f && f < 2530.0f)
                r = -(q_base_power * 0.436f) * (2530.0f - qu_fixed_v_filt) / 115.0f;
            else if (f >= 1955.0f && f < 2415.0f)
                r = -q_base_power * 0.436f;
            else
                r = 0.0f;
        } else if (q_mode == 8) {
            if (f >= 2185.0f && f <= 2530.0f)
                r = q_base_power * 0.436f;
            else if (f >= 2070.0f && f < 2185.0f)
                r = q_base_power * 0.436f * (qu_fixed_v_filt - 2070.0f) / 115.0f;
            else
                r = 0.0f;
        } else {
            r = 0.0f;
        }
    }
    return r;
}

/* One ramp step of the auto test value: steps 1 and 5 +2.0 V (unless held), 2 -2.0 V, 6 -2.5 V,
   3 and 7 +0.02 Hz, 4 and 8 -0.02 Hz. */
void autotest_ramp(void) {
    switch (autotest_step) {
    case 4:
    case 8:
        if (autotest_value >= 2)
            autotest_value -= 2;
        break;
    case 6:
        if (autotest_value >= 25)
            autotest_value -= 25;
        break;
    case 1:
    case 5:
        if (autotest_hold == 0)
            autotest_value += 20;
        break;
    case 2:
        if (autotest_hold == 0 && autotest_value >= 20)
            autotest_value -= 20;
        break;
    case 3:
    case 7:
        autotest_value += 2;
        break;
    }
}

/* The CEI 0-21 protection auto test (code 4), started by the ARM (auto_test_state 1). Eight steps,
   each a test of one threshold with its time: 1 HR72 (59.S2), 2 HR63 (27.S1), 3 HR74 (81>.S2),
   4 HR73 (81<.S2), 5 HR64 (59.S1), 6 HR71 (27.S2), 7 HR66 (81>.S1), 8 HR65 (81<.S1). Each step waits
   250 ticks (5 s) on the grid, then ramps the test value one step every 50 ticks (1 s) from the
   measured value; the protection functions use the test value in place of the measurement. A trip
   that takes the inverter off the grid ends the step: result 2 when the trip time is within
   27 ms of HR time x 20 + 20 ms, 4 when outside it or after 300 ramp steps, 3 when nothing tripped
   or the wait ran out. After step 8 the result is 5 (done). The ARM reads the result on D frame
   page 4. */
void autotest_run(void) {
    if (grid_code != 4)
        return;
    if (auto_test_state == 0) {
        autotest_value = 0;
        autotest_step = 0;
        autotest_result = 0;
    } else if (auto_test_state == 1) {
        auto_test_state = 2;
        autotest_result = 0;
        autotest_ramp_cnt = 0;
        autotest_elapsed_ms = 0;
        autotest_trip_ms = 0;
        autotest_trip_value = 0;
        autotest_value = 0;
        autotest_step = 0;
        autotest_wait_cnt = 0;
        autotest_phase = 0;
    } else if (auto_test_state == 2) {
        if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
            autotest_wait_cnt = 0;
            if (autotest_phase < 250) {
                if (autotest_phase == 0) {
                    if (autotest_step <= 8U)
                        autotest_step++;
                    autotest_result = 0;
                    autotest_elapsed_ms = 0;
                    autotest_time_tol = 27;
                    autotest_hold = 0;
                    switch (autotest_step) {
                    default:
                        autotest_result = 5;
                        autotest_hold = 0;
                        break;
                    case 8:
                        autotest_expect_ms = f_ac_low_out_time * 20 + 20;
                        autotest_value = grid_freq;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = f_ac_low_out;
                        break;
                    case 7:
                        autotest_expect_ms = f_ac_high_out_time * 20 + 20;
                        autotest_value = grid_freq;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = f_ac_high_out;
                        break;
                    case 6:
                        autotest_value = grid_voltage;
                        autotest_expect_ms = v_ac_low_in_time * 20 + 20;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = v_ac_low_in;
                        break;
                    case 5:
                        autotest_value = grid_voltage;
                        autotest_expect_ms = v_ac_high_out_time * 20 + 20;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = v_ac_high_out;
                        break;
                    case 4:
                        autotest_expect_ms = f_ac_low_in_time * 20 + 20;
                        autotest_value = grid_freq;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = f_ac_low_in;
                        break;
                    case 3:
                        autotest_expect_ms = f_ac_high_in_time * 20 + 20;
                        autotest_value = grid_freq;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = f_ac_high_in;
                        break;
                    case 2:
                        autotest_value = grid_voltage;
                        autotest_expect_ms = v_ac_low_out_time * 20 + 20;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = v_ac_low_out;
                        break;
                    case 1:
                        autotest_value = grid_voltage;
                        autotest_expect_ms = v_ac_high_in_time * 20 + 20;
                        autotest_expect_time = autotest_expect_ms;
                        autotest_limit = v_ac_high_in;
                        break;
                    }
                } else if (autotest_phase == 249) {
                    autotest_result = 1;
                }
                autotest_phase++;
                autotest_ramp_cnt = 0;
            } else {
                if (autotest_phase == 250) {
                    autotest_phase++;
                    autotest_ramp();
                    if (++autotest_ramp_cnt > 300) {
                        autotest_result = 4;
                        autotest_trip_value = autotest_value;
                        autotest_trip_ms = autotest_elapsed_ms;
                    }
                } else if (autotest_phase < 300) {
                    autotest_phase++;
                } else {
                    autotest_phase = 250;
                }
            }
        } else {
            autotest_phase = 0;
            if (fault_grid1.bit.b12 | fault_grid1.bit.b15) {
                if (autotest_ramp_cnt == 0)
                    autotest_result = 3;
                else if (autotest_time_tol + autotest_trip_ms < autotest_expect_ms ||
                         autotest_expect_ms + autotest_time_tol < autotest_trip_ms)
                    autotest_result = 4;
                else
                    autotest_result = 2;
                if (++autotest_wait_cnt > 500)
                    autotest_result = 3;
            } else if (op_state != OP_START) {
                if (++autotest_wait_cnt > 9000)
                    autotest_result = 3;
            }
        }
    }
}

/* The 10-minute mean of the grid voltage (most codes; code 11 is tested twice in the list):
   every 150 ticks (3 s) the mean of those ticks goes into a ring of 200 slots, and the mean of the
   ring is grid_v_10min (0.1 V). The ring starts at 230.0 V. */
void grid_v_10min_update(void) {
    unsigned long sum;
    unsigned int i;
    if (grid_code == 1 || grid_code == 4 || grid_code == 7 || grid_code == 2 || grid_code == 11 ||
        grid_code == 3 || grid_code == 13 || grid_code == 9 || grid_code == 11 || grid_code == 18 ||
        grid_code == 28 || grid_code == 24 || grid_code == 23 || grid_code == 22 ||
        grid_code == 25 || grid_code == 26 || grid_code == 19 || grid_code == 6 || grid_code == 0) {
        v10_window = 200;
        v10_sum += (unsigned int)grid_voltage;
        if (++v10_sample_cnt >= 150) {
            if (++v10_index >= 200)
                v10_index = 0;
            v10_slots[v10_index] = udiv32_16(v10_sum, v10_sample_cnt);
            v10_sample_cnt = 0;
            v10_sum = 0;
            sum = 0;
            i = 0;
            if (v10_window)
                do {
                    sum += v10_slots[i];
                    i++;
                } while (i < v10_window);
            grid_v_10min = (sum + (v10_window >> 1)) / v10_window;
        }
    }
}

/* DC injection, in op_states 5/6 with dci_comp_enable (0xD8D8): after 20 s the DC component is
   filtered 0.9/0.1 and every 50 ticks (1 s) the correction dci_offset (a bridge voltage offset
   added in grid_ictl, +-200) steps by 1 against it when it is over 25. After 60 s, |DC| over
   dci_limit (Italy: over 1000) for dci_trip_time (dci_trip_time_it) ticks sets CF17 bit 0; the
   counts fall by one per tick below. Off the grid the correction is 0 and the fault clears after
   1501 ticks (30 s). */
void dci_control(void) {
    if ((op_state == OP_GRID_PV || op_state == OP_GRID_BATT) && dci_comp_enable == 1) {
        if (++dci_tick > 1000) {
            dci_filt = dci_filt * 0.9f + dc_injection * 0.1f;
            if (dci_tick > 1500) {
                dci_tick = 1450;
                if (dci_filt > 25.0f)
                    dci_offset_cmd = dci_offset_cmd + -1.0f;
                else if (dci_filt < -25.0f)
                    dci_offset_cmd = dci_offset_cmd + 1.0f;
                if (dci_offset_cmd > 200.0f)
                    dci_offset_cmd = 200.0f;
                else if (dci_offset_cmd < -200.0f)
                    dci_offset_cmd = -200.0f;
            }
        }
        if (++dci_settle_cnt >= 3000) {
            dci_settle_cnt = 3000;
            if (fabs(dci_filt) > 1000.0f && grid_code == 4) {
                if (++dci_trip_cnt_it >= dci_trip_time_it) {
                    fault_stop1.all |= 1;
                    dci_trip_cnt_it = 0;
                }
            } else if (fabs(dci_filt) > dci_limit) {
                if (++dci_trip_cnt >= dci_trip_time) {
                    fault_stop1.all |= 1;
                    dci_trip_cnt = 0;
                }
            } else {
                if (dci_trip_cnt != 0)
                    dci_trip_cnt--;
                if (dci_trip_cnt_it != 0)
                    dci_trip_cnt_it--;
            }
        }
    } else {
        dci_tick = 0;
        dci_settle_cnt = 0;
        dci_offset_cmd = 0.0f;
        if ((fault_stop1.all & 1) && ++dci_trip_cnt > 1500) {
            fault_stop1.all &= 0xFFFE;
            dci_trip_cnt = 0;
        }
    }
    DINT;
    dci_offset = dci_offset_cmd;
    EINT;
}

/* RoCoF: sums the change of the input over n + 1 samples and sets flag when the sum's magnitude is
   over thr. grid_freq_protect uses n = 25 (about 0.5 s) and thr 110 (1.10 Hz), so about 2.2 Hz/s.
   (grid.h calls the block an oscillation detector; it is a RoCoF check.) The first window after
   start compares with 0, see docs/findings-control.md. */
void rocof_detect(struct rocof_det_t *p) {
    float x, d;
    if (p->en == 1) {
        x = p->x;
        d = rocof_prev - x;
        rocof_sample_cnt++;
        rocof_sum_acc += d;
        rocof_prev = x;
        rocof_last = x;
        if (rocof_sample_cnt > p->n) {
            if (fabs(rocof_sum_acc) > p->thr)
                p->flag = 1;
            else
                p->flag = 0;
            p->sum = rocof_sum_acc;
            rocof_sample_cnt = 0;
            rocof_sum_acc = 0.0f;
        }
    } else {
        rocof_sample_cnt = 0;
        rocof_last = 0.0f;
        rocof_prev = 0.0f;
        rocof_sum_acc = 0.0f;
        p->flag = 0;
    }
}

/* Frequency protection, CF12 bit 15, once 10 grid periods have been captured and the 50/60 Hz
   detection is done. Stage 2: at or above HR74 for HR78 ticks, or at or below HR73 for HR77 ticks.
   Stage 1 (most codes; Italy only during the auto test's steps 7 and 8): HR66 for HR70 ticks, HR65
   for HR69 ticks. During the auto test the test value replaces the measured frequency, and a trip
   records the value and the time. The bit clears after 250 ticks (5 s) inside the stage 1 window
   (stage 2 during Italian steps 7/8). The European group also runs the RoCoF check, whose flag
   sets the same bit. */
void grid_freq_protect(void) {
    unsigned int t;
    if (ecap_valid_count < 10)
        return;
    if (grid_freq_detected == 0)
        return;
    if ((autotest_step == 3 || autotest_step == 4 || autotest_step == 7 || autotest_step == 8) &&
        (op_state == OP_GRID_PV || op_state == OP_GRID_BATT)) {
        ft_test_freq = autotest_value;
        if (autotest_step == 3 || autotest_step == 4)
            freq_stage_sel = 0;
        else if (autotest_step == 7 || autotest_step == 8)
            freq_stage_sel = 3;
    } else {
        ft_test_freq = grid_freq;
        if (autotest_step == 8)
            freq_stage_sel = 0;
    }
    prot_high1 = f_ac_high_out;
    prot_low1 = f_ac_low_out;
    prot_high2 = f_ac_high_in;
    prot_low2 = f_ac_low_in;
    if (grid_code != 4)
        freq_stage_sel = 0;
    if (fault_grid1.bit.b15 == 1) {
        if ((freq_stage_sel != 3 && ft_test_freq < prot_high1 && ft_test_freq > prot_low1) ||
            (freq_stage_sel == 3 && ft_test_freq < prot_high2 && ft_test_freq > prot_low2)) {
            if (++ft_clear_cnt >= 250) {
                ft_clear_cnt = 0;
                fault_grid1.bit.b15 = 0;
            }
        } else {
            ft_clear_cnt = 0;
        }
        if (autotest_step == 3 || autotest_step == 4 || autotest_step == 7 || autotest_step == 8)
            autotest_elapsed_ms = 0;
    } else {
        if (ft_test_freq >= prot_high2 && grid_voltage != 0.0f) {
            if (++ft_high2_cnt >= f_ac_high_in_time) {
                fault_grid1.bit.b15 = 1;
                ft_high2_cnt = 0;
                if (grid_code == 7 || grid_code == 4) {
                    t = autotest_value;
                    autotest_trip_value = t;
                    autotest_trip_value2 = t;
                    autotest_trip_ms = autotest_elapsed_ms + 20;
                }
            }
        } else if (ft_test_freq <= prot_low2 && grid_voltage != 0.0f) {
            if (autotest_step != 8 && ++ft_low2_cnt >= f_ac_low_in_time) {
                fault_grid1.bit.b15 = 1;
                ft_low2_cnt = 0;
                if (grid_code == 4) {
                    t = autotest_value;
                    autotest_trip_value = t;
                    autotest_trip_value2 = t;
                    autotest_trip_ms = autotest_elapsed_ms + 20;
                }
            }
        } else {
            ft_low2_cnt = 0;
            ft_high2_cnt = 0;
            if (autotest_step == 3 || autotest_step == 4)
                autotest_elapsed_ms = 0;
        }
        if ((freq_stage_sel == 3 && grid_code == 4) || grid_code == 8 || grid_code == 12 ||
            grid_code == 2 || grid_code == 15 || grid_code == 13 || grid_code == 9 ||
            grid_code == 11 || grid_code == 16 || grid_code == 17 || grid_code == 18 ||
            grid_code == 3 || grid_code == 1 || grid_code == 7 || grid_code == 28 ||
            grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
            grid_code == 26 || grid_code == 19 || grid_code == 6 || grid_code == 0) {
            if (ft_test_freq >= prot_high1 && grid_voltage != 0.0f) {
                if (++ft_high1_cnt >= f_ac_high_out_time) {
                    ft_high1_cnt = 0;
                    fault_grid1.bit.b15 = 1;
                    if (grid_code == 4) {
                        t = autotest_value;
                        autotest_trip_value = t;
                        autotest_trip_value2 = t;
                        autotest_trip_ms = autotest_elapsed_ms + 20;
                    }
                }
            } else if (ft_test_freq <= prot_low1 && grid_voltage != 0.0f) {
                if (++ft_low1_cnt >= f_ac_low_out_time) {
                    ft_low1_cnt = 0;
                    fault_grid1.bit.b15 = 1;
                    if (grid_code == 4) {
                        t = autotest_value;
                        autotest_trip_value = t;
                        autotest_trip_value2 = t;
                        autotest_trip_ms = autotest_elapsed_ms + 20;
                    }
                }
            } else {
                ft_low1_cnt = 0;
                ft_high1_cnt = 0;
                if (autotest_step == 7 || autotest_step == 8)
                    autotest_elapsed_ms = 0;
            }
        }
    }
    if (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 || grid_code == 24 ||
        grid_code == 23 || grid_code == 22 || grid_code == 25 || grid_code == 26 ||
        grid_code == 28) {
        rocof_enable = 1;
        rocof_freq_in = grid_freq;
        rocof_window = 25.0f;
        rocof_threshold = 110;
        rocof_detect(&rocof_blk.osc);
    } else {
        rocof_trip = 0;
    }
    if (rocof_trip == 1 || fault_grid1.bit.b15 == 1)
        fault_grid1.bit.b15 = 1;
    else
        fault_grid1.bit.b15 = 0;
}

/* Voltage protection for every code but Italy, on the one-cycle RMS: below HR71 for more than
   HR75 ticks, below HR63 for more than HR67, above HR72 for HR76 ticks or more, above HR64 for
   more than HR68, sets CF12 bit 12. The 10-minute mean over HR83 for 50 ticks (1 s) does too, for
   the listed codes. */
void grid_volt_trip_std(void) {
    float v = grid_voltage_fast;
    if (v < v_ac_low_in) {
        vt_low2_cnt++;
        vt_low1_cnt++;
        vt_high1_cnt = 0;
        vt_high2_cnt = 0;
        if (vt_low2_cnt > v_ac_low_in_time) {
            fault_grid1.bit.b12 = 1;
            vt_low2_cnt = 0;
            vt_low1_cnt = 0;
        }
    } else if (v < v_ac_low_out) {
        vt_low1_cnt++;
        vt_high1_cnt = 0;
        vt_high2_cnt = 0;
        if (vt_low1_cnt > v_ac_low_out_time) {
            fault_grid1.bit.b12 = 1;
            vt_low2_cnt = 0;
            vt_low1_cnt = 0;
        }
    } else if (v > v_ac_high_in) {
        vt_high2_cnt++;
        vt_high1_cnt++;
        vt_low2_cnt = 0;
        vt_low1_cnt = 0;
        if (vt_high2_cnt >= v_ac_high_in_time) {
            vt_high1_cnt = 0;
            vt_high2_cnt = 0;
            fault_grid1.bit.b12 = 1;
        }
    } else if (v > v_ac_high_out) {
        vt_high1_cnt++;
        vt_low2_cnt = 0;
        vt_low1_cnt = 0;
        if (vt_high1_cnt > v_ac_high_out_time) {
            vt_high1_cnt = 0;
            vt_high2_cnt = 0;
            fault_grid1.bit.b12 = 1;
        }
    } else {
        vt_low2_cnt = 0;
        vt_low1_cnt = 0;
        vt_high1_cnt = 0;
        vt_high2_cnt = 0;
    }
    if (grid_code == 3 || grid_code == 2 || grid_code == 1 || grid_code == 7 || grid_code == 13 ||
        grid_code == 9 || grid_code == 11 || grid_code == 18 || grid_code == 24 ||
        grid_code == 23 || grid_code == 22 || grid_code == 25 || grid_code == 26 ||
        grid_code == 19 || grid_code == 28 || grid_code == 6 || grid_code == 0) {
        if (grid_v_10min > v_10_min_protection) {
            if (++vt_10min_cnt >= 50) {
                fault_grid1.bit.b12 = 1;
                vt_10min_cnt = 0;
            }
        } else {
            vt_10min_cnt = 0;
        }
    }
}

/* Voltage protection, every tick. With the grid lost (state_flags bit 8) it sets CF13 bit 15 and
   stops; that bit is cleared every 101 ticks otherwise. During the auto test steps 1, 2, 5, 6 the
   test value replaces the measurement. A set CF12 bit 12 clears after 101 ticks inside HR63..HR64
   with the 10-minute mean under HR83. Codes other than 4 use grid_volt_trip_std. Italy: stage 2 OV
   (HR72, HR76 ticks), stage 2 UV (HR71, HR75), stage 1 UV (HR63, HR67), and stage 1 OV as the
   10-minute mean (HR83, HR76 ticks); the instantaneous stage 1 OV (HR64) counts only in auto test
   step 5, as CEI 0-21 has 59.S1 on the 10-minute mean (inference). Last, for every code, two ticks
   over 300.0 V (276.0 V for code 18) trip at once. */
void grid_volt_protect(void) {
    unsigned int t;
    if (state_flags & 0x100) {
        fault_grid2.all |= 0x8000;
        return;
    }
    if (++hw_fault_clear_cnt > 100) {
        hw_fault_clear_cnt = 0;
        fault_grid2.all &= 0x7FFF;
    }
    if ((autotest_step == 1 || autotest_step == 2 || autotest_step == 5 || autotest_step == 6) &&
        (op_state == OP_GRID_PV || op_state == OP_GRID_BATT))
        vt_test_voltage = autotest_value;
    else
        vt_test_voltage = grid_voltage_fast;
    prot_high1 = v_ac_high_out;
    prot_low1 = v_ac_low_out;
    prot_low2 = v_ac_low_in;
    prot_high2 = v_ac_high_in;
    if (fault_grid1.bit.b12 == 1) {
        if (vt_test_voltage < prot_low1 || vt_test_voltage > prot_high1 ||
            grid_v_10min > v_10_min_protection) {
            vt_clear_cnt = 0;
        } else if (++vt_clear_cnt > 100) {
            fault_grid1.bit.b12 = 0;
            vt_clear_cnt = 0;
        }
        if (grid_code == 4 &&
            (autotest_step == 1 || autotest_step == 2 || autotest_step == 5 || autotest_step == 6))
            autotest_elapsed_ms = 0;
        return;
    }
    if (grid_code != 4) {
        grid_volt_trip_std();
    } else {
        if (vt_test_voltage > prot_high2) {
            vt_it_high_cnt++;
            vt_it_low_cnt = 0;
            if (autotest_step == 1) {
                if (vt_it_high_cnt > 2)
                    autotest_hold = 1;
                if (vt_it_high_cnt >= v_ac_high_in_time) {
                    fault_grid1.bit.b12 = 1;
                    if (grid_code == 4) {
                        t = autotest_value;
                        autotest_trip_value = t;
                        autotest_trip_value2 = t;
                        autotest_trip_ms = autotest_elapsed_ms + 20;
                    }
                    vt_it_high_cnt = 0;
                }
            } else if (autotest_step == 2 || autotest_step == 5 || autotest_step == 6) {
                autotest_elapsed_ms = 0;
            } else if (vt_it_high_cnt >= v_ac_high_in_time) {
                vt_it_high_cnt = 0;
                fault_grid1.bit.b12 = 1;
            }
        } else if (vt_test_voltage > prot_high1) {
            vt_it_low_cnt++;
            vt_it_high_cnt = 0;
            if (autotest_step == 5) {
                if (vt_it_low_cnt > 1)
                    autotest_hold = 1;
            } else {
                vt_it_low_cnt = 0;
            }
            if (autotest_step == 1) {
                autotest_elapsed_ms = 0;
            } else if (vt_it_low_cnt >= v_ac_high_out_time) {
                fault_grid1.bit.b12 = 1;
                if (grid_code == 4) {
                    t = autotest_value;
                    autotest_trip_value = t;
                    autotest_trip_value2 = t;
                    autotest_trip_ms = autotest_elapsed_ms + 20;
                }
                vt_it_low_cnt = 0;
            }
        } else if (vt_test_voltage < prot_low2) {
            vt_it_high_cnt++;
            vt_it_low_cnt = 0;
            if (vt_it_high_cnt >= v_ac_low_in_time) {
                fault_grid1.bit.b12 = 1;
                if (grid_code == 4) {
                    t = autotest_value;
                    autotest_trip_value = t;
                    autotest_trip_value2 = t;
                    autotest_trip_ms = autotest_elapsed_ms + 20;
                }
                vt_it_high_cnt = 0;
            }
        } else if (vt_test_voltage < prot_low1) {
            vt_it_low_cnt++;
            vt_it_high_cnt = 0;
            if (vt_it_low_cnt > 1)
                autotest_hold = 1;
            if (autotest_step == 6) {
                vt_it_low_cnt = 0;
                if (autotest_step == 1 || autotest_step == 2 || autotest_step == 5 ||
                    autotest_step == 6)
                    autotest_elapsed_ms = 0;
            } else if (vt_it_low_cnt >= v_ac_low_out_time) {
                fault_grid1.bit.b12 = 1;
                if (grid_code == 4) {
                    t = autotest_value;
                    autotest_trip_value = t;
                    autotest_trip_value2 = t;
                    autotest_trip_ms = autotest_elapsed_ms + 20;
                }
                vt_it_low_cnt = 0;
            }
        } else {
            vt_it_high_cnt = 0;
            vt_it_low_cnt = 0;
            autotest_hold = 0;
            if (autotest_step == 1 || autotest_step == 2 || autotest_step == 5 ||
                autotest_step == 6)
                autotest_elapsed_ms = 0;
        }
        if (grid_v_10min > v_10_min_protection) {
            if (++vt_it_10min_cnt >= v_ac_high_in_time) {
                vt_it_10min_cnt = 0;
                fault_grid1.bit.b12 = 1;
            }
        } else {
            vt_it_10min_cnt = 0;
        }
    }
    if (grid_code == 18) {
        if (grid_voltage_fast <= 2760.0f)
            return;
    } else if (grid_voltage_fast <= 3000.0f) {
        return;
    }
    if (++vt_inst_ov_cnt >= 2) {
        fault_grid1.bit.b12 = 1;
        vt_inst_ov_cnt = 0;
    }
}

/* Momentary cessation for the European group, 3, 7, 19 (0xD7C2 bit 4, HR124, and bit 15, which
   code 25 forces on), in op_states 5/6: the grid voltage at or below HR315 (setting
   fast_trip_low_v) or at or above HR316 stops the bridge at once (inv_off, interrupts off) without
   leaving the grid state; after 6 ticks back inside, state_flags bit 13 restarts the bridge. */
void grid_v_fast_trip(void) {
    unsigned int lo, hi;
    fast_trip_v_low = grid_voltage_fast;
    fast_trip_v_high = grid_voltage_fast;
    lo = fast_trip_v_low_set;
    hi = fast_trip_v_high_set;
    if (grid_code == 25)
        grid_func_flags.bit.b15 = 1;
    if (grid_code == 2 || grid_code == 13 || grid_code == 7 || grid_code == 9 || grid_code == 11 ||
        grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
        grid_code == 26 || grid_code == 3 || grid_code == 28 || grid_code == 19) {
        if ((op_state == OP_GRID_PV || op_state == OP_GRID_BATT) && grid_func_flags.bit.b4 == 1) {
            if (fast_trip_v_low <= lo && grid_func_flags.bit.b15 == 1) {
                fast_trip_low_v = 1;
                fast_trip_active = 1;
                DINT;
                inv_off();
                EINT;
            } else if (fast_trip_v_high >= hi && grid_func_flags.bit.b15 == 1) {
                fast_trip_active = 1;
                DINT;
                inv_off();
                EINT;
            } else if (fast_trip_active == 1 && ++fast_trip_release_cnt > 5) {
                fast_trip_release_cnt = 0;
                state_flags |= 0x2000;
                fast_trip_active = 0;
            }
        } else {
            fast_trip_active = 0;
            fast_trip_hold = 250;
        }
    }
}

/* Q(U) (Q mode 6) with a power lock-in: for code 4 and the European group it starts above HR142 %
   load and stops after 11 ticks below HR143; codes 0, 1, 3, 6, 7, 18, 19 always run it (6 is
   tested twice in the list); others give 0. */
float q_of_u_lockin(float x) {
    if (grid_code == 4 || grid_code == 2 || grid_code == 13 || grid_code == 11 || grid_code == 9 ||
        grid_code == 28 || grid_code == 24 || grid_code == 23 || grid_code == 22 ||
        grid_code == 25 || grid_code == 26) {
        if (load_percent > qu_lock_in_p) {
            qu_active = 1;
            qu_lockout_cnt = 0;
            goto call;
        } else if (load_percent >= qu_lock_out_p) {
            qu_lockout_cnt = 0;
        } else if (qu_active == 1) {
            if (++qu_lockout_cnt > 10) {
                qu_active = 0;
                qu_lockout_cnt = 0;
                goto zero;
            }
        }
    } else if (grid_code == 3 || grid_code == 7 || grid_code == 1 || grid_code == 6 ||
               grid_code == 18 || grid_code == 19 || grid_code == 6 || grid_code == 0) {
        qu_active = 1;
        goto call;
    } else {
        qu_active = 0;
        goto zero;
    }
    if (qu_active == 1)
    call:
        q_qu = q_of_u_curve(x);
    else
    zero:
        q_qu = 0.0f;
    return q_qu;
}

/* The Q(U) curve on the voltage filtered 0.6/0.4: 0 between HR140 (low1) and HR138 (high1);
   rising to +hi per mille of rated at HR139 (high2) and beyond, falling to -lo at HR141 (low2) and
   below. hi/lo: code 3 600/420 (60 %/42 %), code 19 600/600, others qu_q_max (0xD806, 438).
   Positive Q at high voltage is taken as absorbing (inference from the curve's shape). */
float q_of_u_curve(float x) {
    float lo, hi, r, k = 0.0f;
    if (grid_code == 3) {
        lo = 420.0f;
        hi = 600.0f;
    } else {
        if (grid_code == 19)
            lo = 600.0f;
        else
            lo = qu_q_max;
        hi = lo;
    }
    qu_v_filt = qu_v_filt * 0.6f + x * 0.4f;
    if (qu_v_filt >= qu_v_high1 || qu_v_filt <= qu_v_low1) {
        if (qu_v_filt >= qu_v_high2) {
            k = hi;
        } else if (qu_v_filt >= qu_v_high1 && qu_v_filt < qu_v_high2) {
            if (qu_v_high2 != qu_v_high1)
                k = (qu_v_filt - qu_v_high1) * hi / (qu_v_high2 - qu_v_high1);
        } else if (qu_v_filt >= qu_v_low2 && qu_v_filt <= qu_v_low1) {
            if (qu_v_low2 != qu_v_low1)
                k = (qu_v_filt - qu_v_low1) * lo / (qu_v_low1 - qu_v_low2);
        } else {
            k = -lo;
        }
        limitf(&k, hi, -lo);
        r = rated_ac_power * k * 0.001f;
        qu_k = r;
    } else {
        qu_k = k;
        r = 0.0f;
    }
    return r;
}

/* P(f) under-frequency in op_state 5 (0xD7C2 bit 12): for the European group and 7, below HR326
   for HR312 ticks, the limit x is raised by rated x HR327 / 10000 per 0.01 Hz below HR326, up to
   rated. Codes 3 and 19: the base is latched (the smaller of x and the volt-watt limit) and the
   limit rises with the same droop, never falling while latched; above 49.85 Hz for 1500 ticks
   (30 s) it ramps back down by 0.0055 % of rated per tick to the base. Also refreshes
   grid_freq_f. */
float ufr_power_limit(float x) {
    ufr_droop_k = ufr_droop * 0.0001f;
    ufr_recover_step = rated_ac_power * 5.5e-5f;
    ufr_recover_freq = 4985;
    ufr_recover_delay = 1500;
    grid_freq_f = grid_freq - 1;
    if (op_state == OP_GRID_PV && grid_func_flags.bit.b12 == 1 &&
        (grid_code == 2 || grid_code == 7 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
         grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
         grid_code == 26 || grid_code == 28)) {
        if (grid_freq_f >= ufr_start_freq) {
            ufr_delay_cnt = 0;
            ufr_active = 0;
            ufr_p_limit = x;
        } else if (ufr_active != 0) {
            ufr_p_limit = x + rated_ac_power * ufr_droop_k * (ufr_start_freq - grid_freq_f);
            limitf(&ufr_p_limit, rated_ac_power, x);
        } else {
            if (++ufr_delay_cnt > ufr_delay) {
                ufr_delay_cnt = 0;
                ufr_active = 1;
            }
            ufr_p_limit = x;
        }
    } else if (op_state == OP_GRID_PV && grid_func_flags.bit.b12 == 1 &&
               (grid_code == 3 || grid_code == 19)) {
        if (grid_freq_f < ufr_start_freq) {
            ufr_recover_cnt = 0;
            if (ufr_latched != 0) {
                ufr_p_prev = ufr_p_limit;
                ufr_p_limit =
                    ufr_p_base + rated_ac_power * ufr_droop_k * (ufr_start_freq - grid_freq_f);
                limitf(&ufr_p_limit, rated_ac_power, ufr_p_base);
                if (ufr_p_limit < ufr_p_prev)
                    ufr_p_limit = ufr_p_prev;
            } else if (++ufr_delay_cnt > ufr_delay) {
                ufr_delay_cnt = 0;
                ufr_p_base = lim_volt_watt < x ? lim_volt_watt : x;
                ufr_latched = 1;
                ufr_p_limit = ufr_p_base;
            }
        } else if (ufr_latched == 0) {
            ufr_p_limit = x;
            ufr_recover_cnt = 0;
            ufr_delay_cnt = 0;
        } else if (grid_freq_f <= ufr_recover_freq) {
            ufr_recover_cnt = 0;
            ufr_delay_cnt = 0;
        } else {
            ufr_recover_cnt++;
            ufr_delay_cnt = 0;
            if (ufr_recover_cnt > ufr_recover_delay) {
                ufr_recover_cnt = ufr_recover_delay;
                ufr_p_limit = ufr_p_limit - ufr_recover_step;
                if (ufr_p_limit <= ufr_p_base) {
                    ufr_latched = 0;
                    ufr_p_limit = ufr_p_base;
                }
            }
        }
    } else {
        ufr_recover_cnt = 0;
        ufr_delay_cnt = 0;
        ufr_active = 0;
        ufr_p_limit = x;
        ufr_latched = 0;
    }
    return ufr_p_limit;
}

/* Over-frequency storage response, op_state 6, 0xD7C2 bit 3, for the European group, 7, 3 and 19,
   on the filtered frequency. Above HR321 for HR300 ticks the base is the battery power. Curve 1
   (codes 7, 18): base - rated x HR123 / 10000 per 0.01 Hz; recovery steps back to the base at
   once, then ramps by rated / 33000 per tick to the restore point. Curve 2 (2, 9, 11, 13 with
   HR179): the same droop, never rising while active; recovery below HR328 after HR303 ticks.
   Curve 3 (3, 19): from the base down to 0 at 50.75 Hz (51.00 Hz for 19), then to -95 % (charging)
   at 52.00 Hz, or from a negative base straight to -rated; never rising; recovery ramp rated /
   18000 per tick. Others: plain droop, ended at once. The European group does not charge while
   batt_state_cmd is 2. Above 52.10 Hz everything resets. Writes freq_resp.of_resp_power. */
void ofr_storage_response(void) {
    float f, thr;
    ofr_s6_droop = ofr_droop * 0.0001f;
    f = grid_freq_filt;
    freq_resp.of_start_freq = ofr_start_freq;
    freq_resp.of_delay = ofr_delay;
    freq_resp.of_recover_delay = ofr_recover_delay;
    freq_resp.of_full_freq = 5200.0f;
    freq_resp.of_recover_freq = ofr_recover_freq;
    if (grid_code == 19)
        thr = 5100.0f;
    else
        thr = 5075.0f;
    if (grid_code == 7 || grid_code == 18)
        ofr_curve_type = 1;
    else if (grid_func_flags.bit.b2 == 1 &&
             (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11))
        ofr_curve_type = 2;
    else if (grid_code == 3 || grid_code == 19)
        ofr_curve_type = 3;
    else
        ofr_curve_type = 0;
    if (grid_func_flags.bit.b3 == 1 && op_state == OP_GRID_BATT &&
        (grid_code == 2 || grid_code == 7 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
         grid_code == 28 || grid_code == 24 || grid_code == 23 || grid_code == 22 ||
         grid_code == 25 || grid_code == 26 || grid_code == 3 || grid_code == 19)) {
        ofr_s6_enabled = 1;
        if (f > freq_resp.of_start_freq) {
            if (freq_resp.of_active == 0) {
                if (++ofr_s6_delay_cnt > freq_resp.of_delay) {
                    ofr_s6_delay_cnt = 0;
                    ofr_first_recover_s6 = 1;
                    freq_resp.of_active = 1;
                    ofr_s6_p_base = batt_power;
                    ofr_s6_power = batt_power;
                    if (ofr_curve_type == 3)
                        freq_resp.of_ramp_step = rated_ac_power / 18000.0f;
                    else
                        freq_resp.of_ramp_step = rated_ac_power / 33000.0f;
                }
            } else if (ofr_curve_type == 2) {
                ofr_s6_power_prev = ofr_s6_power;
                ofr_s6_power =
                    ofr_s6_p_base - rated_ac_power * ofr_s6_droop * (f - freq_resp.of_start_freq);
                limitf(&ofr_s6_power, rated_ac_power + 5000.0f, -rated_ac_power);
                if (ofr_s6_power > ofr_s6_power_prev)
                    ofr_s6_power = ofr_s6_power_prev;
            } else if (ofr_curve_type == 3) {
                ofr_s6_power_prev = ofr_s6_power;
                if (ofr_s6_p_base < 0.0f)
                    ofr_s6_power =
                        ofr_s6_p_base - (rated_ac_power + ofr_s6_p_base) *
                                            ((f - freq_resp.of_start_freq) /
                                             (freq_resp.of_full_freq - freq_resp.of_start_freq));
                else if (f <= thr)
                    ofr_s6_power = ofr_s6_p_base - (f - freq_resp.of_start_freq) /
                                                       (thr - freq_resp.of_start_freq) *
                                                       ofr_s6_p_base;
                else
                    ofr_s6_power =
                        rated_ac_power * -0.95f * ((f - thr) / (freq_resp.of_full_freq - thr));
                limitf(&ofr_s6_power, rated_ac_power + 5000.0f, -rated_ac_power);
                if (ofr_s6_power > ofr_s6_power_prev)
                    ofr_s6_power = ofr_s6_power_prev;
            } else {
                ofr_s6_power =
                    ofr_s6_p_base - rated_ac_power * ofr_s6_droop * (f - freq_resp.of_start_freq);
                limitf(&ofr_s6_power, rated_ac_power + 5000.0f, -rated_ac_power);
            }
            if ((grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
                 grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
                 grid_code == 26 || grid_code == 28) &&
                batt_state_cmd == 2)
                limitf(&ofr_s6_power, rated_ac_power + 5000.0f, 0.0f);
        } else if (freq_resp.of_active == 0) {
            ofr_s6_delay_cnt = 0;
            ofr_s6_recover_cnt = 0;
            ofr_first_recover_s6 = 0;
            freq_resp.of_ramp_step = 0.0f;
            ofr_s6_power_prev = 0.0f;
        } else if (ofr_curve_type == 1) {
            if (ofr_first_recover_s6 == 1) {
                ofr_first_recover_s6 = 0;
                ofr_s6_power = ofr_s6_p_base;
            } else {
                ofr_s6_power = freq_resp.of_ramp_step + ofr_s6_power;
                if (ofr_s6_power >= freq_resp.resp_restore_power) {
                    ofr_s6_power = freq_resp.resp_restore_power;
                    freq_resp.of_active = 0;
                }
            }
        } else if (ofr_curve_type == 2 || ofr_curve_type == 3) {
            if (f >= freq_resp.of_recover_freq) {
                ofr_s6_recover_cnt = 0;
            } else if (++ofr_s6_recover_cnt > freq_resp.of_recover_delay) {
                ofr_s6_power = freq_resp.of_ramp_step + ofr_s6_power;
                ofr_s6_recover_cnt = freq_resp.of_recover_delay;
                if (ofr_s6_power >= freq_resp.resp_restore_power) {
                    ofr_s6_power = freq_resp.resp_restore_power;
                    freq_resp.of_active = 0;
                }
            }
        } else {
            freq_resp.of_active = 0;
        }
    } else {
        freq_resp.of_active = 0;
        ofr_s6_enabled = 0;
        ofr_s6_delay_cnt = 0;
        ofr_s6_recover_cnt = 0;
        freq_resp.of_ramp_step = 0.0f;
        ofr_s6_power = 0.0f;
    }
    if (f > freq_resp.of_full_freq + 10.0f) {
        freq_resp.of_active = 0;
        ofr_s6_delay_cnt = 0;
        ofr_s6_recover_cnt = 0;
        freq_resp.of_ramp_step = 0.0f;
        ofr_s6_power = 0.0f;
        ofr_s6_power_prev = 0.0f;
    }
    freq_resp.of_resp_power = ofr_s6_power;
}

/* Under-frequency storage response, op_state 6, 0xD7C2 bit 12, the European group, 3 and 19, on
   the filtered frequency. Below HR326 for HR312 ticks the base is the battery power. Codes 3 and
   19: from a negative base (charging) to 0 at 49.00 Hz and then to rated + 250 W at 48.00 Hz, or
   from a positive base to rated at 48.00 Hz; never falling; above 49.85 Hz for 1500 ticks (30 s) a
   ramp of rated / 18000 per tick back to the restore point. Others: base + rated x HR327 / 10000
   per 0.01 Hz, ended at once when the frequency recovers. Writes freq_resp.uf_resp_power. */
void ufr_storage_response(void) {
    float f;
    ufr_s6_droop = ufr_droop * 0.0001f;
    freq_resp.uf_start_freq = ufr_start_freq;
    freq_resp.uf_delay = ufr_delay;
    freq_resp.uf_full_freq = 4800.0f;
    freq_resp.uf_recover_freq = 4985;
    freq_resp.uf_recover_delay = 1500;
    f = grid_freq_filt;
    if (grid_code == 3 || grid_code == 19)
        ufr_s6_au_curve = 1;
    else
        ufr_s6_au_curve = 0;
    if (grid_func_flags.bit.b12 == 1 && op_state == OP_GRID_BATT &&
        (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
         grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
         grid_code == 26 || grid_code == 3 || grid_code == 19 || grid_code == 28)) {
        ufr_s6_enabled = 1;
        if (f < freq_resp.uf_start_freq) {
            if (freq_resp.uf_active == 0) {
                if (++ufr_s6_delay_cnt > freq_resp.uf_delay) {
                    ufr_s6_delay_cnt = 0;
                    freq_resp.uf_active = 1;
                    ufr_s6_p_base = batt_power;
                    ufr_s6_power = batt_power;
                    if (ufr_s6_au_curve == 1)
                        freq_resp.uf_ramp_step = rated_ac_power / 18000.0f;
                    else
                        freq_resp.uf_ramp_step = rated_ac_power / 100.0f;
                }
            } else if (ufr_s6_au_curve == 1) {
                ufr_s6_power_prev = ufr_s6_power;
                if (ufr_s6_p_base < 0.0f) {
                    if (f >= 4900.0f)
                        ufr_s6_power =
                            (f - 4900.0f) / (freq_resp.uf_start_freq - 4900.0f) * ufr_s6_p_base;
                    else if (f >= 4800.0f)
                        ufr_s6_power = (rated_ac_power + 2500.0f) *
                                       ((4900.0f - f) / (4900.0f - freq_resp.uf_full_freq));
                    else
                        ufr_s6_power = rated_ac_power + 2500.0f;
                } else {
                    ufr_s6_power =
                        ufr_s6_p_base + (rated_ac_power - ufr_s6_p_base) *
                                            ((freq_resp.uf_start_freq - f) /
                                             (freq_resp.uf_start_freq - freq_resp.uf_full_freq));
                }
                limitf(&ufr_s6_power, rated_ac_power + 5000.0f, -rated_ac_power);
                if (ufr_s6_power < ufr_s6_power_prev)
                    ufr_s6_power = ufr_s6_power_prev;
            } else {
                ufr_s6_power =
                    ufr_s6_p_base + rated_ac_power * ufr_s6_droop * (freq_resp.uf_start_freq - f);
                limitf(&ufr_s6_power, rated_ac_power, -rated_ac_power);
            }
        } else if (freq_resp.uf_active == 0) {
            ufr_s6_delay_cnt = 0;
            freq_resp.uf_ramp_step = 0.0f;
            ufr_s6_power_prev = 0.0f;
            ufr_s6_power = 0.0f;
        } else if (ufr_s6_au_curve == 1) {
            if (f > freq_resp.uf_recover_freq) {
                if (++ufr_s6_recover_cnt > freq_resp.uf_recover_delay) {
                    ufr_s6_power = ufr_s6_power - freq_resp.uf_ramp_step;
                    ufr_s6_recover_cnt = freq_resp.uf_recover_delay;
                    if (ufr_s6_power <= freq_resp.resp_restore_power) {
                        ufr_s6_power = freq_resp.resp_restore_power;
                        freq_resp.uf_active = 0;
                    }
                }
            } else {
                ufr_s6_recover_cnt = 0;
            }
        } else {
            freq_resp.uf_active = 0;
        }
    } else {
        freq_resp.uf_active = 0;
        ufr_s6_enabled = 0;
        ufr_s6_delay_cnt = 0;
        ufr_s6_recover_cnt = 0;
        freq_resp.uf_ramp_step = 0.0f;
        ufr_s6_power = 0.0f;
    }
    freq_resp.uf_resp_power = ufr_s6_power;
}

/* Over-voltage storage response, op_state 6, 0xD7C2 bit 6 (HR176 bit 1), European group: above
   253.0 V (248.4 V for code 13) the power steps down by 1 % of rated every 70 ticks (1.4 s) from the
   battery power to -85 % (charging), and back up by the same steps to the base below it. A change
   of batt_state_cmd ends it. */
void ovr_storage_response(void) {
    if (op_state == OP_GRID_BATT && grid_func_flags.bit.b6 == 1 &&
        (grid_code == 2 || grid_code == 13 || grid_code == 9 || grid_code == 11 ||
         grid_code == 24 || grid_code == 23 || grid_code == 22 || grid_code == 25 ||
         grid_code == 26 || grid_code == 28)) {
        if (grid_code == 13)
            ovr_v_start = 2484.0f;
        else
            ovr_v_start = 2530.0f;
        if (grid_voltage > ovr_v_start) {
            if (freq_resp.ov_resp_active == 0) {
                freq_resp.ov_resp_active = 1;
                ovr_period = 70;
                ovr_step = rated_ac_power * 0.01f;
                ovr_power_min = -rated_ac_power * 0.85f;
                ovr_batt_state = batt_state_cmd;
                ovr_p_base = batt_power;
                ovr_power = batt_power;
            } else {
                if (ovr_batt_state == batt_state_cmd) {
                    if (++ovr_tick >= ovr_period) {
                        ovr_tick = 0;
                        ovr_power = ovr_power - ovr_step;
                        if (ovr_power < ovr_power_min)
                            ovr_power = ovr_power_min;
                    }
                } else {
                    freq_resp.ov_resp_active = 0;
                }
                ovr_batt_state = batt_state_cmd;
            }
        } else if (freq_resp.ov_resp_active == 0) {
            ovr_period = 0;
            ovr_power = 0.0f;
            ovr_power_min = 0.0f;
            ovr_step = 0.0f;
        } else if (++ovr_tick >= ovr_period) {
            ovr_tick = 0;
            ovr_power = ovr_step + ovr_power;
            if (ovr_power > ovr_p_base) {
                freq_resp.ov_resp_active = 0;
                ovr_power = ovr_p_base;
            }
        }
    } else {
        freq_resp.ov_resp_active = 0;
        ovr_period = 0;
        ovr_power = 0.0f;
        ovr_power_min = 0.0f;
        ovr_step = 0.0f;
    }
    freq_resp.ov_resp_power = ovr_power;
}

/* Under-voltage storage response, op_state 6, 0xD7C2 bit 6, codes 3 and 19: below 215.0 V (224.0 V
   for 19), every 50 ticks, a charging power beyond -18 % of rated is reduced in proportion to the
   voltage, to -18 % at 207.0 V (216.0 V); recovery ends it 50 ticks after the voltage is back. */
void uvr_storage_response(void) {
    if (op_state == OP_GRID_BATT && grid_func_flags.bit.b6 == 1 &&
        (grid_code == 3 || grid_code == 19)) {
        if (grid_code == 19) {
            uvr_v_start = 2240.0f;
            uvr_v_full = 2160.0f;
        } else {
            uvr_v_start = 2150.0f;
            uvr_v_full = 2070.0f;
        }
        if (grid_voltage < uvr_v_start) {
            if (freq_resp.uv_resp_active == 0) {
                freq_resp.uv_resp_active = 1;
                uvr_power_target = -rated_ac_power * 0.18f;
                uvr_period = 50;
                uvr_batt_state = batt_state_cmd;
                uvr_p_base = batt_power;
                uvr_power = batt_power;
            } else {
                if (uvr_batt_state == batt_state_cmd) {
                    if (++uvr_tick >= uvr_period) {
                        uvr_tick = 0;
                        if (uvr_p_base < uvr_power_target) {
                            uvr_power = uvr_p_base - (uvr_p_base - uvr_power_target) *
                                                         (uvr_v_start - grid_voltage) /
                                                         (uvr_v_start - uvr_v_full);
                            if (uvr_power > uvr_power_target)
                                uvr_power = uvr_power_target;
                        } else {
                            uvr_power = uvr_p_base;
                        }
                    }
                } else {
                    freq_resp.uv_resp_active = 0;
                }
                uvr_batt_state = batt_state_cmd;
            }
        } else if (freq_resp.uv_resp_active == 0) {
            uvr_power = 0.0f;
        } else if (++uvr_tick >= uvr_period) {
            uvr_tick = 0;
            freq_resp.uv_resp_active = 0;
        }
    } else {
        freq_resp.uv_resp_active = 0;
        uvr_power = 0.0f;
    }
    freq_resp.uv_resp_power = uvr_power;
}

/* Code 7: a voltage-dependent limit on the fixed PF y, on the voltage filtered 0.9/0.1. For PF
   codes 17500..20000: -0.9 from 195.5 to 241.5 V, moving to -1.0 at 253.0 V, else -1.0, and the
   result is the lower of that and y. For codes 1..2500: 0.9 from 218.5 to 253.0 V, from 1.0 at
   207.0 V, else 1.0, and the result is the higher of that and y. Other codes: 1.0. */
float pf_of_u_adjust(float x, float y) {
    float k;
    pfu_v_filt = pfu_v_filt * 0.9f + x * 0.1f;
    if (power_factor_set >= 17500 && power_factor_set <= 20000) {
        if (pfu_v_filt >= 2415.0f && pfu_v_filt < 2530.0f) {
            k = -(1.0f - (2530.0f - pfu_v_filt) * 0.1f / 115.0f);
        } else if (pfu_v_filt >= 1955.0f && pfu_v_filt < 2415.0f) {
            k = -0.9f;
        } else {
            k = -1.0f;
        }
        if (k > y)
            k = y;
    } else if (power_factor_set == 0) {
        k = 1.0f;
    } else if (power_factor_set > 2500) {
        k = 1.0f;
    } else {
        if (pfu_v_filt >= 2185.0f && pfu_v_filt <= 2530.0f) {
            k = 0.9f;
        } else if (pfu_v_filt >= 2070.0f && pfu_v_filt < 2185.0f) {
            k = 1.0f - (pfu_v_filt - 2070.0f) * 0.1f / 115.0f;
        } else {
            k = 1.0f;
        }
        if (k < y)
            k = y;
    }
    return k;
}

/* Code 7: PT1 smoothing of the cos phi(P) reactive power. Every 10 ticks (200 ms) it looks for a
   step of more than 30 W; once the new value has held, it moves there along
   target -/+ step x exp(-4.92 t / HR301) (99.3 % at t = HR301; t counts 0.02 per call, so HR301 is
   in seconds if read that way). Steps under 13 % of rated are taken at once. With HR301 = 0 the
   output is 0, not the input (docs/findings-control.md). Limited to +-43.8 % of rated. */
float q_pt1_response(float x) {
    float r;
    if (++q_pt1_tick >= 10) {
        q_pt1_tick = 0;
        if (fabs(x - q_pt1_last) > 300.0f && q_pt1_step_pending == 0) {
            q_pt1_step_pending = 1;
            q_pt1_step = fabs(x - q_pt1_last);
            q_pt1_target = x;
            x = q_pt1_last;
        } else if (q_pt1_step_pending == 1) {
            if (fabs(x - q_pt1_target) > 300.0f) {
                q_pt1_target = x;
                x = q_pt1_last;
            } else {
                q_pt1_target = x;
                if (x < q_pt1_last)
                    q_pt1_dir = 1.0f;
                else
                    q_pt1_dir = 2.0f;
                q_pt1_step = fabs(x - q_pt1_last);
                if (q_pt1_step < q_pt1_min_step)
                    q_pt1_dir = 0.0f;
                q_pt1_step_pending = 0;
            }
        }
        q_pt1_last = x;
    }
    q_pt1_min_step = rated_ac_power * 0.13f;
    q_pt1_time += 0.02f;
    q_pt1_k = 4.9235296f;
    if (q_pt1_dir != 0.0f) {
        if (q_pt1_step_pending == 1)
            q_pt1_dir = 0.0f;
        if (q_pt1_time > q_pt1_tau * 10 + 10) {
            q_pt1_time = q_pt1_tau * 10 + 10;
            q_pt1_dir = 0.0f;
        }
    } else {
        q_pt1_time = 0.0f;
    }
    if (q_pt1_tau == 0)
        r = 0.0f;
    else if (q_pt1_dir == 2.0f)
        r = q_pt1_target - expf(-q_pt1_k * q_pt1_time / q_pt1_tau) * q_pt1_step;
    else if (q_pt1_dir == 1.0f)
        r = q_pt1_target + expf(-q_pt1_k * q_pt1_time / q_pt1_tau) * q_pt1_step;
    else
        r = q_pt1_last;
    x = r;
    limitf(&x, rated_ac_power * 0.438f, -rated_ac_power * 0.438f);
    return x;
}

/* Export overload, CF14 bit 5. Code 7 in op_state 5 with the export limit on: the ARM's battery
   setpoint more than 300 W beyond the limit (at least 300 W) for 450 ticks, 500 W beyond for 125
   ticks, or beyond rated + 500 W at once. Codes 3 and 19 in op_state 6 with 0xD7C2 bit 9: 100 W
   beyond for 750 ticks (175 without the export limit), or CF19 bit 2. Otherwise it clears after
   501 ticks with CF19 bit 2 clear. */
void export_overload_check(void) {
    unsigned int n;
    exp_limit_check = grid_port_max_power;
    if (grid_code == 7 && op_state == OP_GRID_PV && grid_func_flags.bit.b0 == 1 &&
        batt_blocks.bit.b2 == 0 && arm_batt_setpoint < -(exp_limit_check + 1000.0f)) {
        if (exp_limit_check < 3000.0f)
            exp_limit_check = 3000.0f;
        exp_fault_clear_cnt = 0;
        if (arm_batt_setpoint < -(exp_limit_check + 3000.0f)) {
            if (++exp_over_cnt1 >= 450) {
                exp_over_cnt1 = 0;
                fault_grid_eps.bit.b5 = 1;
            }
        } else {
            exp_over_cnt1 = 0;
        }
        if (arm_batt_setpoint < -(exp_limit_check + 5000.0f)) {
            if (++exp_over_cnt2 >= 125) {
                exp_over_cnt2 = 0;
                fault_grid_eps.bit.b5 = 1;
            }
        } else {
            exp_over_cnt2 = 0;
        }
        if (arm_batt_setpoint < -(rated_ac_power + 5000.0f))
            fault_grid_eps.bit.b5 = 1;
    } else if ((grid_code == 3 || grid_code == 19) && op_state == OP_GRID_BATT &&
               grid_func_flags.bit.b9 == 1) {
        exp_fault_clear_cnt = 0;
        if (grid_func_flags.bit.b0 == 1)
            n = 750;
        else
            n = 175;
        if (arm_batt_setpoint < -(exp_limit_check + 1000.0f)) {
            if (++exp_over_cnt1 >= n) {
                exp_over_cnt1 = 0;
                fault_grid_eps.bit.b5 = 1;
            }
        } else {
            exp_over_cnt1 = 0;
        }
        fault_grid_eps.bit.b5 |= batt_blocks.bit.b2;
    } else {
        exp_over_cnt1 = 0;
        exp_over_cnt2 = 0;
        if (fault_grid_eps.bit.b5 == 1 && batt_blocks.bit.b2 == 0 && ++exp_fault_clear_cnt > 500) {
            fault_grid_eps.bit.b5 = 0;
            exp_fault_clear_cnt = 0;
        }
    }
}

/* Code 7, fixed Q: after a momentary cessation on low voltage (fast_trip_low_v), Q is 0 while
   the bridge is off and for 50 ticks (1 s) after it is back on; then it rises from 0 to the value
   captured at that moment along x0 (1 - exp(-4.92 t / HR301)), and after HR301 x 4 + 10 the input
   passes straight through again. Without a trip it passes x through. (docs/names/A-notes.md read
   this as a decay to 0; the formula with target 0 gives a rise from 0 to x0,
   docs/findings-control.md.) Limited to +-43.8 % of rated. */
float q_restore_response(float x) {
    q_rec_time += 0.02f;
    q_rec_target = x;
    q_rec_k = 4.9235296f;
    if (q_rec_time > q_pt1_tau * 4.0f + 10.0f) {
        q_rec_time = q_pt1_tau * 4.0f + 10.0f;
        q_rec_active = 0;
    }
    if (fast_trip_low_v == 1) {
        q_rec_target = 0.0f;
        if (inv_on == 0) {
            q_rec_cnt = 0;
            q_rec_active = 0;
        } else if (++q_rec_cnt >= 50) {
            fast_trip_low_v = 0;
            q_rec_active = 1;
            q_rec_start = x;
        }
    }
    if (q_pt1_tau != 0 && q_rec_active == 1) {
        q_rec_target = 0.0f;
        x = q_rec_start + (q_rec_target - q_rec_start) * expf(q_rec_time * -q_rec_k / q_pt1_tau);
    } else {
        q_rec_time = 0.0f;
        x = q_rec_target;
    }
    limitf(&x, rated_ac_power * 0.438f, -rated_ac_power * 0.438f);
    return x;
}
