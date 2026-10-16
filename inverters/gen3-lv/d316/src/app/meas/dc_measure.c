// clang-format off
/* The DC measurements of task 3 (meas_task.c) and their limit checks: PV voltages, currents and
   power, the DC bus, the battery and the DC/DC high side, temperatures, the insulation test, PV
   input detection and which boosts run.
   Entry points: the functions task3_measure calls, most of them once per grid cycle (the slow
   average event from dc_acc_step, about 20 ms), iso_fault_clear on task 3's 10 ms tick.
   Data: its own counters and results at 0xD140..0xD178 (below), including mppt_flags_blk (0xD176..
   0xD178; the third word is the MPPT flags, bit 14 = PV restart while in op_state 6); it fills
   dc_meas (the DC results block) from dc_acc (the per-cycle sums at 0xD180).
   Calibration: each gain is a count over 1024; a gain outside 870..1178 (0.85..1.15) is replaced by
   1.0, or clamped in place for the PV current and power gains.
   Units: voltages 0.1 V, PV currents 0.01 A, powers 0.1 W, temperatures 0.1 degree C. "Cycles"
   below are runs of the slow average branch, one per grid cycle.
   Neighbours: meas_task.c (the caller), dc_avg.c (the sums), mppt.c and mppt_step.c (the MPPT
   references), converter_ctrl.c (boost on flags, pv_stop_all), op_states.c.
   name: descriptive (was meas_b.c; map/filenames.tsv).
   at: meas_batt_dcdc 0x3EBE17; pv_ref_max_update 0x3EBE95; pv_input_detect_run 0x3EC018; bus_voltage_filter 0x3EC4C2;
       meas_init 0x3EC80A; iso_fault_clear 0x3EC8A0; meas_iso 0x3EC599; meas_aux_f16 0x3EC5A6
       bus_voltage_check 0x3EC2E3; pv_short_check 0x3EC4F7; pv_power_limit_flags 0x3EC54C; temp_check 0x3EC38A;
       eps_bus_voltage_check 0x3EC834; ntc_to_temp 0x3EC74F; meas_pv 0x3EC5BB; pv_voltage_check 0x3EC18C;
       meas_bus_voltage 0x3EC721; pv_power_limit_update 0x3EC416; meas_pv_power 0x3EC6B9; pv_boost_select 0x3EC026 */
// clang-format on
#include <string.h>
#include "util.h"
#include "os.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "meas.h"
#include "control.h"
#include "state.h"
#include "rating.h"
#include "F2806x_Gpio.h"

/* this file's variables, 0xD140..0xD178. The compiler does not keep their order; this order gives
 * the image's layout. Any change to this file can move them: rerun
 *     python3 tools/ebss_solve.py src/app/meas/dc_measure.c */

// clang-format off
/* .ebss begin */
unsigned int pv_detect_done = 0;
unsigned int pv_detect_retry = 0;
unsigned int pv_ov_clear_count = 0;
unsigned int pv1_present;
unsigned int bus_low_count = 0;
unsigned int charger_temp_count = 0;
unsigned int pv2_present;
unsigned int pv1_plim_count = 0;
unsigned int eps_bus_uv_fast_count = 0;
unsigned int batt_temp_count = 0;
unsigned int iso_fault_timer = 0;
unsigned int iso_fail_count = 0;
unsigned int ntc_count = 0;
unsigned int heatsink_temp_count = 0;
unsigned int pv2_short_count = 0;
unsigned int eps_bus_uv_slow_count = 0;
unsigned int pv_ov_count = 0;
unsigned int eps_bus_vs_hv_count = 0;
unsigned int pv2_plim_count = 0;
unsigned int pv_restart_count = 0;
unsigned int pv1_short_count = 0;
unsigned int pv_low_count = 0;
unsigned int pv_low_wait_count = 0;
unsigned int cf12b3_clear_count = 0;
unsigned int bus_ov_count = 0;
unsigned int iso_step = 0;
unsigned int meas_d149;
unsigned int rescan_settle_count = 0;
unsigned int pv_back_count = 0;
float iso_acc = 0;
float iso_avg_b = 0;
float iso_bus_v = 0;
float iso_resistance = 0;
float iso_const50 = 0;
float iso_conductance = 0;
float iso_sample;
float iso_resistance_out;
float bus_v_filt = 0;
float pv1_ref_max;
float pv2_ref_max;
float iso_avg_a = 0;
union mppt_flags_t mppt_flags_blk;
/* .ebss end */
// clang-format on

/* Battery voltage, high-side current and high-side voltage: the cycle means, calibrated; the
   battery power filter (0.7 old + 0.003 x I x V). The two gains also go to the interrupt
   (dc_gain_isr), with interrupts off. */
void meas_batt_dcdc(void) {
    float k1, k2;
    dc_meas.batt_v = dc_acc.a.inv_n * dc_acc.a.batt_v;
    dc_meas.hv_i = dc_acc.a.inv_n * dc_acc.a.hv_i;
    dc_meas.hv_v = dc_acc.a.inv_n * dc_acc.a.hv_v;
    dc_meas.batt_power = 0.7f * dc_meas.batt_power + 0.003f * (dc_meas.hv_i * dc_meas.hv_v);
    if (cal_gains.batt_v >= 870 && cal_gains.batt_v <= 1178)
        k1 = cal_gains.batt_v * 0.0009765625f;
    else
        k1 = 1.0f;
    if (cal_gains.hv_i >= 870 && cal_gains.hv_i <= 1178)
        k2 = cal_gains.hv_i * 0.0009765625f;
    else
        k2 = 1.0f;
    if (dc_meas.batt_v < 0.0f)
        dc_meas.batt_v = 0.0f;
    dc_meas.batt_v = k1 * dc_meas.batt_v;
    dc_meas.hv_i = k2 * dc_meas.hv_i;
    DINT;
    dc_gain_isr.batt_v = k1;
    dc_gain_isr.hv_i = k2;
    EINT;
}

/* The MPPT upper limit per input: the measured open-circuit voltage + 2.0 V. */
void pv_ref_max_update(void) {
    pv1_ref_max = mppt.pv1_voc + 20.0f;
    pv2_ref_max = mppt.pv2_voc + 20.0f;
}

/* Runs pv_input_detect when the run mode asks for PV (5 or 7) or a PV restart is pending (MPPT
   flag bit 14). */
void pv_input_detect_run(void) {
    if (run_mode_req == 5 || run_mode_req == 7 || mppt_flags_blk.s.flags.bit.b14)
        pv_input_detect();
}

/* Which PV boosts run and the bus reference for the interrupt. In the PV states (or with the
   restart flag), by pv_input_mode: 1 PV1, 2 PV2, 4 both in parallel (PV1 leads), 5 independent.
   A boost turns on when its MPPT voltage reference is more than 20.0 V below the bus reference and
   stays on while it is below it; while a boost runs the bus reference is at least the MPPT
   reference + 30.0 V, otherwise it follows the MPPT reference up. In mode 5 the input with the lower
   reference always boosts. In op_state 7 with no battery charge current allowed both boosts stay
   off. Outside the PV states the bus reference is just raised to the MPPT references. The result
   goes to the interrupt (bus_ref_isr) with interrupts off, for run modes 5 and 7. */
void pv_boost_select(void) {
    float x;
    int fa, fb;

    x = dc_meas.bus_v_ref;
    fa = 0;
    fb = 0;
    if (op_state == OP_GRID_PV || op_state == OP_EPS_PV || mppt_flags_blk.s.flags.bit.b14) {
        if (dsp_settings.pv_input_mode == 1) {
            if (pv1_boost_on == 0) {
                if (mppt.pv1_ref >= x - 200.0f) {
                    x = mppt.pv1_ref;
                    fb = 0;
                } else {
                    fb = 1;
                }
            } else {
                if (mppt.pv1_ref < x) {
                    fb = 1;
                    if (x < mppt.pv1_ref + 300.0f)
                        x = mppt.pv1_ref + 300.0f;
                } else {
                    x = mppt.pv1_ref;
                    fb = 0;
                }
            }
            fa = 0;
        } else if (dsp_settings.pv_input_mode == 2) {
            if (pv2_boost_on == 0) {
                if (mppt.pv2_ref >= x - 200.0f) {
                    fa = 0;
                    x = mppt.pv2_ref;
                } else {
                    fa = 1;
                }
            } else {
                if (mppt.pv2_ref < x) {
                    fa = 1;
                    if (x < mppt.pv2_ref + 300.0f)
                        x = mppt.pv2_ref + 300.0f;
                } else {
                    x = mppt.pv2_ref;
                    fa = 0;
                }
            }
            fb = 0;
        } else if (dsp_settings.pv_input_mode == 4) {
            if ((pv1_boost_on | pv2_boost_on) == 0) {
                if (mppt.pv1_ref >= x - 200.0f) {
                    x = mppt.pv1_ref;
                    fb = 0;
                    fa = 0;
                } else {
                    fb = 1;
                    fa = 1;
                }
            } else {
                if (mppt.pv1_ref < x) {
                    fb = 1;
                    fa = 1;
                    if (x < mppt.pv1_ref + 300.0f)
                        x = mppt.pv1_ref + 300.0f;
                } else {
                    x = mppt.pv1_ref;
                    fb = 0;
                    fa = 0;
                }
            }
        } else if (dsp_settings.pv_input_mode == 5) {
            if (pv_detect_done == 1 || pv_detect_done == 2) {
                if ((pv1_boost_on | pv2_boost_on) == 0) {
                    if (mppt.pv1_ref >= mppt.pv2_ref) {
                        if (mppt.pv1_ref < dc_meas.bus_v_ref - 200.0f) {
                            fb = 1;
                            x = dc_meas.bus_v_ref;
                        } else {
                            x = mppt.pv1_ref;
                            fb = 0;
                        }
                        fa = 1;
                    } else {
                        if (mppt.pv2_ref < dc_meas.bus_v_ref - 200.0f) {
                            x = dc_meas.bus_v_ref;
                            fa = 1;
                        } else {
                            fa = 0;
                            x = mppt.pv2_ref;
                        }
                        fb = 1;
                    }
                } else {
                    if (mppt.pv1_ref >= mppt.pv2_ref) {
                        if (mppt.pv1_ref < dc_meas.bus_v_ref - 200.0f) {
                            fb = 1;
                            x = dc_meas.bus_v_ref;
                            if (x < mppt.pv1_ref + 300.0f)
                                x = mppt.pv1_ref + 300.0f;
                        } else {
                            x = mppt.pv1_ref;
                            fb = 0;
                        }
                        fa = 1;
                    } else {
                        if (mppt.pv2_ref < dc_meas.bus_v_ref) {
                            fa = 1;
                            x = dc_meas.bus_v_ref;
                            if (x < mppt.pv2_ref + 300.0f)
                                x = mppt.pv2_ref + 300.0f;
                        } else {
                            fa = 0;
                            x = mppt.pv2_ref;
                        }
                        fb = 1;
                    }
                }
                if (op_state == OP_EPS_PV && ac.batt_max_charge_current == 0.0f) {
                    fb = 0;
                    fa = 0;
                }
            }
        }
        pv1_boost_on = fb;
        pv2_boost_on = fa;
    } else {
        if (dsp_settings.pv_input_mode == 1) {
            if (x < mppt.pv1_ref)
                x = mppt.pv1_ref;
        } else if (dsp_settings.pv_input_mode == 2) {
            if (x < mppt.pv2_ref)
                x = mppt.pv2_ref;
        } else if (dsp_settings.pv_input_mode == 5) {
            if (x < mppt.pv1_ref)
                x = mppt.pv1_ref;
            if (x < mppt.pv2_ref)
                x = mppt.pv2_ref;
        } else if (dsp_settings.pv_input_mode == 4) {
            if (x < mppt.pv1_ref)
                x = mppt.pv1_ref;
        } else {
            x = max3f(x, mppt.pv1_ref, mppt.pv2_ref);
        }
    }
    DINT;
    if (dsp_settings.run_mode_req == 5 || dsp_settings.run_mode_req == 7)
        bus_ref_isr = x;
    else
        bus_ref_isr = dc_meas.bus_v_ref;
    EINT;
}

/* The DC bus voltage filtered (0.5/0.5) and clamped to 300.0..585.0 V, and the modulation gain
   pwm_period / bus voltage that turns a bridge voltage into compare counts (mod_gain_isr). */
void bus_voltage_filter(void) {
    bus_v_filt = bus_v_filt * 0.5f + dc_meas.bus_v * 0.5f;
    limitf(&bus_v_filt, 5850.0f, 3000.0f);
    DINT;
    mod_gain = pwm_period / bus_v_filt;
    mod_gain_isr = mod_gain;
    EINT;
}

/* Clears the per-cycle sums and the MPPT block, and sets the MPPT step (5.0 V), its power
   hysteresis, 50 sweep steps per input, flag bits 6, 8 and 9, and the sample count 384. */
void meas_init(void) {
    memset(&dc_acc.b, 0, sizeof(dc_acc.b));
    memset(&dc_acc.a, 0, sizeof(dc_acc.a));
    memset(&mppt, 0, sizeof(mppt));
    memset(mppt_flags_blk.w, 0, sizeof(mppt_flags_blk.w));
    mppt.v_step = 50.0f;
    mppt.p_hyst = 60000.0f;
    mppt.pv1_sweep_steps = 50;
    mppt.pv2_sweep_steps = 50;
    mppt_flags_blk.w[2] |= 0x340;
    dc_acc.a.n = 0x180;
    dc_acc.b.n = 0x180;
}

/* On task 3's 10 ms tick: the insulation fault (CF17 bit 1) clears after 15000 ticks (150 s), and
   the insulation-passed bit (run_status bit 5) is cleared once the inverter runs in op_state 5. */
void iso_fault_clear(void) {
    if (fault_stop1.bit.b1) {
        if (++iso_fault_timer > 15000) {
            iso_fault_timer = 0;
            run_status_bits.bit.b5 = 0;
            fault_stop1.bit.b1 = 0;
        }
    }
    if (op_state == OP_GRID_PV && run_status_bits.bit.b5)
        run_status_bits.bit.b5 = 0;
}

/* The insulation channel's cycle mean (iso_sample), the input of pv_iso_check. */
void meas_iso(void) { iso_sample = dc_acc.a.iso * dc_acc.a.inv_n; }

/* The cycle mean of the slow residual current channel in mA (dc_meas.aux_f16) and in A
   (aux_f16_k, 0xD48A = rcmu_i[1], the RCMU's mean input). */
void meas_aux_f16(void) {
    dc_meas.aux_f16 = dc_acc.a.aux * dc_acc.a.inv_n;
    aux_f16_k = 0.001f * dc_meas.aux_f16;
}

/* DC bus over-voltage, CF17 bit 3: in op_states 5..8, above bus_ov_limit + 10.0 V for 5 cycles;
   then the count is set to 500 and counts down below bus_ov_limit - 10.0 V, so it clears after
   500 such cycles. CF12 bit 3 clears itself after 501 cycles. In the grid states a bus more than
   30.0 V under its reference for 51 cycles posts a stop to task 0. */
void bus_voltage_check(void) {
    if (op_state >= OP_GRID_PV) {
        if (dc_meas.bus_v > bus_ov_limit + 100.0f) {
            if (++bus_ov_count >= 5) {
                bus_ov_count = 500;
                fault_stop1.bit.b3 = 1;
            }
        } else {
            bus_ov_count = 0;
        }
    }
    if (fault_stop1.bit.b3) {
        if (dc_meas.bus_v < bus_ov_limit - 100.0f) {
            if (bus_ov_count == 0)
                fault_stop1.bit.b3 = 0;
            else
                bus_ov_count--;
        }
    }
    if (fault_words[0].bit.b3) {
        if (++cf12b3_clear_count > 500) {
            cf12b3_clear_count = 0;
            fault_words[0].bit.b3 = 0;
        }
    }
    if ((op_state == OP_GRID_PV || op_state == OP_GRID_BATT) &&
        dc_meas.bus_v + 300.0f < dc_meas.bus_v_ref) {
        if (bus_low_count < 50) {
            bus_low_count++;
        } else {
            bus_low_count = 0;
            os_post(TASK_STATE, EV0_STOP);
        }
    } else {
        bus_low_count = 0;
    }
}

/* PV short circuit: under 30.0 V with more than 3.00 A for 6 cycles sets CF17 bit 14 (PV1) or 15
   (PV2); above 120.0 V clears it. */
void pv_short_check(void) {
    if (dc_meas.pv1_v < 300.0f && dc_meas.pv1_i_raw > 300.0f) {
        if (pv1_short_count < 5) {
            pv1_short_count++;
        } else {
            pv1_short_count = 0;
            fault_stop1.bit.b14 = 1;
        }
    } else if (dc_meas.pv1_v > 1200.0f) {
        pv1_short_count = 0;
        fault_stop1.bit.b14 = 0;
    }
    if (dc_meas.pv2_v < 300.0f && dc_meas.pv2_i_raw > 300.0f) {
        if (pv2_short_count < 5) {
            pv2_short_count++;
        } else {
            fault_stop1.bit.b15 = 1;
            pv2_short_count = 0;
        }
    } else if (dc_meas.pv2_v > 1200.0f) {
        fault_stop1.bit.b15 = 0;
        pv2_short_count = 0;
    }
}

/* MPPT flags bits 0 and 1: the total PV power above pv_power_limit for 26 cycles sets each (both
   use the total), below it clears them. Only in op_states 5..8. */
void pv_power_limit_flags(void) {
    if (op_state >= OP_GRID_PV) {
        if (!mppt_flags_blk.s.flags.bit.b0) {
            if (dc_meas.pv_p > pv_power_limit && ++pv1_plim_count > 25) {
                pv1_plim_count = 0;
                mppt_flags_blk.s.flags.bit.b0 = 1;
            }
        } else if (dc_meas.pv_p < pv_power_limit) {
            mppt_flags_blk.s.flags.bit.b0 = 0;
        }
        if (!mppt_flags_blk.s.flags.bit.b1) {
            if (dc_meas.pv_p > pv_power_limit && ++pv2_plim_count > 25) {
                mppt_flags_blk.s.flags.bit.b1 = 1;
                pv2_plim_count = 0;
            }
        } else if (dc_meas.pv_p < pv_power_limit) {
            mppt_flags_blk.s.flags.bit.b1 = 0;
        }
    } else {
        pv1_plim_count = 0;
        pv2_plim_count = 0;
    }
}

/* Over-temperature: heatsink above its limit for 50 cycles sets CF17 bit 7, the charger CF17 bit
   6; each then counts down 500 cycles below the limit - 10.0 degrees before it clears. The
   battery temperature (limit 100.0) is counted the same way but nothing reads the count, so it
   has no effect (docs/names/B-notes.md). */
void temp_check(void) {
    float v, lim;
    if (!fault_stop1.bit.b7) {
        if (dc_meas.heatsink_temp > heatsink_temp_limit) {
            if (++heatsink_temp_count >= 50) {
                heatsink_temp_count = 500;
                fault_stop1.bit.b7 = 1;
            }
        }
    } else if (dc_meas.heatsink_temp < heatsink_temp_limit - 100.0f) {
        if (--heatsink_temp_count == 0) {
            heatsink_temp_count = 0;
            fault_stop1.bit.b7 = 0;
        }
    }
    if (!fault_stop1.bit.b6) {
        if (dc_meas.charger_temp > charger_temp_limit) {
            if (++charger_temp_count >= 50) {
                charger_temp_count = 500;
                fault_stop1.bit.b6 = 1;
            }
        }
    } else if (dc_meas.charger_temp < charger_temp_limit - 100.0f) {
        if (--charger_temp_count == 0) {
            charger_temp_count = 0;
            fault_stop1.bit.b6 = 0;
        }
    }
    lim = batt_temp_limit;
    v = dc_meas.batt_temp;
    if (v > lim) {
        if (++batt_temp_count >= 50)
            batt_temp_count = 500;
    } else if (v < lim - 50.0f) {
        if (--batt_temp_count == 0)
            batt_temp_count = 0;
    }
}

/* EPS bus under-voltage, CF14 bit 11, in op_states 7/8: the bus under 0.79 x sqrt 2 x the
   nominal voltage for 150 cycles, under 0.75 x for 5, or less than 20.0 V above the DC/DC high
   side for 50. */
void eps_bus_voltage_check(void) {
    float pk;
    if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT) {
        pk = 1.4142135f * grid_v_nominal;
        if (dc_meas.bus_v < 0.79f * pk) {
            if (++eps_bus_uv_slow_count >= 150) {
                eps_bus_uv_slow_count = 0;
                fault_grid_eps.bit.b11 = 1;
            }
        } else {
            eps_bus_uv_slow_count = 0;
        }
        if (dc_meas.bus_v < 0.75f * pk) {
            if (++eps_bus_uv_fast_count >= 5) {
                eps_bus_uv_fast_count = 0;
                fault_grid_eps.bit.b11 = 1;
            }
        } else {
            eps_bus_uv_fast_count = 0;
        }
        if (dc_meas.bus_v < dc_meas.hv_v + 200.0f) {
            if (++eps_bus_vs_hv_count >= 50) {
                fault_grid_eps.bit.b11 = 1;
                eps_bus_vs_hv_count = 0;
            }
        } else {
            eps_bus_vs_hv_count = 0;
        }
    } else {
        eps_bus_uv_fast_count = 0;
        eps_bus_uv_slow_count = 0;
        eps_bus_vs_hv_count = 0;
    }
}

/* An NTC curve as ten line segments (ADC counts to temperature). Not called: see ntc_temp_check. */
float ntc_to_temp(float x) {
    float r = 0.0f;
    if (x < 408.0f)
        r = 1451.0f - 1.1098633f * x;
    else if (x < 528.0f)
        r = 1344.0f - 0.84375f * x;
    else if (x < 685.0f)
        r = 1242.0f - 0.6464844f * x;
    else if (x < 885.0f)
        r = 1146.0f - 0.5053711f * x;
    else if (x < 1134.0f)
        r = 1059.0f - 0.40600586f * x;
    else if (x < 1429.0f)
        r = 987.0f - 0.3413086f * x;
    else if (x < 1760.0f)
        r = 933.0f - 0.30273438f * x;
    else if (x < 2106.0f)
        r = 909.0f - 0.2890625f * x;
    else if (x < 2437.0f)
        r = 933.0f - 0.3005371f * x;
    else if (x < 2959.0f)
        r = 1133.0f - 0.3828125f * x;
    return r;
}

/* PV voltages and currents: the cycle means, calibrated; negative currents become 0, and an input
   under 10.0 V with under 0.30 A reads as 0. The voltage gains also go to the interrupt. */
void meas_pv(void) {
    float k1, k2;
    dc_meas.pv1_v_raw = dc_acc.a.inv_n * dc_acc.a.pv1_v;
    dc_meas.pv2_v_raw = dc_acc.a.inv_n * dc_acc.a.pv2_v;
    if (cal_gains.pv1_v >= 870 && cal_gains.pv1_v <= 1178)
        k1 = cal_gains.pv1_v * 0.0009765625f;
    else
        k1 = 1.0f;
    if (cal_gains.pv2_v >= 870 && cal_gains.pv2_v <= 1178)
        k2 = cal_gains.pv2_v * 0.0009765625f;
    else
        k2 = 1.0f;
    dc_meas.pv1_v = k1 * dc_meas.pv1_v_raw;
    dc_meas.pv2_v = k2 * dc_meas.pv2_v_raw;
    DINT;
    pv_gain_isr.pv1_v = k1;
    pv_gain_isr.pv2_v = k2;
    EINT;
    dc_meas.pv_v_max = __fmax(dc_meas.pv1_v, dc_meas.pv2_v);
    dc_meas.pv_v_min = __fmin(dc_meas.pv2_v, dc_meas.pv1_v);
    dc_meas.pv1_i_raw = dc_acc.a.inv_n * dc_acc.a.pv1_i;
    dc_meas.pv2_i_raw = dc_acc.a.inv_n * dc_acc.a.pv2_i;
    limitu(&cal_gains.pv1_i, 1178, 870);
    dc_meas.pv1_i = 0.0009765625f * (cal_gains.pv1_i * dc_meas.pv1_i_raw);
    limitu(&cal_gains.pv2_i, 1178, 870);
    dc_meas.pv2_i = 0.0009765625f * (cal_gains.pv2_i * dc_meas.pv2_i_raw);
    if (dc_meas.pv1_i_raw < 0.0f)
        dc_meas.pv1_i_raw = 0.0f;
    if (dc_meas.pv2_i_raw < 0.0f)
        dc_meas.pv2_i_raw = 0.0f;
    if (dc_meas.pv1_i < 0.0f)
        dc_meas.pv1_i = 0.0f;
    if (dc_meas.pv2_i < 0.0f)
        dc_meas.pv2_i = 0.0f;
    if (dc_meas.pv1_v < 100.0f && dc_meas.pv1_i_raw < 30.0f) {
        dc_meas.pv1_v_raw = 0.0f;
        dc_meas.pv1_i_raw = 0.0f;
        dc_meas.pv1_v = 0.0f;
        dc_meas.pv1_i = 0.0f;
    }
    if (dc_meas.pv2_v < 100.0f && dc_meas.pv2_i_raw < 30.0f) {
        dc_meas.pv2_v_raw = 0.0f;
        dc_meas.pv2_i_raw = 0.0f;
        dc_meas.pv2_v = 0.0f;
        dc_meas.pv2_i = 0.0f;
    }
}

/* Decides the PV input mode (pv_input_mode): an input is present above 120.0 V and absent at or
   below 90.0 V. One input gives mode 1 or 2, none mode 0; both give 4 (parallel, when HR305 says
   so) or 5 (independent), after three retries. A change from a two-input mode asks for an MPPT
   rescan. */
void pv_input_detect(void) {
    if (dc_meas.pv1_v > 900.0f) {
        if (dc_meas.pv1_v > 1200.0f)
            pv1_present = 1;
    } else {
        pv1_present = 0;
    }
    if (dc_meas.pv2_v > 900.0f) {
        if (dc_meas.pv2_v > 1200.0f)
            pv2_present = 1;
    } else {
        pv2_present = 0;
    }
    if (pv1_present == 1 && pv2_present == 0) {
        if (pv_input_mode >= 3)
            mppt.rescan = 1;
        pv_input_mode = 1;
        pv_detect_done = 1;
        pv_detect_retry = 0;
    } else if (pv1_present == 0 && pv2_present == 1) {
        if (pv_input_mode >= 3)
            mppt.rescan = 1;
        pv_input_mode = 2;
        pv_detect_done = 1;
        pv_detect_retry = 0;
    } else if (pv1_present == 0 && pv2_present == 0) {
        mppt.rescan = 1;
        pv_input_mode = 0;
        pv_detect_done = 1;
        pv_detect_retry = 0;
    } else {
        if (pv_input_mode < 3) {
            if (op_state == OP_GRID_PV || op_state == OP_EPS_PV || mppt_flags_blk.s.flags.bit.b14)
                mppt.rescan = 1;
            if (++pv_detect_retry < 3)
                return;
            pv_detect_retry = 0;
        }
        pv_input_mode = (pv_mode_cfg == 1) ? 4 : 5;
        pv_detect_done = 1;
    }
}

/* A stubbed NTC check: it compares the constant 2437 with 3350, so CF17 bit 2 is never set
   (docs/findings-control.md). */
void ntc_temp_check(void) {
    if (!fault_stop1.bit.b2) {
        if (__fmax(2437.0f, 2437.0f) > 3350.0f) {
            if (++ntc_count > 500) {
                fault_stop1.bit.b2 = 1;
                ntc_count = 500;
            }
        }
    } else if (ntc_count != 0 && --ntc_count == 0)
        fault_stop1.bit.b2 = 0;
}

/* The PV insulation test, one step per cycle, in the start state only (task 3 calls it with the
   check enabled and run mode 5): GPIO56 on for steps 1..109 and the mean of steps 69..100 taken,
   then GPIO20 on for steps 110..219 and the mean of steps 179..210 taken; from the bus voltage and
   the difference of the two means a conductance and its inverse, the resistance (units not
   established). Under 50 three times sets CF17 bit 1; otherwise run_status bit 5 (passed) is set. */
void pv_iso_check(void) {
    float v, d;
    iso_const50 = 50.0f;
    iso_bus_v = dc_meas.bus_v * 0.1f;
    v = iso_sample;
    if (run_status_bits.bit.b5 || op_state != OP_START) {
        GpioDataRegs.GPBCLEAR.bit.GPIO56 = 1;
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;
        iso_step = 0;
        iso_fail_count = 0;
        return;
    }
    iso_step++;
    if (iso_step < 110) {
        GpioDataRegs.GPBSET.bit.GPIO56 = 1;
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;
        if (iso_step > 100)
            iso_avg_a = iso_acc * 0.03125f;
        else if (iso_step > 68)
            iso_acc += v;
        else
            iso_acc = 0.0f;
    } else if (iso_step < 220) {
        GpioDataRegs.GPBCLEAR.bit.GPIO56 = 1;
        GpioDataRegs.GPASET.bit.GPIO20 = 1;
        if (iso_step > 210)
            iso_avg_b = iso_acc * 0.03125f;
        else if (iso_step > 178)
            iso_acc += v;
        else
            iso_acc = 0.0f;
    } else if (iso_step < 222) {
        GpioDataRegs.GPBCLEAR.bit.GPIO56 = 1;
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;
        if (iso_avg_a > iso_avg_b) {
            d = iso_bus_v / (iso_avg_a - iso_avg_b);
            iso_conductance = d * 0.0008571429f - 0.0011428571f - 0.0010204081f;
            if (iso_conductance > 1.526e-05f)
                iso_resistance = 1.0f / iso_conductance;
            else
                iso_resistance = 65530.0f;
            iso_resistance_out = iso_resistance;
        } else {
            iso_conductance = 0.0f;
            iso_resistance = 0.0f;
        }
    } else {
        if (iso_resistance < 50.0f) {
            if (++iso_fail_count >= 3) {
                iso_fail_count = 3;
                fault_stop1.bit.b1 = 1;
                run_status_bits.bit.b5 = 0;
            }
        } else {
            iso_fail_count = 0;
            fault_stop1.bit.b1 = 0;
            run_status_bits.bit.b5 = 1;
        }
        iso_step = 0;
    }
}

/* PV over-voltage, CF17 bit 4: the higher input above pv_ov_limit + 35.0 V for 6 cycles; it
   clears after 500 cycles with both inputs 10.0 V under the limit.
   PV absent, CF16 bit 5: set during a battery calibration with batt_force_mode 2; otherwise set
   when both inputs stay under the start voltage (HR60; 105.0 V in the PV states) for 51 cycles.
   It clears when an input comes within 10.0 V of the bus reference for 51 cycles, or when an input
   reaches the start voltage: in op_state 6 that sets the restart flags (0x4040) instead, and the
   restart is dropped if the PV falls under 105.0 V, or after 3001 cycles (clearing the fault too if
   an input gives 3000.0 W or more). */
void pv_voltage_check(void) {
    unsigned int u = pv_start_voltage;
    dc_meas.pv_v_max = __fmax(dc_meas.pv1_v, dc_meas.pv2_v);
    if (!fault_stop1.bit.b4) {
        if (dc_meas.pv_v_max > pv_ov_limit + 350.0f) {
            if (++pv_ov_count > 5) {
                pv_ov_count = 0;
                fault_stop1.bit.b4 = 1;
            }
        } else {
            pv_ov_count = 0;
        }
    } else {
        float m = pv_ov_limit;
        if (dc_meas.pv1_v + 100.0f < m && dc_meas.pv2_v + 100.0f < m) {
            if (++pv_ov_clear_count >= 500) {
                pv_ov_clear_count = 0;
                fault_stop1.bit.b4 = 0;
            }
        } else {
            pv_ov_clear_count = 0;
        }
    }
    if (batt_cal_stage != 0 && dsp_settings.batt_force_mode == 2) {
        fault_pv.bit.b5 = 1;
    } else if (!fault_pv.bit.b5) {
        if (op_state == OP_GRID_PV || op_state == OP_EPS_PV)
            u = 1050;
        if (dc_meas.pv1_v < u && dc_meas.pv2_v < u) {
            if (++pv_low_count > 50) {
                fault_pv.bit.b5 = 1;
                pv_low_count = 0;
            }
        } else {
            pv_low_count = 0;
        }
    } else if (dc_meas.pv1_v > dc_meas.bus_v_ref - 100.0f ||
               dc_meas.pv2_v > dc_meas.bus_v_ref - 100.0f) {
        if (++pv_back_count > 50) {
            mppt_flags_blk.s.flags.bit.b14 = 0;
            fault_pv.bit.b5 = 0;
            pv_back_count = 0;
        }
    } else {
        pv_back_count = 0;
        if (!mppt_flags_blk.s.flags.bit.b14) {
            if (dc_meas.pv1_v >= pv_start_voltage || dc_meas.pv2_v >= pv_start_voltage) {
                if (op_state == OP_GRID_BATT) {
                    if (++pv_restart_count > 50) {
                        mppt_flags_blk.s.flags.all |= 0x4040;
                        pv_restart_count = 0;
                    }
                } else if (++pv_low_count > 50) {
                    fault_pv.bit.b5 = 0;
                    pv_low_count = 0;
                }
            }
        } else if (op_state == OP_GRID_BATT) {
            if (dc_meas.pv1_v >= 1050.0f || dc_meas.pv2_v >= 1050.0f) {
                if (++pv_low_wait_count > 3000) {
                    pv_low_wait_count = 0;
                    if (dc_meas.pv1_p >= 30000.0f || dc_meas.pv2_p >= 30000.0f) {
                        mppt_flags_blk.s.flags.bit.b14 = 0;
                        fault_pv.bit.b5 = 0;
                    } else {
                        mppt_flags_blk.s.flags.bit.b14 = 0;
                    }
                }
            } else {
                pv_low_wait_count = 0;
                mppt_flags_blk.s.flags.bit.b14 = 0;
                DINT;
                pv_stop_all();
                EINT;
            }
        } else {
            mppt_flags_blk.s.flags.bit.b14 = 0;
            fault_pv.bit.b5 = 1;
            pv_low_wait_count = 0;
        }
    }
}

/* DC bus voltage: the cycle mean, calibrated; the gain also goes to the interrupt. */
void meas_bus_voltage(void) {
    unsigned int r = cal_gains.bus_v;
    float k;
    float p = dc_acc.a.bus_v * dc_acc.a.inv_n;
    if (r >= 870 && r <= 1178)
        k = r * 0.0009765625f;
    else
        k = 1.0f;
    dc_meas.bus_v = p * k;
    DINT;
    dc_gain_isr.bus_v = k;
    EINT;
}

/* The PV power limit: rated, derated by 0.06 % per V above the derate voltage, by 0.2 % per V
   (over the whole span) more than 40 V above it, and by 24 % above 545.0 V (at most 70 %);
   filtered 0.6/0.4 and kept within 10..100 % of rated. */
void pv_power_limit_update(void) {
    float a[4];
    volatile float k;
    float m, v, ref, c;
    unsigned int i;
    k = 0.0f;
    c = rating_params.pv_power_rated;
    ref = rating_params.pv_derate_voltage;
    v = dc_meas.pv_v_max;
    a[0] = c;
    a[1] = c;
    a[2] = c;
    if (v > ref) {
        if (v > 5450.0f)
            k = 24.0f;
        else if (v > ref + 400.0f)
            k = 0.2f * (0.1f * (dc_meas.pv_v_max - rating_params.pv_derate_voltage));
        else if (v > ref)
            k = 0.06f * (0.1f * (v - ref));
        limitf((float *)&k, 70.0f, 0.0f);
        a[3] = rating_params.pv_power_rated - 0.01f * (rating_params.pv_power_rated * k);
    } else {
        a[3] = c;
    }
    m = rating_params.pv_power_rated;
    for (i = 0; i < 4; i++)
        m = __fmin(m, a[i]);
    rating_params.pv_power_limit = 0.6f * rating_params.pv_power_limit + 0.4f * m;
    limitf(&rating_params.pv_power_limit, rating_params.pv_power_rated,
           0.1f * rating_params.pv_power_rated);
}

/* PV power per input: V x I filtered (0.6 old + 0.4 new), times the power gain (limited in place
 * to 870..1178) / 1024, and their sum dc_meas.pv_p (0.1 W, as used by pv_power_limit_flags).
 * match: the scaled b product is a temporary of its own (e = b * k; e = c * e;), which puts the
 * D22A product in the b*k register as the image has it. */
void meas_pv_power(void) {
    float a, b, x, c, d, e;
    a = dc_meas.pv1_v * dc_meas.pv1_i_raw;
    d = dc_meas.pv2_v * dc_meas.pv2_i_raw;
    dc_meas.pv1_p_filt = 0.6f * dc_meas.pv1_p_filt + 0.4f * a;
    dc_meas.pv2_p_filt = 0.6f * dc_meas.pv2_p_filt + 0.4f * d;
    a = dc_meas.pv1_p_filt;
    b = dc_meas.pv2_p_filt;
    limitu(&cal_gains.pv1_p, 1178, 870);
    x = cal_gains.pv1_p;
    limitu(&cal_gains.pv2_p, 1178, 870);
    e = b * 0.0009765625f;
    dc_meas.pv1_p = x * (a * 0.0009765625f);
    c = cal_gains.pv2_p;
    e = c * e;
    dc_meas.pv2_p = e;
    dc_meas.pv_p = dc_meas.pv2_p + dc_meas.pv1_p;
}
