// clang-format off
/* RTOS task 2 (every 20 ms): the control task and its steps: the charge and discharge power
   limits, the battery current tapers, the SoC floor, the battery voltage monitor.
   at: task2_control 0x3EC8BF; aging_test_update 0x3EC8D7; soft_start_permit 0x3EC908;
       batt_power_blocks 0x3EC92F; hv_current_max_set 0x3ED314; soc_power_limits 0x3EC96D;
       status_led_and_reset 0x3ED31B; batt_voltage_monitor 0x3ED155; full_current_trim 0x3ED0E0;
       batt_power_setpoint 0x3ECDE3; ctl_voltage_limits_set 0x3ED2F4; soc_floor 0x3ECFB1
   Any change to this file can move its variables: rerun
       python3 tools/ebss_solve.py src/app/power/power_limits.c

   Job: turn the battery's reports (the BMS page 0xD500, bms_link.c) and the ARM's settings into
   the battery converter's command and bounds. Each 20 ms pass it decides the battery power command
   (batt_power_setpoint: self-consumption, forced charge or forced discharge), the blocks on it
   (full, force charge, no discharge), the current limits and power limits, and from them the two
   current bounds the converter's interrupt code clamps to (isr_limits 0xC212/0xC214,
   current_ctrl.c).
   It also sets the battery fault bits CF15 1, 2, 3, 4, 6, 8, 15 and the blocks CF19 8, 9, 10, 11.
   Entry point: task2_control (created by main). has_batt_fault and friends are in op_states.c.
   Data it owns: the counters and filters 0xD4C0 .. 0xD4FB below. It writes the float half of the
   BMS page (0xD50E .. 0xD52E) and isr_limits (0xC208 .. 0xC216).
   Neighbours: bms_link.c (the page), arm_link.c/arm_page_write.c (settings), dc_measure.c
   (dc_meas), grid_support.c (lim_soft_start, freq_resp), op_states.c (soft_start reads
   start_permit and soft_start_enable), converter_ctrl.c/current_ctrl.c (the bounds and the trim).
   Units: voltages 0.1 V (dc_meas, HR97/HR98 copies), powers 0.1 W, BMS currents and caps 0.01 A,
   the ISR bounds and command 0.1 A on the HV side of the DC-DC, SoC in %, one pass = 20 ms.
   Positive power and current = charge. docs/battery.md has every limit with its formula.
   name: descriptive (map/filenames.tsv: src/control_b.c, RTOS task 2 and its steps). */
// clang-format on
/* match: this file reaches the BMS page as union bms_page_u (named words and the first three
 * as an array), not as the volatile struct the other files see; with the struct view the
 * accesses would all be volatile and the code would differ. */
#define OWN_bms
#define OWN_batt_current_cmd_i /* match: signed here (the code compares and converts it as int); converter_ctrl.c defines it Uint16 and the header has that, which changes batt_power_setpoint */
#define OWN_chg_current_bound_i /* match: signed here; with the header's Uint16 current_limit_tapers compiles differently */
#define OWN_dis_current_bound_i /* match: signed here; with the header's Uint16 current_limit_tapers compiles differently */
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "meas.h"
#include "control.h"
#include "grid.h"
#include "state.h"
#include "rating.h"
#include "F2806x_Gpio.h"
#include <math.h>
#include "power_limits.h"
/* match: this file reaches the fault words both as an array and by name (one object) */
extern union fault_words_u {
    union bits16_u w[8];
    struct {
        union bits16_u fault_grid1, fault_grid2, fault_grid_eps, fault_batt, fault_pv, fault_stop1,
            fault_stop2, batt_blocks;
    } s;
} fault_words;
extern union bms_page_u bms; /* match: see OWN_bms above */

extern int batt_current_cmd_i;

/* 0xD212 and 0xD528 under names of their own (see current_limit_tapers) */

extern int dis_current_bound_i, chg_current_bound_i;
/* name: the 0xCE00 page is grid_support.c's storage frequency/voltage response block, not a remote command */

/* this file's variables, 0xD4C0..0xD4FB. The compiler does not keep their order; this order gives
 * the image's layout (tools/ebss_solve.py src/app/power/power_limits.c) */
// clang-format off
/* .ebss begin */
unsigned int uv_clear_limit = 150;  /* passes over HR97 + 2.0 V that clear CF15 bit 2 */
unsigned int full_trim_div = 0;
unsigned int uv_clear_count = 0;
unsigned int ov_clear_count = 0;
unsigned int vmis_clear_count = 0;
unsigned int no_dis_count = 0;
unsigned int vmis_set_count = 0;  /* CF15 bit 1 set count */
union bits16_u soc_floor_latch;  /* bit 3: the EPS latch (soc_floor) */
unsigned int ov_slow_count = 0;  /* over max + 1.0 V, 50 trips */
unsigned int start_hold_count = 0;  /* soft_start_permit, 6000 = 120 s */
unsigned int force_chg_count = 0;  /* CF19 bit 9 set count */
unsigned int aging_delay = 0;
unsigned int upgrade_end_count = 0;
unsigned int start_permit_count = 0;
unsigned int led_count = 0;
unsigned int cf18b2_reset_count = 0;  /* ARM link lost: reset after 30000 */
unsigned int full_clear_count = 0;
unsigned int dcdc_ratio_clear_count = 0;
unsigned int cf15b3_clear_count = 0;
unsigned int dcdc_ratio_count = 0;
unsigned int full_set_count = 0;
unsigned int force_chg_soc = 0;  /* the force-charge SoC F (floor - 3, or 1) */
unsigned int ov_fast_count = 0;  /* over max + 2.0 V, 2 trips */
unsigned int floor_latch_set_count = 0;
unsigned int floor_latch_clear_count = 0;
unsigned int cfg_reset_count = 0;
unsigned int full_dis_count = 0;  /* full_current_trim: readings at -5 A or less */
unsigned int uv_set_count = 0;
float pv_pi_max = 0;
float pv_current_max = 0;
float hr19b5_chg_ramp = 0;
float chg_lowv_taper = 0;  /* f: the charge-limit taper under 48 V */
float full_trim_out = 0;
float full_trim_integ = 0;
float chg_plim_pv = 0;  /* charge power limit from the PV PIs (0.1 W) */
float chg_plim_pv_prev = 0;
float arm_setpoint_prev = 0;
float force_chg_cap = 0;  /* forced charge cap, capacity x HR111 % x V (0.1 W) */
float bus_ov_derate = 0;  /* filtered bus voltage over 520.0 V (0.1 V, 0..50 V) */
float force_dis_cap = 0;  /* forced discharge cap, -capacity x HR112 % x V (0.1 W) */
float batt_current_cmd = 0;  /* current command, 0.1 A HV side */
float batt_power_cmd = 0;  /* 0xD4FA: power command, 0.1 W, positive = charge */
float pack_current_filt = 0;
float dis_highv_taper = 0;  /* g: the discharge-limit taper over 54.5 V */
/* .ebss end */
// clang-format on

/* RTOS task 2: on each 20 ms tick run the steps in the image's order. */
void task2_control(void) {
    for (;;) {
        if (os_wait(0) & EV_TICK_M) {
            status_led_and_reset();
            hv_current_max_set();
            ctl_voltage_limits_set();
            soc_floor();
            batt_voltage_monitor();
            full_current_trim();
            batt_power_setpoint();
            soft_start_permit();
            aging_test_update();
        }
    }
}

/* HR332 (aging_test_enable, ARM page 2): 101 passes (2 s) after it is set, aging mode 1 if both
   PV inputs read 160 V or more, else mode 2; 0 when it is cleared. Mode 1 forces charging with the
   PV references at 250 V and the ISR battery maximum at 50.0 V; mode 2 forces discharging with
   the ISR battery limits 46.0 .. 54.0 V (ctl_voltage_limits_set, batt_power_setpoint, mppt.c). */
void aging_test_update(void) {
    unsigned int s = 0;
    if (aging_test_enable == 1) {
        if (++aging_delay > 100) {
            aging_delay = 100;
            if (dc_meas.pv1_v_raw >= 1600.0f && dc_meas.pv2_v_raw >= 1600.0f)
                s = 1;
            else
                s = 2;
        }
    } else {
        s = 0;
        aging_delay = 0;
    }
    DINT;
    aging_test_mode = s;
    EINT;
}

/* soft_start_enable (0xD52E), which the run states need before they call soft_start: set when
   there is no battery fault and the HV side reads 220.0 V or more, or after 6000 passes (120 s)
   without a fault; cleared at once by any battery fault (has_batt_fault). */
void soft_start_permit(void) {
    if (has_batt_fault() == 0) {
        if (dc_meas.hv_v >= 2200.0f || ++start_hold_count >= 6000) {
            soft_start_enable = 1;
            start_hold_count = 0;
        }
    } else {
        soft_start_enable = 0;
        start_hold_count = 0;
    }
}

/* Apply the CF19 blocks to the power command p (0.1 W, positive = charge), first match wins:
   - force charge (CF19 bit 9), or BMS mode with HR19 bit 2 clear outside a calibration and aging
     test: a command of 300 W or less becomes +300 W;
   - no discharge (CF19 bit 11): a negative command becomes 0;
   - full (CF19 bit 8): the command is at most 0.
   Then HR19 bit 5, outside a calibration, makes it at most -120 W (some discharge). */
float batt_power_blocks(float p) {
    unsigned int low;
    if ((batt_blocks.bit.b9 ||
         (battery_type != 0 && !bms_hr19.bit.b2 && (aging_test_enable | batt_cal_stage) == 0)) &&
        p <= 3000.0f)
        p = 3000.0f;
    else if (batt_blocks.bit.b11 && p < 0.0f)
        p = 0.0f;
    else if (batt_blocks.bit.b8)
        p = __fmin(p, 0.0f);
    if (bms_hr19.bit.b5 && batt_cal_stage == 0)
        p = __fmin(p, -1200.0f);
    return p;
}

/* The current command limit on the HV side (dcdc_ref.i_cmd_max 0xC20E): 15.0 A, every pass. */
void hv_current_max_set(void) { isr_limits.hv_current_max = 150.0f; }

/* Power limits by SoC, called by current_limit_tapers while the BMS reports (HR19 bits 0-1 not
   both 0, HR22 not 0). Charge: rated x (1 - 0.095 x (SoC - 90)), SoC - 90 held to 0..8, so 100 %
   up to 90 % SoC and 24 % from 98 %; in a calibration 0.075 per % over 0..10, 25 % at 100 %.
   HR20 bit 2 (0xD500 bit 1) makes the charge limit 0 (5 % of rated in a calibration). HR20 bit 3
   (0xD500 bit 2) makes the discharge limit 10 % of the discharge rating, else it is the full
   rating. b (15 - SoC, 0..5) is computed and never used: a discharge taper below 15 % that was
   dropped. */
void soc_power_limits(void) {
    float a = 0.0f;
    float b = a;
    if (bms.s.soc > 90)
        a = bms.s.soc - 90;
    if (batt_cal_stage == 0) {
        limitf(&a, 8.0f, 0.0f);
        bms.s.chg_plim_soc = batt_power_rating.chg_max * (1.0f - 0.095f * a);
    } else {
        limitf(&a, 10.0f, 0.0f);
        bms.s.chg_plim_soc = batt_power_rating.chg_max * (1.0f - 0.075f * a);
    }
    if (bms.s.alarm_flags.bit.b1)
        bms.s.chg_plim_soc = (batt_cal_stage != 0) ? 0.05f * batt_power_rating.chg_max : 0.0f;
    if (!bms.s.alarm_flags.bit.b2) {
        if (bms.s.soc <= 15)
            b = 15 - bms.s.soc;
        limitf(&b, 5.0f, 0.0f);
        bms.s.dis_plim_soc = batt_power_rating.dis_max;
    } else {
        bms.s.dis_plim_soc = 0.1f * batt_power_rating.dis_max;
    }
}

/* The status LED (GPIO22 toggles every 50 passes without a BMS, every 15 with one) and four
   resets through GPIO27 (an external reset, as in the boot loader): a settings change pending
   (cfg_changed_flags) for 751 passes (15 s), the update tag "END" for 251 passes (5 s), the ARM
   link lost (CF18 bit 2) for 30001 passes (10 min), BMS pass-through for 15001 passes (5 min). */
void status_led_and_reset(void) {
    if (battery_type == 0) {
        if (++led_count >= 50) {
            led_count = 0;
            GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
        }
    } else if (++led_count >= 15) {
        led_count = 0;
        GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;
    }
    if (cfg_changed_flags != 0) {
        if (++cfg_reset_count > 750) {
            cfg_reset_count = 0;
            GpioDataRegs.GPASET.bit.GPIO27 = 1;
        }
    } else {
        cfg_reset_count = 0;
    }
    if (upgrade_tag[0] == 'E' && upgrade_tag[1] == 'N' && upgrade_tag[2] == 'D') {
        if (upgrade_end_count < 250) {
            upgrade_end_count++;
        } else {
            upgrade_end_count = 0;
            GpioDataRegs.GPASET.bit.GPIO27 = 1;
        }
    } else {
        upgrade_end_count = 0;
    }
    if (fault_stop2.bit.b2) {
        if (cf18b2_reset_count < 30000) {
            cf18b2_reset_count++;
        } else {
            cf18b2_reset_count = 0;
            GpioDataRegs.GPASET.bit.GPIO27 = 1;
        }
    } else {
        cf18b2_reset_count = 0;
    }
    if (bms_relay_active == 1) {
        if (++bms_relay_idle > 15000) {
            GpioDataRegs.GPASET.bit.GPIO27 = 1;
            bms_relay_idle = 0;
        }
    } else {
        bms_relay_idle = 0;
    }
}

/* The battery voltage checks on dc_meas.batt_v (the DSP's own reading). Counts are passes of 20 ms.
   - Without a BMS: CF15 bit 1 set under 20.0 V for 5, cleared over 30.0 V for 5; start_permit 1.
   - With a BMS: start_permit (0xD52D) after 500 passes (10 s) with HR19 bits 0-1 not both 0 and
     HR22 above HR97 - 2.0 V, cleared at once otherwise. CF15 bit 1 (mismatch) at once under 24.0 V,
     or after 5 checks with |V - HR22| over 5.0 V while the BMS reports, HR22 is not 0 and |HR23| is
     under 2.00 A; cleared after 500 checks within 3.0 V.
   - CF15 bit 2 (under-voltage): under HR97 for 5 (not while the BMS is idle), cleared after 150
     above HR97 + 2.0 V (500 with low_batt_wake); held set while fewer than 50 BMS replies have come
     in and low_batt_wake is 1.
   - CF15 bit 6 (over-voltage): over max + 2.0 V for 2 or over max + 1.0 V for 50, cleared after 500
     under max; max is HR98 in BMS mode, else 56.0 V.
     The set and clear counts of bits 2 and 6 are totals: a counter is reset only when it fires.
   - CF15 bit 4 (DC-DC ratio): in converter mode 5 only, the HV side 50.0 V or more away from
     5.5 x V for 50 passes in a row; it clears itself 500 passes after it was set.
   - CF15 bit 3 (calibration end, set by bms_link.c) clears itself after 501 passes. */
void batt_voltage_monitor(void) {
    unsigned int t;
    float v;
    unsigned int m;
    if (dsp_settings.battery_type == 0) {
        if (dc_meas.batt_v < 200.0f) {
            vmis_set_count++;
            vmis_clear_count = 0;
            if (vmis_set_count >= 5) {
                fault_batt.bit.b1 = 1;
                vmis_set_count = 0;
            }
        } else if (dc_meas.batt_v > 300.0f) {
            vmis_clear_count++;
            vmis_set_count = 0;
            if (vmis_clear_count >= 5) {
                fault_batt.bit.b1 = 0;
                vmis_clear_count = 0;
            }
        }
        bms.s.start_permit = 1;
    } else {
        if ((bms.s.hr19_status.all & 3) != 0 &&
            bms.s.pack_voltage > dsp_settings.batt_min_voltage - 20) {
            if (++start_permit_count >= 500)
                bms.s.start_permit = 1;
        } else {
            bms.s.start_permit = 0;
            start_permit_count = 0;
        }
        v = dc_meas.batt_v;
        if (v < 240.0f) {
            fault_batt.bit.b1 = 1;
        } else {
            if (fabs(v - bms.s.pack_voltage) > 50.0f && (bms.s.hr19_status.all & 3) != 0 &&
                bms.s.pack_voltage != 0 && fabs((float)bms.s.pack_current) < 200.0f) {
                vmis_set_count++;
                vmis_clear_count = 0;
                if (vmis_set_count >= 5) {
                    fault_batt.bit.b1 = 1;
                    vmis_set_count = 0;
                }
            } else if (fabs(v - bms.s.pack_voltage) < 30.0f) {
                vmis_clear_count++;
                vmis_set_count = 0;
                if (vmis_clear_count >= 500) {
                    fault_batt.bit.b1 = 0;
                    vmis_clear_count = 0;
                }
            }
        }
    }
    m = dsp_settings.batt_min_voltage;
    v = dc_meas.batt_v;
    if (v < m) {
        if (dsp_settings.battery_type != 0 && (bms.s.hr19_status.all & 3) == 0) {
            uv_set_count = 0;
        } else if (++uv_set_count >= 5) {
            uv_set_count = 0;
            fault_batt.bit.b2 = 1;
        }
    } else if (v > m + 20) {
        uv_clear_limit = (low_batt_wake == 1) ? 500 : 150;
        if (++uv_clear_count >= uv_clear_limit) {
            uv_clear_count = 0;
            fault_batt.bit.b2 = 0;
        }
    }
    if (bms_reply_count < 50 && low_batt_wake == 1) {
        uv_clear_count = 0;
        fault_batt.bit.b2 = 1;
    }
    {
        float max =
            (dsp_settings.battery_type != 0) ? (float)dsp_settings.batt_max_voltage : 560.0f;
        if (v > max + 20.0f) {
            if (++ov_fast_count >= 2) {
                fault_batt.bit.b6 = 1;
                ov_fast_count = 0;
            }
        } else if (v > max + 10.0f) {
            if (++ov_slow_count >= 50) {
                ov_slow_count = 0;
                fault_batt.bit.b6 = 1;
            }
        } else if (v < max) {
            if (++ov_clear_count >= 500) {
                ov_clear_count = 0;
                fault_batt.bit.b6 = 0;
            }
        }
    }
    if (!fault_batt.bit.b4) {
        if (conv_mode == 5) {
            float p = 5.5f * dc_meas.batt_v;
            if (dc_meas.hv_v <= p - 500.0f || dc_meas.hv_v >= p + 500.0f) {
                if (++dcdc_ratio_count >= 50) {
                    fault_batt.bit.b4 = 1;
                    dcdc_ratio_count = 0;
                }
            } else {
                dcdc_ratio_count = 0;
            }
        }
    } else if (++dcdc_ratio_clear_count >= 500) {
        fault_batt.bit.b4 = 0;
        dcdc_ratio_clear_count = 0;
    }
    if (fault_batt.bit.b3) {
        if (++cf15b3_clear_count > 500) {
            fault_batt.bit.b3 = 0;
            cf15b3_clear_count = 0;
        }
    } else {
        cf15b3_clear_count = 0;
    }
}

/* While the battery is full (CF19 bit 8) in BMS mode: every 11th pass, filter HR23 (0.6/0.4) and
   build a PI trim, 0.001 x I + the integral of 0.002 x I (held to +-20), sent to the current
   controller as full_trim_isr (0xC206, which current_ctrl.c subtracts from its reference). Units:
   HR23 0.01 A in, 0.1 A HV side out, the output held to 0..2.0 A. 251 readings of -5.00 A or
   less (in total) reset it; outside the full state it is 0. */
void full_current_trim(void) {
    float x;
    if (battery_type != 0 && batt_blocks.bit.b8) {
        if (++full_trim_div > 10) {
            full_trim_div = 0;
            if (bms.s.pack_current <= -500) {
                if (++full_dis_count > 250)
                    goto off;
            } else {
                pack_current_filt = 0.6f * pack_current_filt + 0.4f * bms.s.pack_current;
                full_trim_integ = 0.002f * pack_current_filt + full_trim_integ;
                if (full_trim_integ > 20.0f)
                    full_trim_integ = 20.0f;
                else if (full_trim_integ < -20.0f)
                    full_trim_integ = -20.0f;
                full_trim_out = 0.001f * pack_current_filt + full_trim_integ;
            }
        }
    } else {
    off:
        full_dis_count = 0;
        full_trim_out = 0.0f;
        full_trim_integ = 0.0f;
    }
    limitf(&full_trim_out, 20.0f, 0.0f);
    DINT;
    full_trim_isr = full_trim_out;
    EINT;
}

/* The battery power command batt_power_cmd (0.1 W, positive = charge) and the current command.
   The mode is dsp_settings.batt_force_mode (0xD8CE), which the ARM writes from frame byte 4:
   batt_cal_request (HR175) or aging mode 1 force 1, aging mode 2 forces 2, and mode 1 drops to 0
   when HR20 enable_charge_target is set, PV is faulted and SoC is over HR116 charge_target_soc.
   Only in converter mode 5 with op_state 5, or 6 with 6, and no battery fault:
   - mode 0: when the ARM setpoint changes, -battery power - 0.5 x setpoint; 0 while the inverter's
     battery_power_mode (0xD8D3) is 0 or CF19 bit 2 is set; held to +-HR308.
   - mode 1: HR308 x HR313 %, at most capacity x HR111 % x V (a C-rate cap); in BMS mode at
     charge_target_soc_now it stops (0), unless HR15 bit 0, or 1000 W in a calibration.
   - mode 2: -(discharge rating x HR314 %), at most capacity x HR112 % x V; in BMS mode at SoC at or
     under HR110 it stops (0), or -1000 W under 4 % in a calibration (unreachable: the parser holds
     SoC above HR110 then).
   Other states and converter modes give 0. Then: a grid frequency or voltage response replaces it
   while running; until the soft-start ramp is done (run_status bit 4) its size is held to
   lim_soft_start; batt_power_blocks; current = 10 x P / V_hv (V_hv held to 200..560 V), held to
   +-15.0 A, to the ISR; and current_limit_tapers. Converter modes under 5: all zero. */
void batt_power_setpoint(void) {
    float v = 0.0f;
    if (batt_cal_request == 1 || dsp_settings.aging_test_mode == 1) {
        dsp_settings.batt_force_mode = 1;
    } else if (dsp_settings.aging_test_mode == 2) {
        dsp_settings.batt_force_mode = 2;
    } else if (dsp_settings.batt_force_mode == 1 && enable_charge_target == 1 &&
               has_pv_fault() == 1 && bms.s.soc > charge_target_soc) {
        dsp_settings.batt_force_mode = 0;
    }
    if (conv_mode >= 5) {
        if (((conv_mode == 5 && op_state == OP_GRID_PV) ||
             (conv_mode == 6 && op_state == OP_GRID_BATT)) &&
            has_batt_fault() == 0) {
            if (dsp_settings.batt_force_mode == 0) {
                if (arm_setpoint_prev != rating_params.arm_batt_setpoint)
                    batt_power_cmd = -dc_meas.batt_power - 0.5f * rating_params.arm_batt_setpoint;
                arm_setpoint_prev = rating_params.arm_batt_setpoint;
                if (dsp_settings.batt_power_mode == 0 || batt_blocks.bit.b2)
                    batt_power_cmd = 0.0f;
                if (batt_power_cmd > rating_params.batt_chg_power_max)
                    batt_power_cmd = rating_params.batt_chg_power_max;
                if (batt_power_cmd < -rating_params.batt_chg_power_max)
                    batt_power_cmd = -rating_params.batt_chg_power_max;
            } else if (dsp_settings.batt_force_mode == 1) {
                batt_power_cmd =
                    0.01f * (rating_params.batt_chg_power_max * rating_params.force_charge_pct);
                force_chg_cap =
                    0.01f *
                    (((float)dsp_settings.batt_capacity_ah * dsp_settings.batt_charge_limit_pct) *
                     dc_meas.batt_v);
                batt_power_cmd = __fmin(batt_power_cmd, force_chg_cap);
                if (dsp_settings.battery_type != 0 &&
                    bms.s.soc >= dsp_settings.charge_target_soc_now) {
                    if (batt_cal_stage != 0)
                        batt_power_cmd = 10000.0f;
                    else if (!(bms_hr15 & 1))
                        batt_power_cmd = 0.0f;
                }
            } else {
                batt_power_cmd = -(
                    0.01f * (rating_params.batt_dis_power_max * rating_params.force_discharge_pct));
                force_dis_cap = -(0.01f * (((float)dsp_settings.batt_capacity_ah *
                                            dsp_settings.batt_discharge_limit_pct) *
                                           dc_meas.batt_v));
                batt_power_cmd = __fmax(batt_power_cmd, force_dis_cap);
                if (dsp_settings.battery_type != 0 && bms.s.soc <= dsp_settings.discharge_min_soc) {
                    if (batt_cal_stage == 0)
                        batt_power_cmd = 0.0f;
                    else if (bms.s.soc < 4)
                        batt_power_cmd = -10000.0f;
                }
            }
        } else {
            batt_power_cmd = 0.0f;
            arm_setpoint_prev = rating_params.arm_batt_setpoint;
        }
        freq_resp.resp_restore_power = -batt_power_cmd;
        if ((freq_resp.uf_active == 1 || freq_resp.ov_resp_active == 1 ||
             freq_resp.of_active == 1 || freq_resp.uv_resp_active == 1) &&
            run_status_bits.bit.b4)
            batt_power_cmd = -freq_resp.grid_resp_power;
        if (!run_status_bits.bit.b4) {
            float m = lim_soft_start;
            if (m < fabs(batt_power_cmd)) {
                if (batt_power_cmd <= 0.0f)
                    m = -m;
                batt_power_cmd = m;
            }
        }
        v = dc_meas.hv_v;
        limitf(&v, 5600.0f, 2000.0f);
        batt_power_cmd = batt_power_blocks(batt_power_cmd);
        batt_current_cmd = 10.0f * batt_power_cmd / v;
        limitf(&batt_current_cmd, isr_limits.hv_current_max, -isr_limits.hv_current_max);
        batt_current_cmd_isr = batt_current_cmd;
        batt_current_cmd_i = 0.1f * batt_current_cmd;
        current_limit_tapers(v);
    } else {
        batt_power_cmd = 0.0f;
        batt_current_cmd = 0.0f;
        batt_current_cmd_isr = 0.0f;
    }
}

/* The ISR battery voltage limits (0xC208/0xC20A, 0.1 V): 56.0 V max and 46.0 V min (50.0 V max in
   aging mode 1, 54.0 V max in aging mode 2). current_ctrl.c regulates to them only without a BMS
   (battery_type 0); with a BMS the DSP has no battery voltage loop. */
void ctl_voltage_limits_set(void) {
    if (aging_test_mode == 2) {
        isr_limits.batt_v_max = 540.0f;
        isr_limits.batt_v_min = 460.0f;
    } else {
        float x;
        if (aging_test_mode == 1)
            x = 500.0f;
        else
            x = 560.0f;
        isr_limits.batt_v_max = x;
        isr_limits.batt_v_min = 460.0f;
    }
}

/* The SoC floor and the full flag, from the BMS page, every pass. In BMS mode while the BMS
 * reports (HR19 bits 0-1 not both 0 and HR22 not 0):
 * - full, CF19 bit 8: after 1500 passes (30 s) with HR26 = 0, SoC >= 100, 0xD502 bit 0 or
 *   HR20 bit 2, and no comms fault; cleared after 251 passes at SoC under 99; HR15 bit 0
 *   clears it every pass outside a calibration.
 * - the floor L is HR114 (soc_reserve), raised to HR110 (discharge_min_soc) in run modes 5 and 6.
 *   F = L - 3, or 1 when L <= 4. Force charge, CF19 bit 9: SoC <= F for 250 passes (in total),
 *   cleared at SoC >= F + 4. No discharge, CF19 bit 11: SoC <= F + 3 for 250, cleared at F + 7.
 * - CF15 bit 8 is the OR of 0xD500 bits 0, 3, 5, 8, 12 and CF19 bit 10 is 0xD502 bit 11: nothing
 *   in the image writes those bits, so both stay 0.
 * - in a calibration CF19 bits 8, 9 and 11 are cleared every pass.
 * When the BMS does not report: bits 8, 9, 10, 11 of CF19 and CF15 bit 8 are cleared. Without a
 * BMS: only CF19 bits 8 and 9. In every mode: CF15 bit 15 (a battery fault, so the converter stops)
 * = schedule window 3 or HR19 bit 4; and the EPS latch soc_floor_latch bit 3 is set after 51 passes
 * (in total) with bit 9 or 11, released 30001 passes (10 min) after both clear, at once on a grid
 * fault.
 * match: CF15.b8 and CF19.b10 read the BMS words through the struct view
 * (s.vD500, s.vD502), not the volatile array view w[]: with w[] the compiler keeps the source
 * order of the five reads, with s.vD500 it places them as the image has them. */
void soc_floor(void) {
    unsigned int f;
    float x;
    if (dsp_settings.battery_type != 0) {
        if ((bms.w[1].all & 3) != 0 && bms.s.pack_voltage != 0) {
            bms.s.charge_voltage_copy = bms.s.charge_voltage;
            bms.s.bms_charge_limit_copy = bms.s.bms_charge_limit;
            if ((bms.s.bms_charge_limit_copy == 0 || bms.s.soc >= 100 || bms.w[2].bit.b0 ||
                 bms.s.alarm_flags.bit.b1) &&
                !fault_words.s.fault_batt.bit.b9) {
                full_set_count++;
                full_clear_count = 0;
                if (full_set_count >= 1500) {
                    full_set_count = 0;
                    fault_words.s.batt_blocks.bit.b8 = 1;
                }
            } else if (bms.s.soc < 99) {
                full_clear_count++;
                full_set_count = 0;
                if (full_clear_count > 250) {
                    full_clear_count = 0;
                    fault_words.s.batt_blocks.bit.b8 = 0;
                }
            }
            if (bms_hr15 & 1) {
                if (batt_cal_stage == 0)
                    fault_words.s.batt_blocks.bit.b8 = 0;
            }
            if (dsp_settings.run_mode_req == 5 || dsp_settings.run_mode_req == 6) {
                if (dsp_settings.discharge_min_soc > dsp_settings.soc_reserve)
                    dsp_settings.soc_reserve = dsp_settings.discharge_min_soc;
            }
            force_chg_soc_f =
                (dsp_settings.soc_reserve > 4) ? (float)(dsp_settings.soc_reserve - 3) : 1.0f;
            force_chg_soc = force_chg_soc_f;
            force_chg_soc_isr = force_chg_soc;
            if (bms.s.soc <= force_chg_soc) {
                if (++force_chg_count >= 250) {
                    fault_words.s.batt_blocks.bit.b9 = 1;
                    force_chg_count = 0;
                }
            } else if (bms.s.soc >= force_chg_soc + 4) {
                fault_words.s.batt_blocks.bit.b9 = 0;
            }
            if (bms.s.soc <= force_chg_soc + 3) {
                if (++no_dis_count >= 250) {
                    no_dis_count = 0;
                    fault_words.s.batt_blocks.bit.b11 = 1;
                }
            } else if (bms.s.soc >= force_chg_soc + 7) {
                fault_words.s.batt_blocks.bit.b11 = 0;
            }
            fault_words.s.fault_batt.bit.b8 = bms.s.alarm_flags.bit.b0 | bms.s.alarm_flags.bit.b8 |
                                              bms.s.alarm_flags.bit.b3 | bms.s.alarm_flags.bit.b5 |
                                              bms.s.alarm_flags.bit.b12;
            fault_words.s.batt_blocks.bit.b10 = bms.s.status2.bit.b11;
            if (batt_cal_stage != 0)
                fault_words.s.batt_blocks.all &= 0xF4FF;
        } else {
            bms.s.charge_voltage_copy = 580;
            bms.s.bms_charge_limit_copy = 100;
            fault_words.w[7].all &= 0xF4FF;
            fault_words.w[3].bit.b8 = 0;
            batt_blocks.bit.b10 = 0;
        }
    } else {
        fault_words.s.batt_blocks.all &= 0xFCFF;
    }
    if (sched_window == 3 || bms.s.hr19_status.bit.b4)
        fault_words.s.fault_batt.bit.b15 = 1;
    else
        fault_words.s.fault_batt.bit.b15 = 0;
    if (!soc_floor_latch.bit.b3) {
        floor_latch_clear_count = 0;
        if (fault_words.s.batt_blocks.bit.b9 || fault_words.s.batt_blocks.bit.b11) {
            if (++floor_latch_set_count > 50) {
                soc_floor_latch.bit.b3 = 1;
                floor_latch_set_count = 0;
            }
        }
    } else {
        floor_latch_set_count = 0;
        if (has_grid_fault() == 1)
            floor_latch_clear_count = 30000;
        if (!fault_words.s.batt_blocks.bit.b9 && !fault_words.s.batt_blocks.bit.b11) {
            if (++floor_latch_clear_count > 30000) {
                soc_floor_latch.bit.b3 = 0;
                floor_latch_clear_count = 0;
            }
        } else {
            floor_latch_clear_count = 0;
        }
    }
}

/* The BMS current limits (0xD522..0xD528), the power limits (0xD50E..0xD51C) and the charge and
 * discharge current bounds (0xC212/0xC214); v is the HV-side voltage (0.1 V, 200..560 V).
 * In BMS mode (0.01 A):
 *   charge cap    = capacity (HR55) x HR111 % + 1.50 A, at most HR309;
 *   discharge cap = capacity x HR112 % + 1.50 A, at most HR309 + 15.00 A;
 *   charge limit  = HR26 x (1 - f), f the filtered (48.0 V - V) x 0.0225 held to 0..0.9,
 *                   at most the charge cap (full at 48 V and up, 10 % at 44 V and down);
 *   discharge limit = HR27 x (1 - g), g the filtered (V - 54.5 V) x 0.0257 held to 0..0.9,
 *                   at most the discharge cap (full at 54.5 V and down, 10 % from 58 V);
 *   (0xD502 bits 6/7, or 1/3 for discharge, would replace the taper with min(HR, 20 % of HR309);
 *   nothing writes 0xD502, so the tapers always apply). Without a BMS both are 60.00 A.
 *   Then charge is held to 1..80 A and discharge to 2..80 A. The filters are 0.98/0.02 per pass.
 * Power limits (0.1 W), each side the smallest of: limit x V (at most the rating), the charger
 * temperature derate (5 % per degree over 87.0), the bus derate (from 520 V, down to 10 % at 570 V
 * filtered), the SoC limits (soc_power_limits); the charge side also the PV limit chg_plim_pv
 * (two PIs cut it when PV power is over its rating or a PV current over 18 A; 10 W per pass).
 * Bounds (0.1 A, HV side) = 10 x power / v; schedule window 1 makes charge 0.25 A, window 2
 * discharge -0.25 A; HR19 bit 5 (outside a calibration) makes charge 0.1 A, or a ramp between
 * -0.5 and 0.1 A when more than 1 A of discharge is allowed. Held to -0.5..15 A and -15..0 A.
 * match: three things give the image's scheduling:
 * - the status word 0xD502 is read through a plain word pointer to the page (p[2]), not through
 *   the volatile struct member;
 * - the discharge power limit reads HR27's taper (0xD528) and the battery voltage (0xD212) under
 *   names of their own (dis_current_limit, batt_voltage), not as members of their pages;
 * - the discharge limit is one nested __fmin, like the charge limit, computed before it. */
void current_limit_tapers(float v) {
    float a = 0.0f;
    float b = a;
    float t, x;
    unsigned int *p = (unsigned int *)&bms;
    if (dsp_settings.battery_type != 0) {
        bms.s.cfg_d50b = 0;
        bms.s.limit_scale = 1;
        bms.s.max_charge_current_copy = ac.batt_max_charge_current;
        ac.batt_max_discharge_current = ac.batt_max_charge_current + 1500.0f;
        bms.s.dis_current_cap =
            dsp_settings.batt_capacity_ah * (float)dsp_settings.batt_discharge_limit_pct + 150.0f;
        if (bms.s.dis_current_cap > ac.batt_max_discharge_current)
            bms.s.dis_current_cap = ac.batt_max_discharge_current;
        bms.s.chg_current_cap =
            dsp_settings.batt_capacity_ah * (float)dsp_settings.batt_charge_limit_pct + 150.0f;
        bms.s.chg_current_cap = __fmin(bms.s.chg_current_cap, ac.batt_max_charge_current);
        if (!(((p[2] >> 7) & 1) | ((p[2] >> 6) & 1))) {
            chg_lowv_taper = 0.98f * chg_lowv_taper + 0.02f * (0.0225f * (480.0f - dc_meas.batt_v));
            limitf(&chg_lowv_taper, 0.9f, 0.0f);
            bms.s.chg_current_limit = bms.s.bms_charge_limit * (1.0f - chg_lowv_taper);
            bms.s.chg_current_limit = __fmin(bms.s.chg_current_limit, bms.s.chg_current_cap);
        } else {
            bms.s.chg_current_limit =
                __fmin((float)bms.s.bms_charge_limit,
                       0.01f * (20.0f * (ac.batt_max_charge_current * bms.s.limit_scale)));
        }
        t = bms.s.bms_discharge_limit;
        if (!(((p[2] >> 7) & 1) | ((p[2] >> 6) & 1) | ((p[2] >> 3) & 1) | ((p[2] >> 1) & 1))) {
            dis_highv_taper =
                0.98f * dis_highv_taper + 0.02f * (0.02571428f * (dc_meas.batt_v - 545.0f));
            limitf(&dis_highv_taper, 0.9f, 0.0f);
            bms.s.dis_current_limit = t * (1.0f - dis_highv_taper);
            bms.s.dis_current_limit = __fmin(bms.s.dis_current_limit, bms.s.dis_current_cap);
        } else {
            bms.s.dis_current_limit =
                __fmin(t, 0.01f * (20.0f * (ac.batt_max_discharge_current * bms.s.limit_scale)));
        }
    } else {
        bms.s.chg_current_limit = 6000.0f;
        bms.s.dis_current_limit = 6000.0f;
    }
    limitf(&bms.s.chg_current_limit, 8000.0f, 100.0f);
    limitf(&bms.s.dis_current_limit, 8000.0f, 200.0f);
    bms.s.chg_plim_current = 0.01f * (bms.s.chg_current_limit * dc_meas.batt_v);
    if (bms.s.chg_plim_current > batt_power_rating.chg_max)
        bms.s.chg_plim_current = batt_power_rating.chg_max;
    bms.s.dis_plim_current = 0.01f * (dis_current_limit * batt_voltage);
    if (bms.s.dis_plim_current > rating_params.batt_dis_power_max)
        bms.s.dis_plim_current = rating_params.batt_dis_power_max;
    if (dc_meas.charger_temp > rating_params.charger_temp_derate) {
        t = 0.1f * (dc_meas.charger_temp - rating_params.charger_temp_derate);
        t = __fmax(__fmin(t, 100.0f), 0.0f);
        bms.s.chg_plim_temp =
            rating_params.batt_chg_power_max - t * (0.05f * rating_params.batt_chg_power_max);
        bms.s.dis_plim_temp =
            rating_params.batt_dis_power_max - t * (0.05f * rating_params.batt_dis_power_max);
    } else {
        bms.s.chg_plim_temp = rating_params.batt_chg_power_max;
        bms.s.dis_plim_temp = rating_params.batt_dis_power_max;
    }
    if (dc_meas.bus_v >= 5200.0f) {
        bus_ov_derate = 0.98f * bus_ov_derate + 0.02f * (dc_meas.bus_v - 5200.0f);
        limitf(&bus_ov_derate, 500.0f, 0.0f);
        t = 1.0f - 0.0018f * bus_ov_derate;
        bms.s.chg_plim_bus = t * rating_params.batt_chg_power_max;
        bms.s.dis_plim_bus = t * rating_params.batt_dis_power_max;
    } else {
        bms.s.chg_plim_bus = rating_params.batt_chg_power_max;
        bms.s.dis_plim_bus = rating_params.batt_dis_power_max;
        bus_ov_derate = 0.0f;
    }
    if (dsp_settings.battery_type == 0) {
        bms.s.chg_plim_soc = rating_params.batt_chg_power_max;
        bms.s.dis_plim_soc = rating_params.batt_dis_power_max;
    } else if ((bms.s.hr19_status.all & 3) != 0 && bms.s.pack_voltage != 0) {
        soc_power_limits();
    }
    if (dsp_settings.batt_force_mode == 0 || dsp_settings.batt_force_mode == 2) {
        ac.pvp_pi.e = 0.01f * (dc_meas.pv_p - rating_params.pv_power_rated);
        ac.pvp_pi.kp = 0.3125f;
        ac.pvp_pi.ki = 0.1f;
        ac.pvp_pi.kd = 0.0f;
        ac.pvp_pi.umax = 11500.0f;
        ac.pvp_pi.umin = -11500.0f;
        pi_step(&ac.pvp_pi);
        if (ac.pvp_pi.u > 11500.0f)
            ac.pvp_pi.u = 11500.0f;
        else
            ac.pvp_pi.u = __fmax(ac.pvp_pi.u, -11500.0f);
        pv_current_max = __fmax(dc_meas.pv1_i_raw, dc_meas.pv2_i_raw);
        ac.pvi_pi.e = pv_current_max - 1800.0f;
        ac.pvi_pi.kp = 3.1f;
        ac.pvi_pi.ki = 1.0f;
        ac.pvi_pi.kd = 0.0f;
        ac.pvi_pi.umax = 20000.0f;
        ac.pvi_pi.umin = -20000.0f;
        pi_step(&ac.pvi_pi);
        if (ac.pvi_pi.u > 20000.0f)
            ac.pvi_pi.u = 20000.0f;
        else
            ac.pvi_pi.u = __fmax(ac.pvi_pi.u, -20000.0f);
        pv_pi_max = __fmax(ac.pvp_pi.u, ac.pvi_pi.u);
        chg_plim_pv = rating_params.batt_chg_power_max - pv_pi_max;
    } else {
        ac.pvp_pi.i = 0.0f;
        ac.pvp_pi.u = 0.0f;
        ac.pvi_pi.i = 0.0f;
        ac.pvi_pi.u = 0.0f;
        chg_plim_pv = rating_params.batt_chg_power_max;
    }
    rate_limit(&chg_plim_pv, chg_plim_pv_prev, 100.0f, -100.0f);
    limitf(&chg_plim_pv, rating_params.batt_chg_power_max, 0.0f);
    t = __fmin(bms.s.dis_plim_soc,
               __fmin(__fmin(bms.s.dis_plim_temp, bms.s.dis_plim_current), bms.s.dis_plim_bus));
    x = __fmin(bms.s.chg_plim_soc,
               __fmin(__fmin(bms.s.chg_plim_temp, bms.s.chg_plim_current), bms.s.chg_plim_bus));
    if (x > chg_plim_pv)
        x = chg_plim_pv;
    chg_plim_pv_prev = chg_plim_pv;
    a = 10.0f * x / v;
    b = 10.0f * -t / v;
    if (sched_window == 1)
        a = 2.5f;
    else if (sched_window == 2)
        b = -2.5f;
    if (bms.s.hr19_status.bit.b5 && batt_cal_stage == 0) {
        if (b < -10.0f) {
            if (dc_meas.bus_v >= 5200.0f)
                hr19b5_chg_ramp += 0.5f;
            else
                hr19b5_chg_ramp -= 0.5f;
            limitf(&hr19b5_chg_ramp, 1.0f, -5.0f);
            a = hr19b5_chg_ramp;
        } else {
            a = 1.0f;
        }
    }
    limitf(&a, isr_limits.hv_current_max, -5.0f);
    limitf(&b, 0.0f, -isr_limits.hv_current_max);
    DINT;
    isr_limits.chg_current_bound = a;
    isr_limits.dis_current_bound = b;
    EINT;
    chg_current_bound_i = 0.1f * a;
    dis_current_bound_i = 0.1f * b;
}
