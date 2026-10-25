// clang-format off
/* The converter control module: the control interrupt and the loops it runs, with the module's own
   variables at 0xC000 .. 0xC07F.

   Entry points (interrupts): control_isr (EPWM4_INT, every PWM period: 19.2 kHz, PIE group 3),
   tz_trip_isr (EPWM3 trip zone, group 2), grid_period_isr (ECAP1 capture of the grid period,
   group 4). Init and stop functions called by the state machine (op_states.c) and task 3:
   grid_ctrl_init, dcdc_ctrl_init, pv_ctrl_init, pv_ctrl_reset, pv1_stop, pv2_stop, pv_stop_all.

   The control interrupt, in order (control_isr):
     1. wait for ADCINT1, adc_scale (ADC to floats), meas_avg8 (eight-sample averages; it also
        steps ctrl_slot 0..7 at 0xC83A), grid_pll_run (PLL), island_detect_run;
     2. a start request (state_flags bit 12 EPS, bit 13 grid) sets inv_on and clears the trips;
     3. iso_dcdc_pwm (EPWM7/EPWM8, the isolated stage), fast_fault_check, converter_mode (EPWM5,
        the battery buck/boost and its current controller), grid_angle_update;
     4. the bridge (EPWM3/EPWM4): grid_ictl_run in op_states 5 and 6, eps_vctl_run in 7 and 8, the
        grid voltage as a feed-forward only while state_flags bit 15 is set, else forced off;
     5. grid_rms_accum, relay_drive, q_loop_run;
     6. the PV boosts (EPWM1A/1B): bus precharge in the start state, then pv_bus_limit, pv1_ctrl,
        pv2_ctrl; the compares the PV loops left pending are loaded at the end;
     7. dc_acc_step and eps_rms_accum hand per-cycle sums to tasks 3 and 1.
   The eight-slot schedule (ctrl_slot, 0xC83A): slow loops run on one sample in eight (2.4 kHz):
   slot 0 the grid bus voltage PI and PV1's MPPT voltage PI, slot 1 PV2's voltage PI, slot 2 the PV
   precharge PIs, slot 4 the battery converter's outer loops, slot 6 the PV bus limiter. Slots 3,
   5 and 7 run nothing extra.

   The battery path has two stages (docs/names/C-notes.md, finding 4): EPWM7/EPWM8 drive an
   isolated, phase-shifted DC/DC from the battery to its high side (about 5.5 x the battery
   voltage; soft-started by soft_start in op_states.c, duty dcdc_ref.soft_duty up to 1054), and
   EPWM5 is a buck/boost between that high side (hv_v) and the DC link (bus_v). The current the
   controller regulates is the high-side current hv_i.

   Units: voltages in 0.1 V; inverter current in 0.1 A; hv_i in 0.01 A in meas_fast (0.1 A once
   converter_mode scales it); PV currents in 0.01 A. Compare values are counts of the 2343-count
   PWM period (pwm_period).
   Neighbours: ac_ctrl.c, current_ctrl.c, pll.c, ctrl_glue.c, island_detect.c, pi_ctrl.c;
   adc_scale.c, meas_avg8.c, dc_avg.c, grid_rms_accum.c, eps_rms_accum.c, avg_win.c;
   rcmu_fast_check.c; pwm_force.c; power_limits.c (task 2: the battery references) and
   grid_support.c (task 4: the grid current bounds and reactive reference).
   name: descriptive (was ctrl.c; map/filenames.tsv).
   match: the variables are the module's own globals, not a struct: with globals of the same file
   the compiler shares the data page and schedules the stores as the image does (a struct or
   extern gave other store orders, docs/notes.md).
   at: converter_mode 0x3F23E0, fast_fault_check 0x3F2DF4, control_isr 0x3F3039 (ramfuncs, load
   addresses), tz_trip_isr 0x3F11FF, pv_ctrl_reset 0x3EFDD9, pv1_stop 0x3EFF8A, pv2_stop 0x3EFFC6,
   pv_stop_all 0x3F0004; ramfuncs iso_dcdc_pwm 0x3F2399, eps_vctl_run 0x3F2213, relay_drive
   0x3F29C7, island_detect_run 0x3F2A1E, grid_v_check 0x3F2EFB, pv_bus_limit 0x3F2A5E,
   dcdc_ctrl_reset 0x3F259C; pv_ctrl_init 0x3EFD04, dcdc_ctrl_init 0x3F0060; ramfuncs pv1_ctrl
   0x3F2B04, pv2_ctrl 0x3F2BD4, grid_angle_update 0x3F2F4C; interrupt grid_period_isr 0x3F2F9E,
   grid_ictl_run 0x3F2252, grid_ictl_clear 0x3F296A (ramfuncs); grid_ctrl_init 0x3EFE48 */
// clang-format on
#define C_CTRL_EPS_TAIL_WITH_CYCLE
/* match: this file sees the grid angle's step count, index and wrap flag (0xC0E4..) as members
 * of eps_vctl_tail (stores to them keep their order relative to the angle's), so eps_cycle is
 * that object here */
#define OWN_eps_cycle
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#define OWN_grid_ictl_blk /* match: 0xC400.. as one object of this file's layout (grid_ictl_obj_t); the header has the C page view */
#define OWN_mppt_flags /* match: a volatile plain word here; the header has the union bits16_u view */
#define OWN_power_limits /* match: its first word as a volatile plain word here; op_states.c defines the struct power_limits_t */
#include <string.h>
#include "F2806x_Device.h"
#include "F2806x_EPwm.h"
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "meas.h"
#include "control.h"
#include "state.h"
#include "mppt.h"
#include "init.h"
#include "F2806x_Adc.h"
#include "F2806x_PieCtrl.h"
#include "F2806x_Gpio.h"
#include "F2806x_ECap.h"
#include "rating.h"
#define eps_cycle eps_vctl_tail          /* match: see OWN_eps_cycle */
extern struct fault_words_t fault_words; /* match: own form, see OWN_fault_words */

/* 0xC400 .. 0xC47F as one object (the block grid_ictl works on starts here; grid_ictl_t in
 * control.h has the same layout with the PI and the repetitive filter as sub-blocks).
 * match: the scheduler orders stores to members of one object, docs/notes.md */
struct grid_ictl_obj_t {
    float v_ff;
    float i_meas;
    float i_ref_p;
    float i_ref_q;
    float q_cmd;
    float bus_v;
    float bus_v_ref;
    float mod_gain;
    float amp_ramp;
    float amp_max;
    float amp_min;
    float sin_ref;
    float cos_ref;
    float kp_i;
    Uint16 ramp_mode;
    Uint16 rep_lead;
    Uint16 rep_idx;
    Uint16 rep_len;
    float i_ref;
    float i_err;
    float v_cmd;
    float island_inj;
    float q_corr;
    Uint32 xC42A;
    float amp_cmd;
    float pi_e;
    float pi_kp;
    float pi_ki;
    float pi_kd;
    float pi_umax;
    float pi_umin;
    Uint16 xC43A;
    Uint16 xC43B;
    float pi_i;
    float pi_u;

    Uint16 xC440;
    Uint16 xC441;
    float rep_in;
    float rep_max;
    float rep_min;
    float rep_a;
    float rep_b;
    float rep_gain;
    float rep_y;
    float rep_out;
    Uint16 xC452;
    Uint16 xC453;
    Uint16 xC454;
    Uint16 xC455;
    Uint16 xC456;
    Uint16 xC457;
    Uint16 xC458;
    Uint16 xC459;
    Uint16 xC45A;
    Uint16 xC45B;
    Uint16 xC45C;
    Uint16 xC45D;
    Uint16 xC45E;
    Uint16 xC45F;
    Uint16 xC460;
    Uint16 xC461;
    Uint16 xC462;
    Uint16 xC463;
    Uint16 xC464;
    Uint16 xC465;
    Uint16 xC466;
    Uint16 xC467;
    Uint16 xC468;
    Uint16 xC469;
    Uint16 xC46A;
    Uint16 xC46B;
    Uint16 xC46C;
    Uint16 xC46D;
    Uint16 xC46E;
    Uint16 xC46F;
    Uint16 xC470;
    Uint16 xC471;
    Uint16 xC472;
    Uint16 xC473;
    Uint16 xC474;
    Uint16 xC475;
    Uint16 xC476;
    Uint16 xC477;
    Uint16 xC478;
    Uint16 xC479;
    Uint16 xC47A;
    Uint16 xC47B;
    Uint16 xC47C;
    Uint16 xC47D;
    Uint16 xC47E;
    Uint16 xC47F;
};
extern struct grid_ictl_obj_t grid_ictl_blk;
#define grid_rep grid_ictl_blk

// clang-format off
// ebss-begin
Uint16 dis_current_bound_i = 0;
Uint16 hr4000_value = 0;
Uint16 grid_cycle_samples = 0x0180;  /* samples per grid cycle: 384 (50 Hz) or 320 (60 Hz), grid_freq_setup */
Uint16 pv1_ref_isr = 0;
volatile int16 inv_duty;  /* the bridge duty, +-pwm_period (inv_pwm_update) */
int16 pwm_period = 0x0927;  /* 2343 counts: 90 MHz / (2 x 2343) = 19.2 kHz up-down */
Uint16 force_chg_soc_isr = 0;
Uint16 chg_current_bound_i = 0;
Uint16 relay_sync_b = 0;  /* second relay close sample in the cycle */
volatile Uint16 dcdc_soft_lim = 0;  /* EPWM5 duty ceiling of the aging-test soft start */
Uint16 inv_on = 0;      /* name: set on either start bit (grid 12, EPS 13), cleared by inv_off and the fast fault; the bridge runs while 1 */
Uint16 dcdc_soft_done = 0;  /* 1: the aging-test soft start of EPWM5 is done */
Uint16 batt_current_cmd_i = 0;
Uint16 state_trace = 0;
Uint16 conv_mode = 0;  /* battery converter mode: 5, 6 grid (current command), 7, 8 EPS (bus PI), else off */
Uint16 tz_trip_count = 0;  /* trip-zone interrupts seen */
Uint16 dcdc_duty;  /* EPWM5 compare */
int16 inv_cmp_a;
Uint16 bms_hr15_copy = 0;
Uint16 soft_start_phase = 0;  /* isolated stage: 4 or 50 run EPWM7/EPWM8 (op_states.c soft_start) */
Uint16 fast_trip_code = 0;  /* last fast trip: 1 grid peak, 2 inverter current, 3 bus OV, 4 HV current, 5 HV OV, 6 battery fault */
int16 inv_cmp_b;
Uint16 pv2_ref_isr = 0;
Uint16 hv_ov_count = 0;  /* samples with the high side over 560.0 V */
Uint16 pv1_boost_on;
Uint16 hv_oc_count = 0;  /* samples with the high-side current over the limit */
Uint16 relay_sync_a = 0;  /* first relay close sample in the cycle */
Uint16 ecap_valid_count;  /* grid periods captured, up to 100 (task 4 needs 10) */
Uint16 bus_high = 0;  /* 1 above 520.0 V bus, 0 below 480.0 V: caps the grid current at 23.5 A peak */
Uint16 pv2_boost_on;
Uint16 grid_buf_clear_idx = 0;  /* grid_ictl_clear clears one word of the 430-word ring per call */
Uint16 dcdc_soft_div = 0;  /* divides the soft start: one count per 50 samples */
Uint16 batt_ov_count = 0;  /* samples with the battery over 63.0 V */
Uint16 grid_vpk_count = 0;  /* consecutive samples with |grid v| over 396.0 V */
float pll_cos;
Uint32 ecap_last;
Uint32 ecap_delta;
float pll_sin;
Uint32 ecap_period;  /* grid period in 90 MHz counts, at most 2250000 (40 Hz) */
struct leak_acc_t leak_fast_acc;
struct relay_ctl_t relay_ctl;
struct pv_bus_pi_t pv_bus_pi;
struct island_det_t island_det;
// ebss-end
// clang-format on

/* clamp v to [min, max] through a copy of v */
#define SAT_TERNARY(v, max, min)                                                                   \
    {                                                                                              \
        float t_ = (v);                                                                            \
        (v) = (t_ > (max)) ? (max) : __fmax(t_, (min));                                            \
    }

#pragma CODE_SECTION(converter_mode, "ramfuncs");
/* The battery converter, once per sample. conv_mode 5/6 gives current-controller mode 1 (grid:
   the current command from task 2), 7/8 gives mode 2 (EPS: the battery side holds the bus
   voltage), anything else mode 0. The references and limits task 2 left in dcdc_ref (0xC200..) are
   copied into the controller block with the measurements; bus_band is 10.0 V.
   - Running (mode 1 or 2, not low_batt_wake): the wake PIs are cleared; if the bus is more than
     30.0 V above its reference the full-battery trim is dropped; current_controller runs and its
     duty, clamped to 500..2300 counts, goes to EPWM5. In aging_test_mode 1 the duty is held under
     dcdc_soft_lim, which rises one count per 50 samples to 2300 (or jumps there once the battery
     is above 50.0 V), with EPWM5B forced until that is done.
   - Low-battery wake (low_batt_wake, the isolated stage at full duty 1054, 0xCF1A = 2):
     wake_batt_pi drives EPWM5 directly (1..900 counts) to bring the battery terminal to a fixed
     56.0 V (560); its integrator is cleared above 59.0 V, with the high side over 450.0 V, or with
     the battery over 30.0 V and the bus under 320.0 V. wake_bus_pi (bus to 320.0 V) is computed
     every sample but never used (docs/findings-control.md).
   - Otherwise EPWM5 is forced off and the controller cleared. */
void converter_mode(void) {
    Uint16 m;

    if (conv_mode == 5 || conv_mode == 6) {
        m = 1;
    } else if (conv_mode == 7 || conv_mode == 8) {
        m = 2;
    } else {
        m = 0;
    }
    dcdc_cc.mode = m;
    dcdc_cc.i_limit = dcdc_ref.i_limit;
    dcdc_cc.i_cmd_max = dcdc_ref.i_cmd_max;
    dcdc_cc.chg_bound = dcdc_ref.chg_bound;
    dcdc_cc.dis_bound = dcdc_ref.dis_bound;
    dcdc_cc.i_cmd = dcdc_ref.i_cmd;
    dcdc_cc.batt_v_max = dcdc_ref.batt_v_max;
    dcdc_cc.batt_v_min = dcdc_ref.batt_v_min;
    dcdc_cc.bus_v_ref = grid_ictl_blk.bus_v_ref;
    dcdc_cc.hv_i = 0.1f * meas_fast.hv_i;
    dcdc_cc.batt_v = meas_fast.batt_v;
    dcdc_cc.bus_v = meas_avg.bus_v_filt;
    dcdc_cc.duty_ff = dcdc_ref.duty_ff;
    dcdc_cc.i_offset = dcdc_ref.i_offset;
    dcdc_cc.bus_band = 100.0f;
    dcdc_cc.batt_full = (fault_words.batt_blocks & BB_FULL) >> 8;
    dcdc_cc.slot = grid_rep_tail.ctrl_slot;
    dcdc_cc.bms_mode = dsp_settings.battery_type;
    if (dcdc_cc.mode != 0 && low_batt_wake == 0) {
        dcdc_ref.wake_batt_pi.e1 = 0;
        dcdc_ref.wake_batt_pi.i = 0;
        dcdc_ref.wake_batt_pi.u = 0;
        dcdc_ref.wake_bus_pi.e1 = 0;
        dcdc_ref.wake_bus_pi.i = 0;
        dcdc_ref.wake_bus_pi.u = 0;
        if (dcdc_cc.bus_v - dcdc_cc.bus_v_ref > 300.0f) {
            dcdc_cc.i_offset = 0;
        }
        current_controller(&dcdc_cc);
        SAT_TERNARY(dcdc_cc.duty, 2300.0f, 500.0f);
        dcdc_duty = (Uint16)(dcdc_cc.duty + 0.5f);
        if (dsp_settings.aging_test_mode == 1) {
            dcdc_soft_div++;
            if (dcdc_soft_div >= 50) {
                Uint16 n_ = ++dcdc_soft_lim;
                dcdc_soft_div = 0;
                if (n_ >= 2300) {
                    dcdc_soft_lim = 2300;
                    goto start;
                }
            } else if (dcdc_soft_lim >= 2300) {
                goto start;
            }
            if (meas_avg.batt_v > 500.0f) {
            start:
                if (dcdc_soft_done == 0) {
                    dcdc_soft_lim = 2300;
                    dcdc_soft_done = 1;
                    dcdc_cc.pi_v_max.i = 0;
                    dcdc_cc.pi_i.i = 0;
                }
            }
            dcdc_cc.start_ok = 1;
            if (dcdc_duty >= dcdc_soft_lim)
                dcdc_duty = dcdc_soft_lim;
        } else {
            dcdc_soft_done = 0;
            dcdc_soft_lim = 0;
            dcdc_soft_div = 0;
        }
        EPwm5Regs.CMPA.half.CMPA = dcdc_duty;
        if (dcdc_soft_done == 0 && dsp_settings.aging_test_mode == 1) {
            EPwm5Regs.AQCSFRC.bit.CSFA = 0;
            EPwm5Regs.AQCSFRC.bit.CSFB = 2;
        } else {
            pwm5_release();
        }
    } else if (low_batt_wake == 1 && dcdc_ref.soft_duty == 0x041E &&
               fault_words.precharge_step == 2) {
        dcdc_soft_lim = 0;
        dcdc_soft_done = 0;
        if (dcdc_ref.wake_batt_pi.u > 2.0f) {
            EPwm5Regs.AQCSFRC.bit.CSFA = 0;
            EPwm5Regs.AQCSFRC.bit.CSFB = 2;
        }
        dcdc_ref.wake_batt_pi.e = 560.0f - meas_fast.batt_v;
        SAT_TERNARY(dcdc_ref.wake_batt_pi.e, 100.0f, -100.0f);
        pi_step(&dcdc_ref.wake_batt_pi);
        if (meas_fast.batt_v > 590.0f || meas_fast.hv_v > 4500.0f ||
            (meas_fast.batt_v > 300.0f && meas_fast.bus_v < 3200.0f)) {
            dcdc_ref.wake_batt_pi.i = 0;
        }
        dcdc_ref.wake_bus_pi.e = 3200.0f - meas_avg.bus_v_filt;
        SAT_TERNARY(dcdc_ref.wake_bus_pi.e, 100.0f, -100.0f);
        pi_step(&dcdc_ref.wake_bus_pi);
        {
            float t_ = dcdc_ref.wake_batt_pi.u;
            t_ = (t_ > 900.0f) ? 900.0f : __fmax(t_, 1.0f);
            EPwm5Regs.CMPA.half.CMPA = (Uint16)t_;
        }
    } else {
        dcdc_soft_lim = 0;
        dcdc_soft_done = 0;
        pwm5_force_off();
        dcdc_ctrl_reset();
    }
}

#pragma CODE_SECTION(fast_fault_check, "ramfuncs");
/* The fast protection, once per sample:
   - grid_v_check (the grid voltage peak or window);
   - inverter current |inv_i| over 400 (40.0 A) in op_states 5..8: PV boosts (EPWM1) forced low at
     once, the bridge stopped (inv_on = 0), MPPT rescan;
   - bus voltage (8-sample mean) over bus_ov_limit (0xD8C2, 585.0 V): EPWM5 one-shot trip, PV
     stopped, bridge off, conv_mode 0, event 1 to task 0 (stop);
   - high-side current |hv_i| over 2000 (20.00 A) on 2 samples (reset under 10.00 A), or high-side
     voltage over 5600 (560.0 V) on 5 samples (counted, not consecutive): EPWM5 trip and stop;
   - the residual current fast mean: every 10 samples rcmu_i_fast (A) = the mean of leak_i (mA)
     / 1000, and rcmu_fast_check runs;
   - a battery fault (has_batt_fault) outside the wake and aging test modes stops the converter;
   - battery voltage (8-sample mean) over 630 (63.0 V) for more than 40 samples (counted, not
     consecutive): EPWM5 trip and stop. This check is skipped while aging_test_enable (HR332,
     0xD8EF) is set (docs/findings-control.md). */
void fast_fault_check(void) {
    grid_v_check();
    if (fabs(meas_fast.inv_i) > 400.0f && op_state >= OP_GRID_PV) {
        EPwm1Regs.AQCSFRC.bit.CSFA = 1;
        EPwm1Regs.AQCSFRC.bit.CSFB = 1;
        fast_trip_code = 2;
        inv_on = 0;
        mppt.rescan = 1;
        pv_ctrl_reset();
    }
    if (meas_avg.bus_v > dsp_settings.bus_ov_limit) {
        fast_trip_code = 3;
        soft_start_phase = 0;
        conv_mode = 0;
        low_batt_wake = 0;
        EALLOW;
        EPwm5Regs.TZFRC.bit.OST = 1;
        EDIS;
        pv_stop_all();
        inv_off();
        os_post_isr(TASK_STATE, EV0_STOP);
    }
    if (fabs(meas_fast.hv_i) > 2000.0f) {
        hv_oc_count++;
        low_batt_wake = 0;
        fast_trip_code = 4;
        if (hv_oc_count >= 2) {
            hv_oc_count = 0;
            soft_start_phase = 0;
            conv_mode = 0;
            EALLOW;
            EPwm5Regs.TZFRC.bit.OST = 1;
            EDIS;
            os_post_isr(TASK_STATE, EV0_STOP);
        }
    } else if (fabs(meas_fast.hv_i) < 1000.0f) {
        hv_oc_count = 0;
    }
    if (meas_fast.hv_v > 5600.0f) {
        hv_ov_count++;
        low_batt_wake = 0;
        fast_trip_code = 5;
        if (hv_ov_count >= 5) {
            hv_ov_count = 0;
            soft_start_phase = 0;
            conv_mode = 0;
            EALLOW;
            EPwm5Regs.TZFRC.bit.OST = 1;
            EDIS;
            os_post_isr(TASK_STATE, EV0_STOP);
        }
    }
    leak_fast_acc.count++;
    leak_fast_acc.sum += meas_fast.leak_i;
    if (leak_fast_acc.count >= 10) {
        rcmu_i_fast = leak_fast_acc.sum * 0.0001f;
        leak_fast_acc.sum = 0;
        leak_fast_acc.count = 0;
        rcmu_fast_check();
    }
    if (has_batt_fault() == 1 && low_batt_wake == 0 && dsp_settings.aging_test_mode != 1) {
        fast_trip_code = 6;
        soft_start_phase = 0;
        conv_mode = 0;
        soft_start_state = 0;
    }
    if (dsp_settings.aging_test_enable == 0) {
        if (meas_avg.batt_v > 630.0f) {
            batt_ov_count++;
            low_batt_wake = 0;
            if (batt_ov_count > 40) {
                batt_ov_count = 0;
                soft_start_phase = 0;
                conv_mode = 0;
                EALLOW;
                EPwm5Regs.TZFRC.bit.OST = 1;
                EDIS;
                os_post_isr(TASK_STATE, EV0_STOP);
            }
        }
    } else {
        batt_ov_count = 0;
    }
}

extern volatile Uint16 power_limits;

extern volatile Uint16 mppt_flags;

#pragma CODE_SECTION(grid_ictl_run, "ramfuncs");
/* Fills the grid current controller block and runs grid_ictl: inverter current, grid voltage
   plus adc_scale's lead term as the feed-forward, the filtered bus voltage, the PLL's sin/cos,
   sample index and period, the islanding and reactive loop outputs. While bus_high is set the
   peak current bounds task 4 wrote are cut to +-235 (23.5 A) until task 4 writes them again;
   bus_high has hysteresis between 480.0 V and 520.0 V. */
void grid_ictl_run(void) {
    grid_ictl_blk.i_meas = meas_fast.inv_i;
    grid_ictl_blk.v_ff = meas_fast.grid_v + grid_v_hist.lead;
    grid_ictl_blk.bus_v = meas_avg.bus_v_filt;
    grid_ictl_blk.sin_ref = pll_sin;
    grid_ictl_blk.cos_ref = pll_cos;
    grid_ictl_blk.ramp_mode = pv_bus_mode;
    grid_ictl_blk.rep_idx = grid_pll.n;
    grid_ictl_blk.rep_len = grid_pll.period;
    grid_ictl_blk.island_inj = island_det.out;
    grid_ictl_blk.q_corr = q_loop.u;
    if (bus_high == 1) {
        grid_ictl_blk.amp_max = __fmin(grid_ictl_blk.amp_max, 235.0f);
        grid_ictl_blk.amp_min = __fmax(grid_ictl_blk.amp_min, -235.0f);
    }
    if (meas_avg.bus_v >= 5200.0f) {
        bus_high = 1;
    } else if (meas_avg.bus_v <= 4800.0f) {
        bus_high = 0;
    }
    grid_ictl((grid_ictl_t *)&grid_ictl_blk);
}

#pragma CODE_SECTION(inv_pwm_update, "ramfuncs");
/* The bridge compares from the signed duty inv_duty (+-pwm_period): unipolar modulation, EPWM3
   takes duty + period and EPWM4 the duty. A leg whose compare reaches 0 or the period gets
   shadow load mode 1 (load at period) instead of 0 (load at zero), and is pinned there.
   match: the three reads of C00C come first, then the stores to C00B and C009: the stores are
   written through volatile lvalues so that 15.12 keeps them after the sign test's read, as
   the image does. */
void inv_pwm_update(void) {
    int16 s, b, t;
    if (inv_duty > pwm_period)
        inv_duty = pwm_period;
    else if (inv_duty < -pwm_period)
        inv_duty = -pwm_period;
    b = inv_duty;
    s = inv_duty + pwm_period;
    t = inv_duty;
    *(volatile int16 *)&inv_cmp_b = b;
    *(volatile int16 *)&inv_cmp_a = s;
    if (t >= 0) {
        EPwm3Regs.CMPCTL.bit.LOADAMODE = 0;
        EPwm3Regs.CMPCTL.bit.LOADBMODE = 0;
        inv_cmp_a = pwm_period;
    } else if (s <= 0) {
        EPwm3Regs.CMPCTL.bit.LOADAMODE = 1;
        EPwm3Regs.CMPCTL.bit.LOADBMODE = 1;
        inv_cmp_a = 0;
    }
    if (inv_cmp_b >= pwm_period) {
        EPwm4Regs.CMPCTL.bit.LOADAMODE = 0;
        EPwm4Regs.CMPCTL.bit.LOADBMODE = 0;
        inv_cmp_b = pwm_period;
    } else if (inv_cmp_b <= 0) {
        EPwm4Regs.CMPCTL.bit.LOADAMODE = 1;
        EPwm4Regs.CMPCTL.bit.LOADBMODE = 1;
        inv_cmp_b = 0;
    }
    EPwm4Regs.CMPA.half.CMPA = inv_cmp_b;
    EPwm3Regs.CMPA.half.CMPA = inv_cmp_a;
}

#pragma CODE_SECTION(control_isr, "ramfuncs");
/* The control interrupt (EPWM4_INT, 19.2 kHz). The order of the steps is in the file header. The
   bridge runs while inv_on is 1 or state_flags bit 15 is set: in op_states 5/6 the grid current
   controller (the EPS controller kept clear), with bit 15 alone the grid voltage times the
   modulation gain (the bridge follows the grid, a pre-sync; inference), in 7/8 the EPS voltage
   controller (the grid controller kept clear); otherwise EPWM3/EPWM4 are forced off and both
   controllers cleared.
   PV side: the word at 0xCF1A (power_limits) is 1 or 2 during the start's bus precharge; when the
   bus is within 10.0 V of its reference (or the word is 2) it becomes 3 in op_states 5..8, and in
   the start state with the word at 2 the boosts precharge the bus on slot 2 (PI on bus_v_ref -
   bus, +-20.0 V). With the word at 3 and PV in use (op_state 5 or 7, or 0xD178 bit 14), the PV
   loops run; pv_ramp_tick counts samples up to one cycle for their soft current ceiling. */
interrupt void control_isr(void) {
    while (AdcRegs.ADCINTFLG.bit.ADCINT1 == 0) {
    }
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    adc_scale();
    meas_avg8();
    grid_pll_run();
    island_detect_run();
    if (state_flags.bit.b12 || state_flags.bit.b13) {
        state_flags.all &= 0xCFFF;
        inv_on = 1;
        epwm_trip_clear();
    }
    iso_dcdc_pwm();
    fast_fault_check();
    converter_mode();
    grid_angle_update();
    if (inv_on == 1 || state_flags.bit.b15) {
        if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
            eps_vctl_clear();
            grid_ictl_run();
            inv_duty = (int16)(grid_ictl_blk.v_cmd * grid_ictl_blk.mod_gain);
            inv_pwm_update();
            pwm34_release();
        } else if (state_flags.bit.b15) {
            grid_ictl_blk.v_cmd = meas_fast.grid_v;
            inv_duty = (int16)(grid_ictl_blk.v_cmd * grid_ictl_blk.mod_gain);
            inv_pwm_update();
            pwm34_release();
        } else if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT) {
            grid_ictl_clear();
            eps_vctl_run();
            inv_duty = (int16)(eps_vctl_tail.duty * eps_vctl_tail.mod_gain);
            inv_pwm_update();
            pwm34_release();
        } else {
            goto off;
        }
    } else {
    off:
        pwm34_force_off();
        eps_vctl_clear();
        grid_ictl_clear();
    }
    grid_rms_accum();
    relay_drive();
    q_loop_run();
    if (meas_avg.bus_v + 100.0f > bus_v_ref || power_limits == 2) {
        if (op_state >= OP_GRID_PV) {
            power_limits = 3;
        }
        if (power_limits == 2 && op_state == OP_START && ctrl_slot == 2) {
            if (pv1_boost_on == 1) {
                pv_phase[0].pi_precharge.e = grid_ictl_blk.bus_v_ref - meas_avg.bus_v_filt;
                {
                    float t_ = pv_phase[0].pi_precharge.e;
                    pv_phase[0].pi_precharge.e = (t_ >= 200.0f) ? 200.0f : __fmax(t_, -200.0f);
                }
                pi_step(&pv_phase[0].pi_precharge);
                pv_phase[0].cmp = (Uint16)pv_phase[0].pi_precharge.u;
                pv_phase[0].cmp_pending = 1;
            }
            if (pv2_boost_on == 1) {
                pv_phase[1].pi_precharge.e = grid_ictl_blk.bus_v_ref - meas_avg.bus_v_filt;
                if (pv_phase[1].pi_precharge.e >= 200.0f)
                    pv_phase[1].pi_precharge.e = 200.0f;
                else
                    pv_phase[1].pi_precharge.e = __fmax(pv_phase[1].pi_precharge.e, -200.0f);
                pi_step(&pv_phase[1].pi_precharge);
                pv_phase[1].cmp = (Uint16)pv_phase[1].pi_precharge.u;
                pv_phase[1].cmp_pending = 1;
            }
        }
    }
    if (power_limits == 3 &&
        (op_state == OP_GRID_PV || op_state == OP_EPS_PV || (mppt_flags & 0x4000))) {
        meas_avg.pv_ramp_tick++;
        meas_avg.pv1_v_ref = mppt.pv1_ref;
        meas_avg.pv2_v_ref = mppt.pv2_ref;
        if (meas_avg.pv_ramp_tick > eps_cycle.cycle_len) {
            meas_avg.pv_ramp_tick = 0;
        }
        pv_bus_limit();
        pv1_ctrl();
        pv2_ctrl();
    }
    if (pv1_boost_on == 0) {
        pv1_stop();
    }
    if (pv2_boost_on == 0) {
        pv2_stop();
    }
    if (pv_phase[0].cmp_pending == 1) {
        Uint16 v;
        v = pwm_period - pv_phase[0].cmp;
        EPwm1Regs.AQCSFRC.bit.CSFA = 0;
        pv_phase[0].cmp_pending = 0;
        EPwm1Regs.CMPA.half.CMPA = v;
    }
    if (pv_phase[1].cmp_pending == 1) {
        Uint16 v;
        EPwm1Regs.AQCSFRC.bit.CSFB = 0;
        v = pwm_period - pv_phase[1].cmp;
        pv_phase[1].cmp_pending = 0;
        EPwm1Regs.CMPB = v;
    }
    dc_acc_step();
    eps_rms_accum();
    EPwm4Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

/* Trip-zone interrupt (EPWM3 TZ): forces every PWM output off (EPWM1, 3, 4, 5), stops the battery
   converter and the bridge, asks for an MPPT rescan, sets CF12 bit 4 and counts the trip, then
   clears the one-shot trips on EPWM1/3/4/5 and disables the EPWM3 trip interrupt. */
interrupt void tz_trip_isr(void) {
    EPwm5Regs.AQCSFRC.bit.CSFA = 1;
    EPwm5Regs.AQCSFRC.bit.CSFB = 2;
    EPwm4Regs.AQCSFRC.bit.CSFA = 1;
    EPwm4Regs.AQCSFRC.bit.CSFB = 2;
    EPwm3Regs.AQCSFRC.bit.CSFA = 1;
    EPwm3Regs.AQCSFRC.bit.CSFB = 2;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;
    soft_start_phase = 0;
    conv_mode = 0;
    soft_start_state = 0;
    inv_off();
    mppt.rescan = 1;
    fault_words.fault_grid1 |= FG1_TZ_TRIP;
    tz_trip_count++;
    EALLOW;
    EPwm3Regs.TZCLR.bit.INT = 1;
    EPwm1Regs.TZCLR.bit.OST = 1;
    EPwm3Regs.TZCLR.bit.OST = 1;
    EPwm4Regs.TZCLR.bit.OST = 1;
    EPwm5Regs.TZCLR.bit.OST = 1;
    EPwm3Regs.TZEINT.bit.OST = 0;
    EDIS;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;
}

/* Stops PV1's boost (EPWM1A forced low, compare past the period) and clears its loops. */
void pv1_stop(void) {
    EPwm1Regs.CMPA.half.CMPA = pwm_period + 10;
    pv_phase[0].cmp = 0;
    pv_phase[0].pi_v.e1 = 0;
    pv_phase[0].pi_v.i = 0;
    pv_phase[0].v_loop_out = 0;
    pv_phase[0].pi_v.u = 0;
    pv_phase[0].duty = 0;
    pv1_boost_on = 0;
    pv_phase[0].pi_i.e1 = 0;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;
    pv_phase[0].pi_i.i = 0;
    pv_phase[0].pi_i.u = 0;
}

/* Stops PV2's boost (EPWM1B forced low, compare past the period) and clears its loops. */
void pv2_stop(void) {
    pv2_boost_on = 0;
    pv_phase[1].cmp = 0;
    pv_phase[1].v_loop_out = 0;
    pv_phase[1].pi_v.e1 = 0;
    pv_phase[1].pi_v.i = 0;
    pv_phase[1].pi_v.u = 0;
    pv_phase[1].duty = 0;
    pv_phase[1].pi_i.e1 = 0;
    pv_phase[1].pi_i.i = 0;
    pv_phase[1].pi_i.u = 0;
    EPwm1Regs.CMPB = pwm_period + 10;
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;
}

/* Stops both PV boosts and clears their loops and pending compares. */
void pv_stop_all(void) {
    pv1_boost_on = 0;
    pv2_boost_on = 0;
    pv_phase[0].cmp_pending = 0;
    pv_phase[1].cmp_pending = 0;
    pv_phase[0].cmp = 0;
    pv_phase[1].cmp = 0;
    pv_phase[0].v_loop_out = 0;
    pv_phase[0].pi_v.e1 = 0;
    pv_phase[0].pi_v.i = 0;
    pv_phase[0].pi_v.u = 0;
    pv_phase[1].v_loop_out = 0;
    pv_phase[1].pi_v.e1 = 0;
    pv_phase[1].pi_v.i = 0;
    pv_phase[1].pi_v.u = 0;
    pv_phase[0].pi_i.e1 = 0;
    pv_phase[0].pi_i.i = 0;
    pv_phase[0].pi_i.u = 0;
    pv_phase[1].pi_i.e1 = 0;
    pv_phase[1].pi_i.i = 0;
    pv_phase[1].pi_i.u = 0;
    EPwm1Regs.CMPA.half.CMPA = pwm_period + 10;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;
    EPwm1Regs.CMPB = pwm_period + 10;
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;
}

/* Clears both PV boosts' loops (precharge, voltage, current), their current references, soft
   ceilings and limits, and the PV bus limiter; called before an MPPT rescan (task 3) and by the
   fast inverter over-current trip. */
void pv_ctrl_reset(void) {
    pv1_boost_on = 0;
    pv2_boost_on = 0;
    pv_phase[0].pi_precharge.e1 = 0;
    pv_phase[0].pi_precharge.i = 0;
    pv_phase[0].pi_precharge.u = 0;
    pv_phase[0].v_loop_out = 0;
    pv_phase[0].pi_v.e1 = 0;
    pv_phase[0].pi_v.i = 0;
    pv_phase[0].pi_v.u = 0;
    pv_phase[0].i_ref = 0;
    pv_phase[0].pi_i.e1 = 0;
    pv_phase[0].pi_i.i = 0;
    pv_phase[0].pi_i.u = 0;
    pv_phase[1].pi_precharge.e1 = 0;
    pv_phase[1].pi_precharge.i = 0;
    pv_phase[1].pi_precharge.u = 0;
    pv_phase[1].v_loop_out = 0;
    pv_phase[1].pi_v.e1 = 0;
    pv_phase[1].pi_v.i = 0;
    pv_phase[1].pi_v.u = 0;
    pv_phase[1].i_ref = 0;
    pv_phase[1].pi_i.e1 = 0;
    pv_phase[1].pi_i.i = 0;
    pv_phase[1].pi_i.u = 0;
    pv_phase[0].i_ceiling = 0;
    pv_phase[1].i_ceiling = 0;
    meas_avg.pv_ramp_tick = 0;
    pv_phase[0].i_limit = 0;
    pv_phase[1].i_limit = 0;
    pv_bus_pi.p = 0;
    pv_bus_pi.i = 0;
    pv_bus_pi.u = 0;
    pv_bus_pi.spare = 0;
}

#pragma CODE_SECTION(iso_dcdc_pwm, "ramfuncs");
/* The isolated stage (EPWM7/EPWM8): while soft_start_phase is 4 or has bits 0x32 set (50), both
   modules get CMPA = period - soft_duty and CMPB = soft_duty and are released; during a battery
   calibration request (HR175, batt_cal_request) EPWM8 is forced low and only EPWM7 runs.
   Otherwise, when EPWM8's counter is in 1055..1153 (a fixed point of the period), both are forced
   low and the duty is reset to 1, so a restart begins from the minimum. */
void iso_dcdc_pwm(void) {
    if (soft_start_phase == 4 || (soft_start_phase & 0x32) == 0x32) {
        EPwm7Regs.CMPA.half.CMPA = 0x0927 - dcdc_ref.soft_duty;
        EPwm7Regs.CMPB = dcdc_ref.soft_duty;
        EPwm8Regs.CMPA.half.CMPA = 0x0927 - dcdc_ref.soft_duty;
        EPwm8Regs.CMPB = dcdc_ref.soft_duty;
        if (batt_cal_request == 0) {
            pwm78_release();
        } else {
            EPwm8Regs.AQCSFRC.bit.CSFA = 1;
            EPwm8Regs.AQCSFRC.bit.CSFB = 1;
            EPwm7Regs.AQCSFRC.bit.CSFA = 0;
            EPwm7Regs.AQCSFRC.bit.CSFB = 0;
        }
    } else if (EPwm8Regs.TBCTR > 1054 && EPwm8Regs.TBCTR < 1154) {
        pwm78_force_low();
        dcdc_ref.soft_duty = 1;
        EPwm7Regs.CMPA.half.CMPA = 0x0927 - dcdc_ref.soft_duty;
        EPwm7Regs.CMPB = dcdc_ref.soft_duty;
        EPwm8Regs.CMPA.half.CMPA = 0x0927 - dcdc_ref.soft_duty;
        EPwm8Regs.CMPB = dcdc_ref.soft_duty;
    }
}

#pragma CODE_SECTION(eps_vctl_run, "ramfuncs");
/* Fills the EPS voltage controller block and runs eps_vctl: mode from 0xD7C0 bit 9 (EPS enabled),
   the amplitude sqrt 2 x eps_v_ref (task 1's ramped RMS reference, 0xD3F4), sin of the EPS angle,
   inverter voltage and current, the voltage feed-forward, a filtered current (0.05 old + 0.95
   new), and a repetitive filter lead of 6 samples. */
void eps_vctl_run(void) {
    eps_vctl_blk.mode = (eps_cfg_bits.eps_cfg.all & 0x200) >> 9;
    eps_vctl_blk.v_amp_ref = 1.4142135f * eps_v_ref;
    eps_vctl_blk.sin_ref = sinf(eps_vctl_tail.theta);
    eps_vctl_blk.v_meas = meas_fast.inv_v;
    eps_vctl_blk.i_meas = meas_fast.inv_i;
    eps_vctl_blk.v_ff = meas_fast.inv_v;
    eps_vctl_blk.i_filt = 0.05f * eps_vctl_blk.i_filt + 0.95f * meas_fast.inv_i;
    eps_vctl_blk.rep_lead = 6;
    eps_vctl((eps_vctl_t *)&eps_vctl_blk);
}

#pragma CODE_SECTION(relay_drive, "ramfuncs");
/* Drives the three relay outputs from the state machine's commands (relay_ctl.relay_a..c, 0xC034).
   relay_a (GPIO12): 1 sets the output at one of the two sync samples of the PLL cycle (the
   switching point grid_freq_setup chose), and after 50 cycles' worth of samples the command
   becomes 2; 2 toggles the output every sample (a 9.6 kHz drive, a reduced hold current;
   inference); 0 clears it. relay_b (GPIO21) and relay_c (GPIO53): 1 on, 2 toggled, else off. */
void relay_drive(void) {
    if (relay_ctl.relay_a == 1) {
        if (grid_pll.n == relay_sync_a || grid_pll.n == relay_sync_b) {
            GpioDataRegs.GPASET.bit.GPIO12 = 1;
        }
        relay_ctl.relay_a_count++;
        if (relay_ctl.relay_a_count > grid_cycle_samples * 50) {
            relay_ctl.relay_a = 2;
            relay_ctl.relay_a_count = 0;
        }
    } else if (relay_ctl.relay_a == 2) {
        GpioDataRegs.GPATOGGLE.bit.GPIO12 = 1;
    } else {
        GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;
    }
    if (relay_ctl.relay_b == 1) {
        GpioDataRegs.GPASET.bit.GPIO21 = 1;
    } else if (relay_ctl.relay_b == 2) {
        GpioDataRegs.GPATOGGLE.bit.GPIO21 = 1;
    } else {
        GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;
    }
    if (relay_ctl.relay_c == 1) {
        GpioDataRegs.GPBSET.bit.GPIO53 = 1;
    } else if (relay_ctl.relay_c == 2) {
        GpioDataRegs.GPBTOGGLE.bit.GPIO53 = 1;
    } else {
        GpioDataRegs.GPBCLEAR.bit.GPIO53 = 1;
    }
}

#pragma CODE_SECTION(island_detect_run, "ramfuncs");
/* Once per grid cycle (PLL sample 100): in op_states 5/6 with 0xD7C2 bit 11 set it runs the
   islanding detector on 1000 x the PLL angle step and, on a flag, stops the bridge (inv_off) and
   asks for an MPPT rescan. Otherwise it resets the detector to the present step. */
void island_detect_run(void) {
    if (grid_pll.n == 100) {
        if ((op_state == OP_GRID_PV || op_state == OP_GRID_BATT) &&
            eps_cfg_bits.grid_func.bit.b11) {
            island_det.in = 1000.0f * grid_pll.w;
            island_detect(&island_det);
            if (island_det.flag == 1) {
                inv_off();
                mppt.rescan = 1;
            }
        } else {
            island_det.in = 1000.0f * grid_pll.w;
            island_det.cnt = 0;
            island_det.flag = 0;
            island_det.out = 0;
            island_det.hist1 = island_det.in;
            island_det.hist2 = island_det.in;
            island_det.hist3 = island_det.in;
        }
    }
}

#pragma CODE_SECTION(grid_v_check, "ramfuncs");
/* The fast grid voltage check. Normally: |grid v| over 3960 (396.0 V) on more than 76
   consecutive samples (4 ms) sets CF12 bit 12. With 0xD7C0 bit 10 set (eps_cfg, HR177 per
   docs/names/C-notes.md) it uses the window average instead: the mean of |v| over a quarter cycle
   (96 samples at 50 Hz, 80 at 60 Hz) outside 91..345 V sets the same bit. */
void grid_v_check(void) {
    if (!eps_cfg_bits.eps_cfg.bit.b10) {
        if (fabs(meas_fast.grid_v) > 3960.0f) {
            grid_vpk_count++;
            fast_trip_code = 1;
            if (grid_vpk_count > 76) {
                fault_words.fault_grid1 |= FG1_GRID_VOLT;
                grid_vpk_count = 0;
            }
        } else {
            grid_vpk_count = 0;
        }
    } else {
        grid_v_win.in = meas_fast.grid_v * 0.1f;
        grid_v_win.mode = dsp_settings.grid_60hz;
        grid_v_win.max = 345.0f;
        grid_v_win.min = 91.0f;
        avg_win_step(&grid_v_win);
        if (grid_v_win.ok == 0) {
            fault_words.fault_grid1 |= FG1_GRID_VOLT;
        }
    }
}

#pragma CODE_SECTION(pv_bus_limit, "ramfuncs");
/* The PV bus limiter, on slot 6: while a boost runs, a PI holds the bus at bus_v_ref + 10.0 V
   (op_state 7) or + 20.0 V (op_state 5 or 0xD178 bit 14): kp 2.32, ki 0.08, output +-1500
   (15.00 A), which pv1_ctrl and pv2_ctrl take as a ceiling on their current references. It also
   clamps both MPPT voltage references to 120.0..545.0 V. */
void pv_bus_limit(void) {
    float off;

    if (grid_rep_tail.ctrl_slot == 6) {
        if (pv1_boost_on == 1 || pv2_boost_on == 1) {
            /* the offset is left unset when CF0D is neither 5 nor 7 and D178 bit 14 is clear:
               latent only, control_isr calls this under the same condition
               (docs/findings-control.md) */
            if (op_state == OP_EPS_PV) {
                off = 100.0f;
            } else if (op_state == OP_GRID_PV || (mppt_flags & 0x4000)) {
                off = 200.0f;
            }
            pv_bus_pi.e = grid_ictl_blk.bus_v_ref + off - meas_avg.bus_v;
            pv_bus_pi.p = pv_bus_pi.e * pv_bus_pi.kp;
            pv_bus_pi.i = pv_bus_pi.e * pv_bus_pi.ki + pv_bus_pi.i;
            pv_bus_pi.i = (pv_bus_pi.i < -1500.0f) ? -1500.0f : __fmin(pv_bus_pi.i, 1500.0f);
            pv_bus_pi.u = pv_bus_pi.p + pv_bus_pi.i;
            pv_bus_pi.u = (pv_bus_pi.u < -1500.0f) ? -1500.0f : __fmin(pv_bus_pi.u, 1500.0f);
        } else {
            pv_bus_pi.i = 0;
            pv_bus_pi.u = 0;
        }
        {
            float t_ = meas_avg.pv1_v_ref;
            meas_avg.pv1_v_ref = t_ > 5450.0f ? 5450.0f : __fmax(t_, 1200.0f);
        }
        {
            float t_ = meas_avg.pv2_v_ref;
            meas_avg.pv2_v_ref = t_ > 5450.0f ? 5450.0f : __fmax(t_, 1200.0f);
        }
    }
}

#pragma CODE_SECTION(dcdc_ctrl_reset, "ramfuncs");
/* Clears the battery converter: the current controller's state, all seven PIs, the two wake PIs,
   the duty and the EPWM5 compare value. */
void dcdc_ctrl_reset(void) {
    dcdc_cc.start_ok = 0;
    dcdc_cc.i_cmd = 0;
    dcdc_cc.pi_bus_sync.e1 = 0;
    dcdc_cc.pi_bus_sync.i = 0;
    dcdc_cc.pi_bus_sync.u = 0;
    dcdc_cc.spare = 0;
    dcdc_cc.i_offset = 0;
    dcdc_cc.duty = 0;
    dcdc_cc.i_cmd_prev = 0;
    dcdc_cc.i_ref = 0;
    dcdc_cc.pi_bus.e = 0;
    dcdc_cc.pi_bus.e1 = 0;
    dcdc_cc.pi_bus.i = 0;
    dcdc_cc.pi_bus.u = 0;
    dcdc_cc.pi_bus_hi.e = 0;
    dcdc_cc.pi_bus_hi.e1 = 0;
    dcdc_cc.pi_bus_hi.i = 0;
    dcdc_cc.pi_bus_hi.u = 0;
    dcdc_cc.pi_bus_lo.e = 0;
    dcdc_cc.pi_bus_lo.e1 = 0;
    dcdc_cc.pi_bus_lo.i = 0;
    dcdc_cc.pi_bus_lo.u = 0;
    dcdc_cc.pi_v_max.e = 0;
    dcdc_cc.pi_v_max.e1 = 0;
    dcdc_cc.pi_v_max.i = 0;
    dcdc_ref.wake_batt_pi.e = 0;
    dcdc_cc.pi_v_max.u = 0;
    dcdc_ref.wake_batt_pi.e1 = 0;
    dcdc_cc.pi_v_min.e = 0;
    dcdc_ref.wake_batt_pi.i = 0;
    dcdc_cc.pi_v_min.e1 = 0;
    dcdc_ref.wake_batt_pi.u = 0;
    dcdc_cc.pi_v_min.i = 0;
    dcdc_ref.wake_bus_pi.e1 = 0;
    dcdc_cc.pi_v_min.u = 0;
    dcdc_cc.pi_i.e = 0;
    dcdc_cc.pi_i.e1 = 0;
    dcdc_duty = 0;
    dcdc_cc.pi_i.i = 0;
    dcdc_cc.pi_i.u = 0;
    dcdc_ref.wake_bus_pi.i = 0;
    dcdc_ref.wake_bus_pi.u = 0;
}

/* Sets up both PV boosts: precharge PI kp 0.01, ki 0.01, 0..800; bus PI (pv_bus_mode only)
   kp 0.12, ki 0.0217, 0..1500; MPPT voltage PI kp 0.08, ki 0.003, 0..1500; current PI kp 0.2, ki
   0.012, 0..2108 counts. The bus limiter kp 2.32, ki 0.08. The boosts are off. */
void pv_ctrl_init(void) {
    Uint16 i;

    memset(&pv_phase[0], 0, sizeof(pv_phase_t));
    memset(&pv_phase[1], 0, sizeof(pv_phase_t));
    for (i = 0; i < 2; i++) {
        pv_phase[i].pi_precharge.kp = 0.01f;
        pv_phase[i].pi_precharge.ki = 0.01f;
        pv_phase[i].pi_precharge.umax = 800.0f;
        pv_phase[i].pi_precharge.umin = 0;
        pv_phase[i].pi_bus.kp = 0.12f;
        pv_phase[i].pi_bus.ki = 0.0217f;
        pv_phase[i].pi_bus.umax = 1500.0f;
        pv_phase[i].pi_bus.umin = 0;
        pv_phase[i].pi_v.kp = 0.08f;
        pv_phase[i].pi_v.ki = 0.003f;
        pv_phase[i].pi_v.umax = 1500.0f;
        pv_phase[i].pi_v.umin = 0;
        pv_phase[i].pi_i.kp = 0.2f;
        pv_phase[i].pi_i.ki = 0.012f;
        pv_phase[i].pi_i.umax = 2108.0f;
        pv_phase[i].pi_i.umin = 0;
        pv_phase[i].i_ceiling = 0;
    }
    pv1_boost_on = 0;
    pv2_boost_on = 0;
    pv_phase[0].v_loop_out = 0;
    pv_phase[0].i_ref = 0;
    pv_phase[0].pi_i.e1 = 0;
    pv_phase[0].pi_i.i = 0;
    pv_phase[0].pi_i.u = 0;
    pv_phase[1].i_ref = 0;
    pv_phase[1].pi_i.e1 = 0;
    pv_phase[1].pi_i.i = 0;
    pv_phase[1].pi_i.u = 0;
    pv_phase[0].i_ceiling = 0;
    pv_phase[1].i_ceiling = 0;
    meas_avg.pv_ramp_tick = 0;
    pv_phase[0].i_limit = 0;
    pv_phase[1].i_limit = 0;
    pv_bus_pi.kp = 2.32f;
    pwm_period = 0x0927;
    pv_bus_pi.p = 0;
    pv_bus_pi.ki = 0.08f;
    pv_bus_pi.i = 0;
    pv_bus_pi.u = 0;
    pv_bus_pi.spare = 0;
}

/* Sets up the battery converter's PIs. The bounds come from dcdc_ref.i_limit (300 = 30.0 A) and
   dcdc_ref.i_cmd_max (task 2's hv_current_max, 15.0 A, read once here):
   bus sync PI kp 0.54, ki 0.003; EPS bus PI kp 0.84, ki 0.006; band PIs kp 0.45, ki 0.0025;
   battery voltage PIs kp 1.25, ki 0.007 (max side +-i_cmd_max, min side -i_cmd_max..0); current
   PI kp 0.3, ki 0.04, +-2343 counts; wake PIs kp 0.003 with ki 0.0008 (+-900) and ki 0.0004
   (0..700). */
void dcdc_ctrl_init(void) {
    dcdc_ref.i_limit = 300.0f;
    dcdc_cc.pi_bus_sync.kp = 0.54f;
    dcdc_cc.pi_bus_sync.ki = 0.003f;
    dcdc_cc.pi_bus_sync.umax = dcdc_ref.i_limit;
    dcdc_cc.pi_bus_sync.umin = -dcdc_ref.i_limit;
    dcdc_cc.pi_bus.kp = 0.84f;
    dcdc_cc.pi_bus.ki = 0.006f;
    dcdc_cc.pi_bus.umax = dcdc_ref.i_limit;
    dcdc_cc.pi_bus.umin = -dcdc_ref.i_limit;
    dcdc_cc.pi_bus_hi.kp = 0.45f;
    dcdc_cc.pi_bus_hi.ki = 0.0025f;
    dcdc_cc.pi_bus_hi.umax = 0;
    dcdc_cc.pi_bus_hi.umin = -dcdc_ref.i_limit;
    dcdc_cc.pi_bus_lo.kp = 0.45f;
    dcdc_cc.pi_bus_lo.ki = 0.0025f;
    dcdc_cc.pi_bus_lo.umax = dcdc_ref.i_limit;
    dcdc_cc.pi_bus_lo.umin = 0;
    dcdc_cc.pi_v_max.kp = 1.25f;
    dcdc_cc.pi_v_max.ki = 0.007f;
    dcdc_cc.pi_v_max.umax = dcdc_ref.i_cmd_max;
    dcdc_cc.pi_v_max.umin = -dcdc_ref.i_cmd_max;
    dcdc_cc.pi_v_min.kp = 1.25f;
    dcdc_cc.pi_v_min.ki = 0.007f;
    dcdc_cc.pi_v_min.umax = 0;
    dcdc_cc.pi_v_min.umin = -dcdc_ref.i_cmd_max;
    dcdc_cc.pi_i.kp = 0.3f;
    dcdc_cc.pi_i.ki = 0.04f;
    dcdc_cc.pi_i.umax = 2343.0f;
    dcdc_cc.pi_i.umin = -2343.0f;
    dcdc_ref.wake_batt_pi.kp = 0.003f;
    dcdc_ref.wake_batt_pi.ki = 0.0008f;
    dcdc_ref.wake_batt_pi.umax = 900.0f;
    dcdc_ref.wake_batt_pi.umin = -900.0f;
    dcdc_ref.wake_bus_pi.kp = 0.003f;
    dcdc_ref.wake_bus_pi.ki = 0.0004f;
    dcdc_ref.wake_bus_pi.umax = 700.0f;
    dcdc_ref.wake_bus_pi.umin = 0;
}

#pragma CODE_SECTION(pv1_ctrl, "ramfuncs");
/* PV1's boost (EPWM1A), every sample while it is on and CF12 bit 3 is clear. Once a cycle the
   soft current ceiling rises 10 (0.10 A) towards i_limit. On slot 0 the MPPT voltage PI acts on
   pv1_v - pv1_v_ref (a higher PV voltage asks for more current), its output held to at most 0.05 A
   above the soft ceiling and to 0..i_limit; with pv_bus_mode (never set) a bus PI would replace
   it. The current reference is capped by the bus limiter, and the current PI (ki 0.002 above
   3.00 A, 0.012 below 2.00 A) gives the compare, loaded at the end of control_isr. */
void pv1_ctrl(void) {
    if (pv1_boost_on == 1 && !(fault_words.fault_grid1 & 0x0008)) {
        if (meas_avg.pv_ramp_tick == 0) {
            pv_phase[0].i_ceiling += 10.0f;
            pv_phase[0].i_ceiling = __fmin(pv_phase[0].i_ceiling, pv_phase[0].i_limit);
        }
        if (grid_rep_tail.ctrl_slot == 0) {
            if (pv_bus_mode == 0) {
                pv_phase[0].pi_v.e = meas_avg.pv1_v - meas_avg.pv1_v_ref;
                pi_step(&pv_phase[0].pi_v);
                pv_phase[0].v_loop_out = pv_phase[0].pi_v.u;
                if (pv_phase[0].v_loop_out - pv_phase[0].i_ceiling > 5.0f) {
                    pv_phase[0].v_loop_out = pv_phase[0].i_ceiling + 5.0f;
                }
                {
                    float t_ = pv_phase[0].v_loop_out;
                    pv_phase[0].v_loop_out =
                        t_ > pv_phase[0].i_limit ? pv_phase[0].i_limit : __fmax(t_, 0.0f);
                }
            } else {
                pv_phase[0].pi_bus.e = grid_ictl_blk.bus_v_ref - meas_avg.bus_v_filt;
                {
                    float t_ = pv_phase[0].pi_bus.e;
                    pv_phase[0].pi_bus.e = (t_ >= 200.0f) ? 200.0f : __fmax(t_, -200.0f);
                }
                pi_step(&pv_phase[0].pi_bus);
                pv_phase[0].v_loop_out = pv_phase[0].pi_bus.u;
            }
            pv_phase[0].i_ref = pv_phase[0].v_loop_out;
        }
        pv_phase[0].i_ref = __fmin(pv_phase[0].i_ref, pv_bus_pi.u);
        if (meas_fast.pv1_i > 300.0f) {
            pv_phase[0].pi_i.ki = 0.002f;
        } else if (meas_fast.pv1_i < 200.0f) {
            pv_phase[0].pi_i.ki = 0.012f;
        }
        pv_phase[0].pi_i.e = pv_phase[0].i_ref - meas_fast.pv1_i;
        pi_step(&pv_phase[0].pi_i);
        pv_phase[0].duty = pv_phase[0].pi_i.u;
        pv_phase[0].cmp = (Uint16)pv_phase[0].duty;
        pv_phase[0].cmp_pending = 1;
    }
}

#pragma CODE_SECTION(pv2_ctrl, "ramfuncs");
/* PV2's boost (EPWM1B): pv1_ctrl's loop on slot 1. With the PV inputs in parallel (pv_input_mode
   4) PV2 takes PV1's current reference. */
void pv2_ctrl(void) {
    volatile pv_phase_t *ph = &pv_phase[1]; /* the image stores the ramp and reads it back */

    if (pv2_boost_on == 1 && !(fault_words.fault_grid1 & 0x0008)) {
        if (meas_avg.pv_ramp_tick == 0) {
            ph->i_ceiling += 10.0f;
            ph->i_ceiling = __fmin(ph->i_ceiling, pv_phase[1].i_limit);
        }
        if (grid_rep_tail.ctrl_slot == 1) {
            if (pv_bus_mode == 0) {
                pv_phase[1].pi_v.e = meas_avg.pv2_v - meas_avg.pv2_v_ref;
                pi_step(&pv_phase[1].pi_v);
                pv_phase[1].v_loop_out = pv_phase[1].pi_v.u;
                if (pv_phase[1].v_loop_out - pv_phase[1].i_ceiling > 5.0f) {
                    pv_phase[1].v_loop_out = pv_phase[1].i_ceiling + 5.0f;
                }
                if (pv_phase[1].v_loop_out > pv_phase[1].i_limit)
                    pv_phase[1].v_loop_out = pv_phase[1].i_limit;
                else
                    pv_phase[1].v_loop_out = __fmax(pv_phase[1].v_loop_out, 0.0f);
            } else {
                pv_phase[1].pi_bus.e = grid_ictl_blk.bus_v_ref - meas_avg.bus_v_filt;
                if (pv_phase[1].pi_bus.e >= 200.0f)
                    pv_phase[1].pi_bus.e = 200.0f;
                else
                    pv_phase[1].pi_bus.e = __fmax(pv_phase[1].pi_bus.e, -200.0f);
                pi_step(&pv_phase[1].pi_bus);
                pv_phase[1].v_loop_out = pv_phase[1].pi_bus.u;
            }
            pv_phase[1].i_ref = pv_phase[1].v_loop_out;
        }
        if (dsp_settings.pv_input_mode == 4) {
            pv_phase[1].i_ref = pv_phase[0].i_ref;
        }
        pv_phase[1].i_ref = __fmin(pv_phase[1].i_ref, pv_bus_pi.u);
        if (meas_fast.pv2_i > 300.0f) {
            pv_phase[1].pi_i.ki = 0.002f;
        } else if (meas_fast.pv2_i < 200.0f) {
            pv_phase[1].pi_i.ki = 0.012f;
        }
        pv_phase[1].pi_i.e = pv_phase[1].i_ref - meas_fast.pv2_i;
        pi_step(&pv_phase[1].pi_i);
        pv_phase[1].duty = pv_phase[1].pi_i.u;
        pv_phase[1].cmp = (Uint16)pv_phase[1].duty;
        pv_phase[1].cmp_pending = 1;
    }
}

#pragma CODE_SECTION(grid_angle_update, "ramfuncs");
/* The angle the EPS controller uses (eps_vctl_tail.theta): in the grid states it follows the PLL
   (angle, period, wrap, index), so a change to EPS starts in phase; in other states it free-runs
   at grid_omega_step (2 pi f / 19.2 kHz), setting cycle_wrap at each turn. */
void grid_angle_update(void) {
    if (op_state == OP_GRID_PV || op_state == OP_GRID_BATT) {
        eps_vctl_tail.theta = grid_pll.theta;
        eps_cycle.cycle_len = grid_pll.period;
        eps_cycle.cycle_wrap = grid_pll.wrap;
        eps_cycle.cycle_idx = grid_pll.n;
    } else {
        eps_cycle.cycle_idx++;
        if (eps_cycle.cycle_idx > eps_cycle.cycle_len)
            eps_cycle.cycle_idx = 0;
        eps_vctl_tail.theta = eps_vctl_tail.theta + grid_omega_step;
        if (eps_vctl_tail.theta < 0.0f) {
            eps_vctl_tail.theta = eps_vctl_tail.theta + 6.2831855f;
        } else if (eps_vctl_tail.theta > 6.2831855f) {
            eps_vctl_tail.theta = eps_vctl_tail.theta - 6.2831855f;
            eps_cycle.cycle_wrap = 1;
        }
    }
}

#pragma CODE_SECTION(grid_period_isr, "ramfuncs");
/* ECAP1 interrupt: the grid period from the capture counter. On a capture event it keeps the
   count, counts valid periods up to 100, hands the last period's RMS sums to task 1 (ac_sums),
   clamps the period to 2250000 counts (40 Hz at 90 MHz), toggles GPIO10 and posts task 1's
   period event. Then it clears the flags and re-arms the capture. */
interrupt void grid_period_isr(void) {
    Uint32 cap = ECap1Regs.CAP1;

    ecap_delta = cap - ecap_last;
    if (ECap1Regs.ECFLG.bit.CEVT1) {
        ecap_last = cap;
        if (ecap_valid_count < 100)
            ecap_valid_count++;
        ac_sums.grid_v_sq = ac_sums_isr.grid_v_sq;
        ac_sums.leak_i_sq = ac_sums_isr.leak_i_sq;
        ac_sums.n_grid = ac_sums_isr.n_grid;
        ac_sums.inv_v_sq = ac_sums_isr.inv_v_sq;
        ac_sums.grid_i_sq = ac_sums_isr.grid_i_sq;
        ac_sums.dc_sum = ac_sums_isr.dc_sum;
        ac_sums.grid_p = ac_sums_isr.grid_p;
        ac_sums_isr.grid_v_sq = 0;
        ac_sums_isr.leak_i_sq = 0;
        ac_sums_isr.inv_v_sq = 0;
        ac_sums_isr.grid_i_sq = 0;
        ac_sums_isr.dc_sum = 0;
        ac_sums_isr.grid_p = 0;
        ac_sums_isr.n_grid = 0;
        ecap_period = ecap_delta > 2250000 ? 2250000 : ecap_delta;
        GpioDataRegs.GPATOGGLE.bit.GPIO10 = 1;
        os_post_isr(TASK_AC_MEAS, EV1_GRID_PERIOD);
    }
    ECap1Regs.ECCLR.bit.CTROVF = 1;
    ECap1Regs.ECCLR.bit.INT = 1;
    ECap1Regs.ECCLR.bit.CEVT1 = 1;
    ECap1Regs.ECCTL2.bit.REARM = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;
}

#pragma CODE_SECTION(grid_ictl_clear, "ramfuncs");
/* Clears the grid current controller's outputs and states and the reactive loop, and one word of
   the 430-word repetitive ring per call (the whole ring in 430 samples, about 22 ms). */
void grid_ictl_clear(void) {
    grid_ictl_blk.pi_e = 0;
    grid_ictl_blk.pi_i = 0;
    grid_ictl_blk.i_ref_q = 0;
    grid_ictl_blk.i_ref_p = 0;
    grid_ictl_blk.pi_u = 0;
    q_loop.ref_sum = 0;
    grid_ictl_blk.amp_cmd = 0;
    q_loop.meas_sum = 0;
    q_loop.e = 0;
    grid_ictl_blk.i_ref = 0;
    grid_ictl_blk.i_err = 0;
    grid_rep.rep_in = 0;
    grid_rep.rep_y = 0;
    q_loop.u = 0;
    grid_rep.rep_out = 0;
    grid_ictl_blk.v_cmd = 0;
    if (++grid_buf_clear_idx >= 430)
        grid_buf_clear_idx = 0;
    grid_rep_buf[grid_buf_clear_idx] = 0;
}

/* The grid-side init, called by the state machine on entry to the idle, fault and update states:
 * - grid current controller (0xC400): current gain 4.5; bus PI kp 0.6, ki 0.008, limits
 *   +-1.1 x sqrt 2 x grid_current_max (the rated RMS current in 0.1 A); repetitive filter
 *   a 0.8, b 0.923, gain 1.0, +-700, lead 5 samples;
 * - PLL (0xC100): filter gain 0.0098, lead 2.8 degrees, PI kp 7e-5, ki 5e-8, angle step
 *   0.00818..0.02618 rad per sample (25..80 Hz at 19.2 kHz);
 * - islanding detector (0xC04E): +-15, limit 60 cycles, threshold 0.97, gain 50;
 * - reactive loop (0xC180): kp 0.00061, ki 0.00049, +-51;
 * the bridge duty and compares cleared, and the 430-sample ring at 0xC452 zeroed.
 * match: each object's stores are written in the image's order, one object after another (the
 * scheduler interleaves them). */
void grid_ctrl_init(void) {
    Uint16 i;
    float v = grid_current_max;

    grid_ictl_blk.kp_i = 4.5f;
    grid_ictl_blk.pi_e = 0;
    grid_ictl_blk.pi_kp = 0.600000024f;
    grid_ictl_blk.pi_ki = 0.00800000038f;
    grid_ictl_blk.pi_kd = 0;
    grid_ictl_blk.pi_i = 0;
    grid_ictl_blk.pi_u = 0;
    grid_ictl_blk.pi_umax = v * 1.41421354f * 1.1f;
    grid_ictl_blk.rep_lead = 5;
    grid_ictl_blk.pi_umin = -v * 1.41421354f * 1.1f;
    grid_ictl_blk.amp_cmd = 0;
    grid_ictl_blk.i_err = 0;
    grid_ictl_blk.v_cmd = 0;
    grid_ictl_blk.amp_ramp = 0;
    grid_rep.rep_in = 0;
    grid_ictl_blk.q_cmd = 0;
    grid_rep.rep_b = 0.922850013f;
    grid_ictl_blk.i_ref_q = 0;
    grid_rep.rep_gain = 1.0f;
    grid_ictl_blk.i_ref_p = 0;
    grid_rep.rep_a = 0.800000012f;
    grid_ictl_blk.i_ref = 0;
    grid_rep.rep_max = 700.0f;
    grid_rep.rep_min = -700.0f;

    grid_pll.in = 0;
    grid_pll.k = 0.00981250033f;
    grid_pll.shift = 2.79999995f;
    grid_pll.pi_e = 0;
    grid_pll.pi_kp = 7.00000019e-05f;
    grid_pll.pi_ki = 5.00000006e-08f;
    grid_pll.pi_kd = 0;
    grid_pll.pi_umax = 0.0261799395f;
    grid_pll.pi_umin = 0.00818123017f;
    grid_pll.pi_e1 = 0;
    grid_pll.pi_i = 0;
    grid_pll.pi_u = 0;

    island_det.max = 15.0f;
    island_det.min = -15.0f;
    island_det.limit = 60;
    island_det.thr = 0.970000029f;
    island_det.gain = 50.0f;

    q_loop.kp = 0.000610349991f;
    q_loop.ki = 0.000488299993f;
    q_loop.max = 51.0f;
    q_loop.min = -51.0f;

    inv_duty = 0;
    inv_cmp_b = 0;
    inv_cmp_a = 0;
    for (i = 0; i < 430; i++)
        grid_rep_buf[i] = 0;
}
