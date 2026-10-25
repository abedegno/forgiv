// clang-format off
/* RTOS task 1 (task1_ac_measure): the AC measurements and the checks that follow them.
   Entry point: task1_ac_measure, an RTOS task. It waits for three events:
     - the grid period event (grid_period_isr, once per grid cycle): grid voltage, inverter
       voltage, grid current and leakage current RMS, DC injection, frequency, active power and
       power factor, the grid power limit PI, 50/60 Hz detection, and the grid-loss watchdog kick;
     - the EPS sums event (eps_rms_accum, once per EPS output cycle): EPS voltage, current and
       power, and the slow EPS voltage loop;
     - the RTOS tick (every 20 ms): power filters, the bus voltage reference, the relay and EPS
       checks, the ARM power check, the grid-loss watchdog and the run mode request (select_state).
   Data: its own counters and filters at 0xD300..0xD339 (below); it fills the measurement block
   ac (0xD3C0) that task 4 (grid_support.c) and the ARM link read.
   Units: voltages 0.1 V, currents 0.1 A (leakage in mA), powers 0.1 W, frequency 0.01 Hz,
   counts of 20 ms ticks unless a function says otherwise.
   Neighbours: converter_ctrl.c (grid_period_isr, the sums, ecap_period), grid_freq_setup.c,
   grid_support.c (out_power_limit, q mode), op_states.c (run_mode_req, the relay commands),
   faults.h (the fault words CF12..CF19).
   name: descriptive (was rating_b.c; map/filenames.tsv). Header rating.h.
   at: eps_current_rms 0x3EE916; eps_dc_clear 0x3EE925; clear_d3e6 0x3EE9E3; clear_d3e4 0x3EE9E9;
       inv_voltage_rms 0x3EE9EF; grid_current_rms 0x3EE9FE; dc_component_calc 0x3EEA0D; current2_rms 0x3EEA1C;
       grid_loss_watchdog_kick 0x3EEE09; init_ac_meas 0x3EF011; eps_voltage_rms 0x3EE8F3; eps_power_calc 0x3EE92B;
       grid_voltage_rms 0x3EE95E; grid_freq_measure 0x3EE989; task1_ac_measure 0x3EE6F2; relays_off_when_disabled 0x3EE764;
       grid_freq_detect 0x3EECFE; grid_loss_watchdog 0x3EEDB9; bus_voltage_ref_update 0x3EECB0; eps_dc_offset_trim 0x3EEC55;
       select_state 0x3EE836; grid_power_filter 0x3EEA33; grid_power_limit_pi 0x3EEF9B; relay_test_check 0x3EED3D;
       eps_voltage_control 0x3EEEDB; eps_output_check 0x3EEABD; arm_power_check 0x3EE788; grid_power_pf_calc 0x3EEE1A */
// clang-format on
#define OWN_arm_fault_word /* match: one bit word here; arm_link.c defines it as unsigned long (the 32-bit fault word of the D frame) */
#define OWN_eps_cfg_bits /* match: this file sees 0xD7C0 as one bit word; the header has the three-word struct eps_cfg_bits_t (0xD7C0 .. 0xD7C2) the C files use */
#include <string.h>
#include <math.h>
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "meas.h"
#include "state.h"
#include "rating.h"
#include "control.h"
extern union bits16_u eps_cfg_bits; /* match: own form, see OWN_eps_cfg_bits */

/* this file's variables, 0xD300..0xD339. The compiler does not keep their order; this order gives
 * the image's layout. Any change to this file can move them: rerun
 *     python3 tools/ebss_solve.py src/app/meas/ac_measure.c */
/* any grid fault: CF12 high byte, CF13 and CF14 low byte */
#define GRID_FAULTS                                                                                \
    ((fault_words[0].all >> 8) | (fault_words[1].all & 0xFF) | (fault_words[1].all >> 8) |         \
     (fault_words[2].all & 0xFF))

extern union bits16_u arm_fault_word;
// clang-format off
/* .ebss begin */
unsigned int eps_fault_clear_count = 0;
unsigned int eps_ov_clear_count = 0;
int arm_power_fast_count = 0;
unsigned int eps_overload_count = 0;
unsigned int cf14b12_clear_count = 0;
unsigned int grid_freq_detected;
unsigned int eps_ov_fast_count = 0;
unsigned int eps_uv_count = 0;
unsigned int eps_ov_count = 0;
int arm_power_slow_count = 0;
unsigned int cf19b7_clear_count = 0;
unsigned int eps_uv2_count = 0;
unsigned int grid_freq;
unsigned int arm_power_trips = 0;
unsigned int freq50_count = 0;
unsigned int eps_short_count = 0;
unsigned int eps_trip_count = 0;
unsigned int eps_dc_trim_delay = 0;
unsigned int freq_hist_idx;
unsigned int freq60_count = 0;
unsigned int eps_overload_fast_count = 0;
unsigned int relay_b4_count = 0;
unsigned int relay_b5_count = 0;
unsigned int relay_lockouts = 0;
unsigned int eps_vctl_delay = 0;
unsigned int watchdog_kick_count = 0;
unsigned int disable_count = 0;
unsigned int grid_pi_delay = 0;
unsigned int relay_fail_count = 0;
float eps_dc_filt = 0;
float active_power = 0;
float apparent_power_est = 0;
float eps_dc_trim = 0;
float pf_denom = 0;
float eps_v_ramp = 2300.0f;
unsigned long eps_lockout_timer = 0;
unsigned long ecap_period_med;
unsigned long relay_lockout_timer = 0;
unsigned long arm_power_trip_timer = 0;
float relay_v_diff = 0;
unsigned long ecap_period_hist[3] = {1200000, 1200000, 1200000};
/* .ebss end */
// clang-format on

/* EPS current RMS: sqrt of the cycle's sum of squares over its sample count. */
void eps_current_rms(void) { ac.eps_i = sqrtf(ac_sums.eps_i_sq * ac_sums.inv_n_eps); }

/* Zeroes ac.eps_dc every EPS cycle. Nothing else writes it, so eps_dc_offset_trim always sees 0. */
void eps_dc_clear(void) { ac.eps_dc = 0.0f; }

/* Zeroes the unused word 0xD3E6 every grid cycle (a stub; nothing reads it). */
void clear_d3e6(void) { ac.unused_26 = 0.0f; }

/* Zeroes the unused word 0xD3E4 every grid cycle (a stub; nothing reads it). */
void clear_d3e4(void) { ac.unused_24 = 0.0f; }

/* Inverter (bridge side) voltage RMS over the last grid cycle. */
void inv_voltage_rms(void) { ac.inv_v = sqrtf(ac_sums.inv_v_sq * ac_sums.inv_n_grid); }

/* Grid (inverter) current RMS over the last grid cycle. */
void grid_current_rms(void) { ac.grid_i = sqrtf(ac_sums.grid_i_sq * ac_sums.inv_n_grid); }

/* DC injection: the mean of the DCI channel over the last grid cycle (ac.dc_component, 0xD3FA,
   which task 4's dci_control filters). It divides by task 3's count (dc_acc.a.inv_n, the samples
   per nominal cycle) rather than ac_sums.inv_n_grid; the two are nearly equal. */
void dc_component_calc(void) { ac.dc_component = ac_sums.dc_sum * dc_acc.a.inv_n; }

/* Residual (leakage) current RMS over the cycle in mA (ac.i2) and in A (ac.i2_k, 0xD488 =
   rcmu_i[0], the RCMU's first input). */
void current2_rms(void) {
    ac.i2 = sqrtf(ac_sums.leak_i_sq * ac_sums.inv_n_grid);
    ac.i2_k = 0.001f * ac.i2;
}

/* Called once per grid period: resets the grid-loss watchdog and, after 3 periods, clears
   state_flags bit 8 (grid lost). */
void grid_loss_watchdog_kick(void) {
    if (++watchdog_kick_count > 2) {
        state_flags.bit.b8 = 0;
        watchdog_kick_count = 0;
    }
    grid_cycle_watchdog = 0;
}

/* Clears the AC sums and results; the EPS sample count starts at 384 and the battery charge
   current limit at 6000 (60.00 A). */
void init_ac_meas(void) {
    memset(&ac_sums_isr, 0, sizeof(ac_sums_isr));
    memset(&ac_sums, 0, sizeof(ac_sums));
    memset(&ac, 0, sizeof(ac));
    ac_sums.n_eps = 0x180;
    ac_sums_isr.n_eps = 0x180;
    ac.batt_max_charge_current = 6000.0f;
}

/* EPS voltage RMS, and a filtered copy (0.7 old + 0.3 new) for the slow EPS loop. */
void eps_voltage_rms(void) {
    ac.eps_v = sqrtf(ac_sums.eps_v_sq * ac_sums.inv_n_eps);
    ac.eps_v_filt = 0.7f * ac.eps_v_filt + 0.3f * ac.eps_v;
}

/* EPS active power (mean of v x i) and apparent power (V x I), both / 10; zero outside
   op_states 7 and 8. */
void eps_power_calc(void) {
    ac.eps_p = 0.1f * (ac_sums.eps_p * ac_sums.inv_n_eps);
    ac.eps_s = 0.1f * (ac.eps_v * ac.eps_i);
    if (op_state != OP_EPS_PV && op_state != OP_EPS_BATT) {
        ac.eps_i = 0.0f;
        ac.eps_p = 0.0f;
        ac.eps_s = 0.0f;
    }
}

/* Grid voltage RMS: raw, calibrated by cal_grid_v / 1024 (grid_voltage_fast, 0xD3CA, one cycle)
   and filtered 0.9/0.1 (grid_voltage, 0xD3CC). Task 4's protection reads both. */
void grid_voltage_rms(void) {
    ac.grid_v_raw = sqrtf(ac_sums.grid_v_sq * ac_sums.inv_n_grid);
    ac.grid_v_cal = 0.0009765625f * (ac.grid_v_raw * cal_grid_v);
    ac.grid_v = 0.9f * ac.grid_v + 0.1f * ac.grid_v_cal;
}

/* Grid frequency in 0.01 Hz from the median of the last three ECAP periods:
   f = 9e9 / period (90 MHz capture clock), plus a +2 correction; under 41.00 Hz it is 0. The
   -1 copy (grid_freq_f) and a 0.05/0.95 filtered copy (grid_freq_filt) go to task 4. */
void grid_freq_measure(void) {
    if (++freq_hist_idx >= 3)
        freq_hist_idx = 0;
    ecap_period_hist[freq_hist_idx] = ecap_period;
    ecap_period_med = median3ul(ecap_period_hist[0], ecap_period_hist[1], ecap_period_hist[2]);
    if (ecap_period_med != 0)
        grid_freq = (unsigned int)(4500000000LL / (long long)(ecap_period_med >> 1)) + 2;
    else
        grid_freq = 0;
    if (grid_freq < 4100)
        grid_freq = 0;
    ac.grid_freq_f = grid_freq - 1;
    ac.grid_freq_filt = 0.05f * ac.grid_freq_f + 0.95f * ac.grid_freq_filt;
}

/* RTOS task 1. The three event branches are described in the file header. */
void task1_ac_measure(void) {
    unsigned long ev;
    unsigned int n;

    init_ac_meas();
    for (;;) {
        ev = os_wait(0);
        if (ev & EV1_GRID_PERIOD_M) {
            n = ac_sums.n_grid;
            if (n != 0)
                ac_sums.inv_n_grid = 1.0f / n;
            grid_voltage_rms();
            grid_freq_measure();
            clear_d3e6();
            clear_d3e4();
            inv_voltage_rms();
            grid_current_rms();
            dc_component_calc();
            current2_rms();
            grid_power_pf_calc();
            grid_power_limit_pi();
            grid_freq_detect(grid_freq);
            grid_loss_watchdog_kick();
        }
        if (ev & EV1_RMS_SUMS_M) {
            n = ac_sums.n_eps;
            if (n != 0)
                ac_sums.inv_n_eps = 1.0f / n;
            eps_voltage_rms();
            eps_current_rms();
            eps_dc_clear();
            eps_power_calc();
            eps_voltage_control();
        }
        if (ev & EV_TICK_M) {
            grid_power_filter();
            eps_dc_offset_trim();
            bus_voltage_ref_update();
            relay_test_check();
            eps_output_check();
            arm_power_check();
            relays_off_when_disabled();
            grid_loss_watchdog(2);
            select_state();
        }
    }
}

/* With the inverter disabled (inverter_enable 0) or the ARM's run mode 10 (stop) for more than 5
   ticks, opens all three relays and clears the output-on bits. */
void relays_off_when_disabled(void) {
    if (dsp_settings.inverter_enable == 0 || dsp_settings.arm_run_mode == 10) {
        if (++disable_count > 5) {
            relay_cmd.relay_a = 0;
            relay_cmd.relay_b = 0;
            state_flags.bit.b3 = 0;
            run_status_bits.bit.b3 = 0;
            disable_count = 0;
            relay_cmd.relay_c = 0;
        }
    } else {
        disable_count = 0;
    }
}

/* 50/60 Hz detection, once per grid period until done: 26 periods in 46..54 Hz set 50 Hz, 26 in
   56..64 Hz set 60 Hz (grid_60hz), and grid_freq_setup sets the control interrupt's cycle
   constants. Any period outside both bands restarts the count. task 1's watchdog clears
   grid_freq_detected when the grid is lost. */
void grid_freq_detect(unsigned int f) {
    if (grid_freq_detected == 0) {
        if (f > 4600 && f < 5400) {
            freq50_count++;
            freq60_count = 0;
            if (freq50_count > 25) {
                dsp_settings.grid_60hz = 0;
                freq50_count = 0;
                grid_freq_setup(dsp_settings.grid_60hz);
                grid_freq_detected = 1;
            }
        } else if (f > 5600 && f < 6400) {
            freq60_count++;
            freq50_count = 0;
            if (freq60_count > 25) {
                dsp_settings.grid_60hz = 1;
                freq60_count = 0;
                grid_freq_setup(dsp_settings.grid_60hz);
                grid_freq_detected = 1;
            }
        } else {
            freq50_count = 0;
            freq60_count = 0;
            grid_freq_detected = 0;
        }
    }
}

/* Grid-loss watchdog, every tick: more than lim ticks (lim 2: 60 ms; lim 15, 320 ms, for grid code
   25 with 0xD7C2 bit 4) without a grid period sets state_flags bit 8 and posts the grid fault event to
   task 0. While bit 8 is set the grid measurements, the period and the detection are zeroed. */
void grid_loss_watchdog(unsigned int lim) {
    if (grid_code == 25 && grid_func_flags.bit.b4)
        lim = 15;
    if (++grid_cycle_watchdog > lim) {
        grid_cycle_watchdog = 0;
        state_flags.bit.b8 = 1;
        os_post(TASK_STATE, EV0_GRID_FAULT);
    }
    if (state_flags.bit.b8) {
        ac.active_p_raw = 0.0f;
        ac.grid_v_raw = 0.0f;
        ac.active_p = 0.0f;
        ecap_valid_count = 0;
        grid_freq_detected = 0;
        ecap_period = 0;
        ac.grid_v_cal = 0.0f;
        ac.apparent_p = 0.0f;
        ac.grid_v = 0.0f;
        ac.apparent_p_filt = 0.0f;
        ac.grid_i = 0.0f;
        ac.eps_dc = 0.0f;
        ac.active_p_filt = 0.0f;
        ac.reactive_p = 0.0f;
    }
}

/* The DC bus voltage reference (dc_meas.bus_v_ref, 0.1 V): 430.0 V normally, 400.0 V in the EPS
   states or with run_mode_req 7 or 8; with pv_bus_mode (never set) sqrt 2 x grid V + 40.0 V in
   370..420 V. It moves at most 5.0 V per tick. */
void bus_voltage_ref_update(void) {
    float x = 1.4142135f * ac.grid_v + 400.0f;
    limitf(&x, 4200.0f, 3700.0f);
    dc_meas.bus_v_ref = 4300.0f;
    if (pv_bus_mode != 0)
        dc_meas.bus_v_ref = x;
    if (dsp_settings.run_mode_req >= 7 || op_state == OP_EPS_PV || op_state == OP_EPS_BATT)
        dc_meas.bus_v_ref = 4000.0f;
    rate_limit(&dc_meas.bus_v_ref, dc_meas.bus_v_ref_prev, 50.0f, -50.0f);
    dc_meas.bus_v_ref_prev = dc_meas.bus_v_ref;
}

/* Meant to trim the EPS output's DC offset by +-1 per tick (limit +-100) after 500 ticks in EPS.
   Dead in effect: its input ac.eps_dc is zeroed every cycle (eps_dc_clear) and the value handed to
   the interrupt (eps_dc_trim_isr, 0xCC74) is always 0.0 (docs/findings-control.md). */
void eps_dc_offset_trim(void) {
    if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT) {
        if (++eps_dc_trim_delay > 500) {
            eps_dc_filt = 0.9f * eps_dc_filt + 0.1f * ac.eps_dc;
            eps_dc_trim_delay = 500;
            if (eps_dc_filt > 300.0f)
                eps_dc_trim -= 1.0f;
            else if (eps_dc_filt < -300.0f)
                eps_dc_trim += 1.0f;
            if (eps_dc_trim > 100.0f)
                eps_dc_trim = 100.0f;
            else if (eps_dc_trim < -100.0f)
                eps_dc_trim = -100.0f;
        }
    } else {
        eps_dc_trim_delay = 0;
        eps_dc_trim = 0.0f;
    }
    DINT;
    eps_dc_trim_isr = 0.0f;
    EINT;
}

/* The run mode request (run_mode_req) for the state machine, every tick, from the ARM's command and
   the fault words: 4 if the ARM asks for an update; 0 when stopped, disabled or reconfiguring
   (bms_update_pending, cfg_changed_flags); 3 with a stop fault (CF17 or CF18); 7 in the low-battery
   wake with no PV fault (CF16); 5 (grid with PV) with no grid fault and no PV fault; 6 (grid, battery only)
   with no grid or battery fault but a PV fault; 7 (EPS with PV) with a grid fault and no EPS (CF14
   high byte), battery (CF15) or PV fault; 8 (EPS, battery only) with a grid fault, a PV fault and EPS enabled (0xD7C0 bit 9). */
void select_state(void) {
    if (dsp_settings.arm_run_mode == 4) {
        dsp_settings.run_mode_req = dsp_settings.arm_run_mode;
    } else if (dsp_settings.arm_run_mode == 10 || dsp_settings.inverter_enable == 0 ||
               (bms_update_pending | dsp_settings.cfg_changed_flags) != 0) {
        dsp_settings.run_mode_req = 0;
    } else if (((fault_words[5].all & 0xFF) | (fault_words[5].all >> 8) |
                (fault_words[6].all & 0xFF)) != 0 ||
               (fault_words[6].all & 0xFF00) != 0) {
        dsp_settings.run_mode_req = 3;
    } else if (low_batt_wake == 1 && (fault_words[4].all & 0xFF) == 0 &&
               (fault_words[4].all & 0xFF00) == 0) {
        dsp_settings.run_mode_req = 7;
    } else if ((GRID_FAULTS | (fault_words[4].all & 0xFF)) == 0 &&
               (fault_words[4].all & 0xFF00) == 0) {
        dsp_settings.run_mode_req = 5;
    } else if ((GRID_FAULTS | (fault_words[3].all & 0xFF)) == 0 &&
               (fault_words[3].all & 0xFF00) == 0 &&
               ((fault_words[4].all & 0xFF) | (fault_words[4].all >> 8)) != 0) {
        dsp_settings.run_mode_req = 6;
    } else if (GRID_FAULTS != 0 && (fault_words[2].all & 0xFF00) == 0 &&
               (fault_words[3].all & 0xFF) == 0 && (fault_words[3].all & 0xFF00) == 0 &&
               (fault_words[4].all & 0xFF) == 0 && (fault_words[4].all & 0xFF00) == 0) {
        dsp_settings.run_mode_req = 7;
    } else if (GRID_FAULTS != 0 && (fault_words[2].all & 0xFF00) == 0 &&
               (fault_words[3].all & 0xFF) == 0 && (fault_words[3].all & 0xFF00) == 0 &&
               (((fault_words[4].all & 0xFF) | (fault_words[4].all >> 8)) != 0 &&
                eps_cfg_bits.bit.b9)) {
        dsp_settings.run_mode_req = 8;
    } else {
        dsp_settings.run_mode_req = 0;
    }
}

/* In op_states 5..8: filters the apparent and active power (0.7/0.3), the load in % of rated
   (load_percent, 0..100) and the reactive power sqrt(S^2 - P^2); zero otherwise. */
void grid_power_filter(void) {
    float d, x;
    if (op_state >= OP_GRID_PV) {
        ac.apparent_p_filt = 0.7f * ac.apparent_p_filt + 0.3f * ac.apparent_p;
        ac.active_p_filt = 0.7f * ac.active_p_filt + 0.3f * ac.active_p;
        d = rated_ac_power;
        if (d != 0.0f)
            load_percent = 100.0f * (fabs(ac.active_p_filt) / d);
        if (load_percent > 100)
            load_percent = 100;
        if (fabs(ac.apparent_p_filt) > fabs(ac.active_p_filt))
            ac.reactive_p = sqrtf(ac.apparent_p_filt * ac.apparent_p_filt -
                                  ac.active_p_filt * ac.active_p_filt);
        else
            ac.reactive_p = 0.0f;
    } else {
        ac.active_p_filt = 0.0f;
        load_percent = 0;
        ac.apparent_p_filt = 0.0f;
        ac.reactive_p = 0.0f;
    }
}

/* Once per grid period in op_states 5/6, after 150 periods: a PI (kp 0.3125, ki 0.1, +-3500) on
   out_power_limit - |P|, the headroom to task 4's power limit; its output goes to ac.grid_pi_out
   (0xD3F6), which task 4 reads as p_limit_trim and adds to the power limit before it turns it
   into the grid current bounds: a closed loop that makes the measured power meet the limit. */
void grid_power_limit_pi(void) {
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        if (++grid_pi_delay >= 150) {
            ac.grid_pi.e = out_power_limit - fabs(ac.active_p);
            ac.grid_pi.kp = 0.3125f;
            ac.grid_pi.ki = 0.1f;
            ac.grid_pi.kd = 0.0f;
            ac.grid_pi.umax = 3500.0f;
            grid_pi_delay = 150;
            ac.grid_pi.umin = -3500.0f;
            pi_step(&ac.grid_pi);
            if (ac.grid_pi.u > 3500.0f)
                ac.grid_pi.u = 3500.0f;
            else if (ac.grid_pi.u < -3500.0f)
                ac.grid_pi.u = -3500.0f;
        }
    } else {
        grid_pi_delay = 0;
        ac.grid_pi.i = 0.0f;
        ac.grid_pi.u = 0.0f;
    }
    ac.grid_pi_out = ac.grid_pi.u;
}

/* The relay test checks of the start sequence (state_flags bits 4 and 5, set by op_states.c):
   with bit 4, inverter and grid voltage more than 30.0 V apart; with bit 5, the inverter side
   above 160.0 V or the EPS side below 160.0 V. 10 ticks of either is one failure: it stops the
   start (event 1), and the 5th failure sets CF17 bit 11, which clears after 180000 ticks
   (1 hour). */
void relay_test_check(void) {
    if (state_flags.bit.b4) {
        relay_v_diff = ac.inv_v - ac.grid_v;
        if (fabs(relay_v_diff) > 300.0f) {
            if (++relay_b4_count >= 10) {
                relay_fail_count++;
                relay_b4_count = 0;
                if (relay_fail_count >= 5) {
                    relay_fail_count = 0;
                    fault_stop1.bit.b11 = 1;
                } else
                    os_post(TASK_STATE, EV0_STOP);
            }
        } else {
            relay_b4_count = 0;
        }
    } else {
        relay_b4_count = 0;
    }
    if (state_flags.bit.b5) {
        if (ac.inv_v > 1600.0f || ac.eps_v < 1600.0f) {
            if (++relay_b5_count >= 10) {
                relay_fail_count++;
                relay_b5_count = 0;
                if (relay_fail_count >= 5) {
                    fault_stop1.bit.b11 = 1;
                    relay_fail_count = 0;
                } else
                    os_post(TASK_STATE, EV0_STOP);
            }
        } else {
            relay_b5_count = 0;
        }
    } else {
        relay_b5_count = 0;
    }
    if (fault_stop1.bit.b11) {
        if (++relay_lockout_timer > 180000) {
            relay_lockouts++;
            fault_stop1.bit.b11 = 0;
            relay_lockout_timer = 0;
            if (relay_lockouts > 2)
                relay_lockouts = 2;
        }
    }
}

/* The slow EPS voltage loop, once per EPS cycle. The RMS reference eps_v_ramp rises 10.0 V per
   cycle from 50.0 V to the nominal grid voltage while the EPS bridge is on (or in the grid
   states). In op_states 7/8 with eps_vctl_enable, after 50 cycles, a PI (kp 0.8, ki 0.1) on the
   reference less the filtered RMS (error +-20.0 V) gives 0..50.0 V; its integrator is held at or
   below 0 while the output is 30.0 V high. The output goes to the interrupt as amp_trim
   (eps_vctl_out, 0xCC76), added to the EPS amplitude. */
void eps_voltage_control(void) {
    if ((inv_on == 1 && (op_state == OP_EPS_BATT || op_state == OP_EPS_PV)) ||
        op_state == OP_GRID_BATT || op_state == OP_GRID_PV) {
        eps_v_ramp += 100.0f;
        if (eps_v_ramp > grid_v_nominal)
            eps_v_ramp = grid_v_nominal;
    } else {
        eps_vctl_delay = 0;
        eps_v_ramp = 500.0f;
        ac.eps_pi.i = 0.0f;
        ac.eps_pi.u = 0.0f;
    }
    ac.eps_v_ref = eps_v_ramp;
    if ((op_state == OP_EPS_PV || op_state == OP_EPS_BATT) && eps_vctl_enable == 1) {
        if (++eps_vctl_delay > 50) {
            eps_vctl_delay = 50;
            ac.eps_pi.e = ac.eps_v_ref - ac.eps_v_filt;
            limitf(&ac.eps_pi.e, 200.0f, -200.0f);
            if (ac.eps_v_filt > ac.eps_v_ref + 300.0f)
                ac.eps_pi.i = __fmin(ac.eps_pi.i, 0.0f);
            ac.eps_pi.kp = 0.8f;
            ac.eps_pi.ki = 0.1f;
            ac.eps_pi.kd = 0.0f;
            ac.eps_pi.umax = 500.0f;
            ac.eps_pi.umin = -500.0f;
            pi_step(&ac.eps_pi);
            if (ac.eps_pi.u > 500.0f)
                ac.eps_pi.u = 500.0f;
            else
                ac.eps_pi.u = __fmax(ac.eps_pi.u, 0.0f);
        }
    } else {
        eps_vctl_delay = 0;
        ac.eps_pi.i = 0.0f;
        ac.eps_pi.u = 0.0f;
    }
    DINT;
    eps_vctl_out = ac.eps_pi.u;
    EINT;
}

/* The EPS output checks, every tick, in op_states 7/8 with EPS enabled (0xD7C0 bit 9), against
   the nominal grid voltage Vn:
   - over-voltage CF14 bit 13: above 1.1 Vn for 51 ticks, or above 1.2 Vn for 26;
   - overload CF14 bit 12: S above 1.1 x rated for 151 ticks, or 1.5 x rated for 6 (CF19 bit 7 is
     set at once as a warning);
   - under-voltage CF14 bit 8: below 0.85 Vn or 0.8 Vn for 101 ticks;
   - short circuit CF17 bit 8: below 40.0 V with more than 15.0 A on 5 ticks (the count is only
     reset by the trip, so the ticks need not be consecutive).
   Clearing: CF14 bits 13 and 12 and CF19 bit 7 after 1500 ticks (30 s). CF14 bit 8 (and bit 11,
   the EPS bus under-voltage) after 1500 ticks too, but the 3rd trip sets run_status bit 6, a lock
   that holds the bit while state_flags bit 1 is set, released after 180000 ticks (1 hour). */
void eps_output_check(void) {
    unsigned int lo1, lo2;
    float r;
    if ((op_state == OP_EPS_PV || op_state == OP_EPS_BATT) && eps_cfg_bits.bit.b9) {
        r = grid_v_nominal;
        lo1 = 0.85f * r;
        lo2 = 0.8f * r;
        if (ac.eps_v > 1.1f * r) {
            if (++eps_ov_count > 50) {
                eps_ov_count = 0;
                fault_grid_eps.bit.b13 = 1;
            }
        } else {
            eps_ov_count = 0;
        }
        if (ac.eps_v > 1.2f * grid_v_nominal) {
            if (++eps_ov_fast_count > 25) {
                eps_ov_fast_count = 0;
                fault_grid_eps.bit.b13 = 1;
            }
        } else {
            eps_ov_fast_count = 0;
        }
        if (ac.eps_s > 1.1f * rating_params.rated_power) {
            fault_words[7].all |= BB_EPS_OVERLOAD;
            if (++eps_overload_count > 150) {
                eps_overload_count = 0;
                fault_words[2].all |= FGE_EPS_OVERLOAD;
            }
        } else {
            eps_overload_count = 0;
        }
        if (ac.eps_s > 1.5f * rating_params.rated_power) {
            fault_words[7].all |= BB_EPS_OVERLOAD;
            if (++eps_overload_fast_count > 5) {
                eps_overload_fast_count = 0;
                fault_words[2].all |= FGE_EPS_OVERLOAD;
            }
        } else {
            eps_overload_fast_count = 0;
        }
        if (ac.eps_v < lo1) {
            if (++eps_uv_count > 100) {
                eps_uv_count = 0;
                fault_grid_eps.bit.b8 = 1;
            }
        } else {
            eps_uv_count = 0;
        }
        if (ac.eps_v < lo2) {
            if (++eps_uv2_count > 100) {
                eps_uv2_count = 0;
                fault_grid_eps.bit.b8 = 1;
            }
        } else {
            eps_uv2_count = 0;
        }
        if (ac.eps_v < 400.0f && ac.eps_i > 150.0f) {
            if (++eps_short_count >= 5) {
                eps_short_count = 0;
                fault_stop1.bit.b8 = 1;
            }
        }
    }
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        eps_trip_count = 0;
        run_status_bits.bit.b6 = 0;
    }
    if (fault_grid_eps.bit.b13) {
        if (++eps_ov_clear_count > 1500) {
            eps_ov_clear_count = 0;
            fault_grid_eps.bit.b13 = 0;
        }
    }
    if (fault_grid_eps.bit.b12) {
        if (++cf14b12_clear_count > 1500) {
            cf14b12_clear_count = 0;
            fault_grid_eps.bit.b12 = 0;
        }
    }
    if (batt_blocks.bit.b7) {
        if (++cf19b7_clear_count > 1500) {
            cf19b7_clear_count = 0;
            batt_blocks.bit.b7 = 0;
        }
    }
    if (fault_grid_eps.bit.b8) {
        if (++eps_fault_clear_count > 1500) {
            eps_fault_clear_count = 0;
            if (++eps_trip_count >= 3)
                run_status_bits.bit.b6 = 1;
            if (!(((run_status_bits.all >> 6) & 1) & ((state_flags.all >> 1) & 1)))
                fault_grid_eps.bit.b8 = 0;
        }
    } else if (fault_grid_eps.bit.b11) {
        if (++eps_fault_clear_count > 1500) {
            eps_fault_clear_count = 0;
            if (++eps_trip_count >= 3)
                run_status_bits.bit.b6 = 1;
            if (!(((run_status_bits.all >> 6) & 1) & ((state_flags.all >> 1) & 1)))
                fault_grid_eps.bit.b11 = 0;
        }
    } else {
        eps_fault_clear_count = 0;
    }
    if (run_status_bits.bit.b6) {
        if (++eps_lockout_timer > 180000) {
            run_status_bits.bit.b6 = 0;
            eps_trip_count = 0;
            eps_lockout_timer = 0;
        }
    } else {
        eps_lockout_timer = 0;
    }
}

/* Checks the power the ARM measures (ac.arm_power, from ARM page 1 bytes 8..11), outside the EPS
   states: above 1.5 x rated for 25 ticks, or above 1.1 x rated (0.9 x with batt_force_mode 1) for
   500 ticks, opens relay_c and sets bit 3 of the ARM fault word. CF19 bit 7 is set while over.
   The trip resets after 1500 ticks (30 s) twice; a third trip holds for 180000 ticks (1 hour). */
void arm_power_check(void) {
    float hi, lo;
    if (dsp_settings.batt_force_mode == 1) {
        hi = 1.5f * rating_params.rated_power;
        lo = 0.9f * rating_params.rated_power;
    } else {
        hi = 1.5f * rating_params.rated_power;
        lo = 1.1f * rating_params.rated_power;
    }
    if (op_state != OP_EPS_PV && op_state != OP_EPS_BATT) {
        if (ac.arm_power > hi) {
            arm_power_fast_count++;
            batt_blocks.bit.b7 = 1;
            arm_power_slow_count++;
            if (arm_power_fast_count >= 25) {
                arm_power_fast_count = 0;
                relay_cmd.relay_c = 1;
                arm_fault_word.bit.b3 = 1;
            }
        } else if (ac.arm_power > lo) {
            arm_power_slow_count++;
            batt_blocks.bit.b7 = 1;
            arm_power_fast_count = 0;
            if (arm_power_slow_count >= 500) {
                arm_fault_word.bit.b3 = 1;
                arm_power_slow_count = 0;
                relay_cmd.relay_c = 1;
            }
        } else {
            arm_power_fast_count--;
            if (--arm_power_slow_count < 0)
                arm_power_slow_count = 0;
            if (arm_power_fast_count < 0)
                arm_power_fast_count = 0;
        }
        if (relay_cmd.relay_c == 1) {
            if (++arm_power_trip_timer > 1500) {
                if (arm_power_trips < 2) {
                    arm_power_trips++;
                    arm_fault_word.bit.b3 = 0;
                    relay_cmd.relay_c = 0;
                    arm_power_trip_timer = 0;
                } else if (arm_power_trip_timer > 180000) {
                    arm_fault_word.bit.b3 = 0;
                    arm_power_trips = 0;
                    relay_cmd.relay_c = 0;
                    arm_power_trip_timer = 0;
                }
            }
        }
    } else {
        if (!state_flags.bit.b3) {
            arm_power_trips = 0;
            arm_power_trip_timer = 0;
        }
        arm_fault_word.bit.b3 = 0;
    }
}

/* Active power (mean of v x i over the cycle / 10, calibrated by cal_power / 1024) and the power
   factor. With no Q mode the apparent power is estimated from the PF code; otherwise
   S = sqrt(P^2 + Q^2) with Q task 4's setpoint, and the power factor is P / S x 1000, negative
   when the reactive current reference is above 0.5 A, 1000 within +-0.5 A. pf_code = PF x 10 +
   10000 (10000 is unity). */
void grid_power_pf_calc(void) {
    unsigned int n = ac_sums.n_grid;
    float re, im, c;
    if (n != 0) {
        ac.active_p_raw = (0.1f * ac_sums.grid_p) / n;
        ac.active_p = 0.0009765625f * (ac.active_p_raw * cal_power);
    }
    active_power = ac.active_p;
    if (q_mode == 0) {
        pf_denom = fabs(1000.0f - 0.1f * ac.pf_code);
        if (pf_denom != 0.0f)
            apparent_power_est = 1000.0f * active_power / pf_denom;
        else
            apparent_power_est = active_power;
        ac.apparent_p = fabs(1.004f * apparent_power_est);
    } else {
        re = ac.active_p;
        im = q_setpoint;
        ac.apparent_p = sqrtf(re * re + im * im);
        c = q_current_ref;
        if (c > 5.0f) {
            if (ac.apparent_p != 0.0f)
                ac.pf_x1000 = -(int)(1000.0f * active_power / ac.apparent_p);
        } else if (c >= -5.0f) {
            ac.pf_x1000 = 1000;
        } else {
            if (ac.apparent_p != 0.0f)
                ac.pf_x1000 = 1000.0f * active_power / ac.apparent_p;
        }
        if (ac.pf_x1000 > 1000)
            ac.pf_x1000 = 1000;
        else if (ac.pf_x1000 < -1000)
            ac.pf_x1000 = -1000;
        ac.pf_code = ac.pf_x1000 * 10 + 10000;
    }
}
