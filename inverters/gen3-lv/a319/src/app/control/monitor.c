/* at: meter_records_5_6 0x20003DE0, meter_records_7_8 0x20003F00 */
// clang-format off
/* at: isqrt_div 0x080250E8, fault_set 0x08025136, fault_clear 0x0802514E, fault_test 0x08025166, dsp_warning_test 0x08025178, fault_check_mode 0x0802518A, fault_none 0x080251B2, buzzer_off 0x080251C0, buzzer_on 0x080251CA, counter_wrap_inc 0x080251D4, led_status_update 0x080251E0, fault_code_update 0x080253E0, fault_led_update 0x080253FC, fault_log_push 0x08025458, event_log_push 0x080255C8, eeprom_reload_flagged 0x08025612, calib_table_defaults 0x080256BE, ntc_heatsink_temp_update 0x08025708, ntc_charger_temp_update 0x08025918, ntc_stub 0x08025B5C, ntc_battery_temp_update 0x08025B5E, batt_soc_from_voltage 0x08025D5A, drm_mode_eval 0x08025E62, power_flow_update 0x08026004, task_monitor 0x0802689A */
/* at: led_status_tick 0x20000095, fault_led_tick 0x20000096, ntc_heatsink_acc 0x20000098, ntc_heatsink_cnt 0x2000009C, ntc_charger_acc 0x200000A0, ntc_charger_cnt 0x200000A4, ntc_battery_acc 0x200000A8, ntc_battery_cnt 0x200000AC, eeprom_save_result 0x200000BC, eeprom_save_retries 0x200000BD, eeprom_reload_count 0x200000BE, fault_code_prev 0x200000C0, fault_code_settle 0x200000C2, aux_tx_last_byte 0x200000C4, g_200000D0 0x200000D0, aux_tx_idle_count 0x200000D2, aux_link_timeout 0x200000D4, ntc_battery_valid 0x200000D6, fault_led_armed 0x200000D7, ct_reversed 0x200000D8, ct_reverse_debounce 0x200000DA, settings_commit_flags 0x2000018C, cfg_changed_flags 0x2000019C, cfg_reset_mode 0x2000019D, eeprom_load_failed 0x200001A6, eeprom_load_ok 0x200001A7, save_req_pending 0x200001A8, grid_country 0x200001AB, inv_run_mode 0x200001AE, dsp_ignored_mode_byte 0x200001B0, g_200001B2 0x200001B2, fault_led_on 0x200001B3, dsp_fault_data1 0x20000230, dsp_fault_data2 0x20000232, fault_code 0x2000023E, eeprom_transaction_state 0x20000266, fault_recover_window 0x20000288, eeprom_busy 0x2000028C, eeprom_busy_2 0x2000028D, rtc_time 0x2000028F, drm_bits 0x20000298, adc_wave_idx 0x2000029A, aux_tx_byte 0x2000029C, aux_tx_repeat_count 0x2000029D, aux_tx_restart_count 0x2000029E, usb_reset_state 0x200002BC, aux_rx_state 0x200002C0, aux_rx_bit 0x200002C1, aux_pages_seen 0x200002D4, charge_status 0x200002D5, dsp_warning_word 0x200002D6, batt_state_dsp 0x200002DB, batt_soc 0x200002E2, batt_soc_dsp 0x200002E3, batt_temp_dsp 0x200002E6, adc_ntc_charger 0x200002FA, adc_ntc_heatsink 0x200002FE, heatsink_temp 0x20000300, adc_ntc_battery 0x20000302, dsp_flash_step 0x20000308, usb_sn_written 0x20000312, batt_full_flag 0x20000313, drm_power_limit 0x20000314, drm0_active 0x20000315, dsp_flash_mode 0x200003A4, eeprom_save_delay 0x200003D0, os_task_dly_idx 0x200004D8, event_log 0x20000FC4, fault_log 0x200019C3, cfg_settings 0x20001ABA, cal_gains 0x20001B8D, energy_totals 0x20001BD1, aux_wave_buf 0x2000206C, adc_wave_buf 0x200021EC, meter_data 0x20003BA0, meter_record_3 0x20003F00, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* monitor.c: task_monitor, the 20 ms housekeeping task, and what it calls: the status and fault
   LEDs, the NTC temperatures, the SOC (from the DSP for lithium, from the voltage for lead
   acid), the aux MCU link and the CT power it measures, DRM, the power flow figures, and the
   EEPROM save loop. Also the fault flag helpers used everywhere.

   Entry points: task_monitor (task 5); fault_set, fault_clear, fault_check_mode,
   fault_code_update, dsp_warning_test, buzzer_on/off, isqrt_div, calib_table_defaults,
   drm_mode_eval (also from the USART1 interrupt), power_flow_update. Dead: fault_test,
   fault_none, counter_wrap_inc, fault_log_push, event_log_push.
   Data: fault_flags (declared int here, see the match note), the NTC accumulators, the CT
   accumulators, the aux link counters, the EEPROM save state.
   Neighbours: util.c before it, measure.c after it; drivers/eeprom.c (eeprom_save_dirty,
   eeprom_load), stm32f10x_it.c (the ADC and aux link data), comm/meter.c (meter_data),
   comm/dsp_link.c (the DSP values), board.c (LEDs).

   Faults: fault_flags is a bit set; fault_code is the number of its lowest set bit (1-32);
   FAULT_AUX_LINK (0x40) alone does not stop the inverter, any other bit sets run mode 3. The
   fault and event logs are kept in EEPROM but nothing writes them: fault_log_push and
   event_log_push have no caller (docs/findings.md).

   name: descriptive (map/filenames.tsv). It starts with isqrt_div; fault_set at 0x08025136 is 2
   mod 4, so no file starts there. */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#define OWN_fault_flags /* match: declared below as int: this file compiled against that, not the header's ushort */
#include "../../include/battery.h"
#include "../../include/board.h"
#include "../../include/clib.h"
#include "../../include/dsp_link.h"
#include "../../include/eeprom.h"
#include "../../include/energy.h"
#include "../../include/faults.h"
#include "../../include/ff.h"
#include "../../include/measure.h"
#include "../../include/meter.h"
#include "../../include/modbus.h"
#include "../../include/rtc.h"
#include "../../include/schedule.h"
#include "../../include/settings.h"
#include "../../include/stdperiph.h"
#include "../../include/sys.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"
#include "../../include/usb.h"
extern int fault_flags;

/* Integer square root of sum / n (an RMS from a sum of squares), by five Newton steps from a
   first guess picked by size. Returns 0 when sum or n is 0. */
int isqrt_div(uint value, int divisor) {
    uint root;
    uchar i;

    if (value == 0 || divisor == 0) {
        return 0;
    }
    value = value / divisor;
    if (value <= 0x400000) {
        root = (value >> 0xa) + 0x3f;
    } else {
        if (value <= 0x8000000) {
            root = (value >> 0xc) + 0xff;
        } else {
            root = (value >> 0xe) + 0x3ff;
        }
    }
    for (i = 0; i < 5; i++) {
        root = value / root + root >> 1;
    }
    return (ushort)root;
}

/* Sets fault bits (with interrupts masked). */
void fault_set(uint fault_mask) {

    OS_ENTER_CRITICAL();
    fault_flags = fault_flags | fault_mask;
    OS_EXIT_CRITICAL();
    return;
}

/* Clears fault bits (with interrupts masked). */
void fault_clear(uint fault_mask) {

    OS_ENTER_CRITICAL();
    fault_flags = fault_flags & ~fault_mask;
    OS_EXIT_CRITICAL();
    return;
}

/* Dead. 1 when any of the bits is set. */
uchar fault_test(uint fault_mask) { return (fault_flags & fault_mask) != 0; }

/* Tests bits of the DSP's warning word (frame bytes 7-8). */
uchar dsp_warning_test(ushort warning_mask) { return (dsp_warning_word & warning_mask) != 0; }

/* Returns 1 (and sets run mode 3) when a fault other than the aux link fault is set, 0 when
   there is none; in run mode 4 (firmware update) it returns 1 and changes nothing. */
int fault_check_mode() {

    if (inv_run_mode != 4) {
        if (fault_flags == 0 || fault_flags == FAULT_AUX_LINK) {
            return 0;
        }
        inv_run_mode = 3;
        return 1;
    }
    return 1;
}

/* Dead. 1 when no fault is set. */
uchar fault_none() { return fault_flags == 0; }

/* Buzzer pin low. */
void buzzer_off() {

    buzzer_pin_set(0);
    return;
}

/* Buzzer pin high. */
void buzzer_on() { buzzer_pin_set(1); }

/* Dead. Increments a byte counter and wraps it from above max back to min. */
int counter_wrap_inc(int count, int min_count, int max_count) {
    count = (uchar)(count + 1);
    if (count > max_count)
        count = min_count;
    return count;
}

/* Drives status LEDs 1 and 2 from the run mode, every 20 ms:

     firmware relay      alternate, 200 ms each
     mode 1 (normal)     LED 1 on, or a short blink-off (25 of 200 passes) while the DSP SOC is
                         at or below HR114 + 1 and no calibration runs (inference: "battery at
                         reserve"); LED 2 on for charge status 6 or the meter fault
     mode 3 (fault)      LED 2 on
     mode 0 (waiting)    both on for 1 s, off for 1 s
     mode 4 (update)     alternate, 0.8 s each, until the DSP update reaches step 0xA3
     mode 2              LED 2 blinks */
void led_status_update() {

    if (dsp_flash_mode >= 1) {
        led_status_tick = led_status_tick + 1;
        if (led_status_tick < 0xa) {
            led_set(1, 0);
            led_set(2, 1);
        } else {
            if (led_status_tick < 0x14) {
                led_set(1, 1);
                led_set(2, 0);
            } else {
                led_status_tick = 0;
            }
        }
    } else {
        if (inv_run_mode == 1) {
            if (cfg_settings.battery_discharge_min_power_reserve + 1 >= batt_soc_dsp &&
                cfg_settings.soc_force_adjust == 0) {
                led_status_tick = led_status_tick + 1;
                if (led_status_tick > 0xaf) {
                    if (led_status_tick >= 0xc8) {
                        led_status_tick = 0;
                    }
                    led_set(1, 0);
                } else {
                    led_set(1, 1);
                }
            } else {
                led_set(1, 1);
            }
            if (charge_status == 6 || board_init_done >= 1) {
                led_set(2, 1);
            } else {
                led_set(2, 0);
            }
        } else {
            if (inv_run_mode == 3) {
                led_set(1, 0);
                led_set(2, 1);
            } else {
                if (inv_run_mode == 0) {
                    led_status_tick = led_status_tick + 1;
                    if (led_status_tick > 0x32) {
                        if (led_status_tick == 0x64) {
                            led_status_tick = 0;
                        }
                        led_set(1, 0);
                        led_set(2, 0);
                    } else if (led_status_tick > 0) {
                        led_set(1, 1);
                        if (charge_status == 6 || board_init_done >= 1) {
                            led_set(2, 1);
                        } else {
                            led_set(2, 0);
                        }
                    }
                } else if (inv_run_mode == 4) {
                    if (dsp_flash_step < 0xa3) {
                        led_status_tick = led_status_tick + 1;
                        if (led_status_tick > 0x28) {
                            if (led_status_tick == 0x50) {
                                led_status_tick = 0;
                            }
                            led_set(1, 0);
                            led_set(2, 1);
                        } else if (led_status_tick > 0) {
                            led_set(1, 1);
                            led_set(2, 0);
                        }
                    }
                } else if (inv_run_mode == 2) {
                    led_status_tick = led_status_tick + 1;
                    if (led_status_tick > 0x32) {
                        if (led_status_tick == 0x64) {
                            led_status_tick = 0;
                        }
                        led_set(1, 0);
                        led_set(2, 1);
                    } else if (led_status_tick > 0) {
                        led_set(1, 0);
                        led_set(2, 0);
                    }
                }
            }
        }
    }
}

/* Outside run mode 4: re-checks the run mode and sets fault_code to the lowest fault bit (1-32,
   0 for none). */
void fault_code_update() {

    if (inv_run_mode != 4) {
        fault_check_mode();
        fault_code = fault_lowest_bit(fault_flags);
    }
}

/* Fault LED (PB9): while a fault is set, on for 20 passes of 90, else off. */
void fault_led_update() {

    fault_code_update();
    if (fault_code > 0) {
        if (fault_led_tick < 0x14) {
            fault_led_on = 1;
            led_fault_set(1);
        } else {
            if (fault_led_tick < 0x5a) {
                fault_led_on = 0;
                led_fault_set(0);
            }
        }
        fault_led_tick = fault_led_tick + 1;
        if (fault_led_tick == 0x5a) {
            fault_led_tick = 0;
        }
    } else {
        fault_led_tick = 0;
        fault_led_on = 0;
        led_fault_set(0);
    }
}

/* one fault record (fault_log holds five, newest first; the other users treat fault_log as the
   bytes of an EEPROM block, so it stays a byte array and this is a view of it) */
typedef __packed struct {
    uchar code;  /* fault_code */
    ushort data; /* the DSP's fault data word */
    uchar year;
    uchar month;
    uchar date;
    uchar hour;
    uchar min;
    uchar sec;
} fault_rec_t;
#define LOG ((fault_rec_t *)fault_log)

/* Dead. Would push the current fault code, its data word and the time onto the five 9-byte fault
   records and mark block 2 for saving. */
void fault_log_push() {
    uchar i;

    for (i = 0; i < 4; i++) {
        LOG[4 - i].code = LOG[3 - i].code;
        LOG[4 - i].data = LOG[3 - i].data;
        LOG[4 - i].year = LOG[3 - i].year;
        LOG[4 - i].month = LOG[3 - i].month;
        LOG[4 - i].date = LOG[3 - i].date;
        LOG[4 - i].hour = LOG[3 - i].hour;
        LOG[4 - i].min = LOG[3 - i].min;
        LOG[4 - i].sec = LOG[3 - i].sec;
    }
    LOG[0].code = fault_code;
    if (fault_code == 0x1f) {
        LOG[0].data = dsp_fault_data2;
    } else {
        LOG[0].data = dsp_fault_data1;
    }
    LOG[0].year = rtc_time.year;
    LOG[0].month = rtc_time.month;
    LOG[0].date = rtc_time.date;
    LOG[0].hour = rtc_time.hour;
    LOG[0].min = rtc_time.min;
    LOG[0].sec = rtc_time.sec;
    eeprom_dirty = eeprom_dirty | 2;
}
#undef LOG

/* Dead. Would push the fault code and time onto the 50 five-byte event records (block 3). */
void event_log_push() {
    uchar i;

    for (i = 0; i < 0xf5; i++) {
        event_log[0xf9 - i] = event_log[0xf4 - i];
    }
    event_log[0] = fault_code;
    event_log[1] = rtc_time.month;
    event_log[2] = rtc_time.date;
    event_log[3] = rtc_time.hour;
    event_log[4] = rtc_time.min;
    eeprom_dirty = eeprom_dirty | 4;
}

/* Re-reads from the EEPROM the blocks (2, 3, 4, 6) that are marked dirty, and clears the mark of
   each block that now reads well. task_monitor calls it twice (pass 100 and 200) when the boot
   load failed, so a block that failed only for a moment is taken from the chip instead of being
   overwritten with defaults. Block 1 is not retried here. */
void eeprom_reload_flagged() {
    ulonglong load_result;

    if ((eeprom_dirty & 2) != 0) {
        os_flags_pend(0);
        load_result = eeprom_load(2, 5);
        if ((int)load_result == 3) {
            eeprom_dirty = eeprom_dirty & 0xfd;
        }
    }
    if ((eeprom_dirty & 4) != 0) {
        os_flags_pend(0);
        load_result = eeprom_load(3, 5);
        if ((int)load_result == 1) {
            eeprom_dirty = eeprom_dirty & 0xfb;
        }
    }
    if ((eeprom_dirty & 8) != 0) {
        os_flags_pend(0);
        load_result = eeprom_load(4, 5);
        if ((int)load_result == 1) {
            eeprom_dirty = eeprom_dirty & 0xf7;
        }
    }
    if ((eeprom_dirty & 0x20) != 0) {
        os_flags_pend(0);
        load_result = eeprom_load(6, 5);
        if ((int)load_result == 1) {
            eeprom_dirty = eeprom_dirty & 0xdf;
        }
    }
    return;
}

/* Calibration defaults: every halfword of block 2 to 0x400 (gain 1.0), then the offsets to 0.
   The word at +0x34 (bms_update_failed) is left at 0x400; bms_update_fail_flag_load only treats
   1 as "failed", so that does no harm. */
void calib_table_defaults() {
    int n;
    ushort *p;
    uchar i;

    n = 0x44;
    p = (ushort *)&cal_gains;
    for (i = 0; i < n / 2; i++) {
        p[i] = 0x400;
    }
    cal_gains.offset0 = 0;
    cal_gains.offset1 = 0;
    cal_gains.offset2 = 0;
    cal_gains.offset3 = 0;
    cal_gains.offset4 = 0;
    cal_gains.offset5 = 0;
    cal_gains.offset6 = 0;
    cal_gains.offset7 = 0;
    cal_gains.offset8 = 0;
    cal_gains.offset_26 = 0;
    cal_gains.offset_2A = 0;
    cal_gains.offset_2E = 0;
    cal_gains.offset_32 = 0;
    cal_gains.offset_36 = 0;
    cal_gains.offset11 = 0;
    cal_gains.offset9 = 0;
    cal_gains.offset10 = 0;
}

/* Heatsink temperature: scales the ADC reading by 30/33, converts it with a 14-piece linear
   table (the same in all three NTC functions; result in 0.1 degC, inference from the numbers)
   and averages 128 readings into heatsink_temp (IR41). */
void ntc_heatsink_temp_update() {
    short temp_value;

    adc_ntc_heatsink = ((adc_ntc_heatsink << 4) - adc_ntc_heatsink << 1) / 0x21;
    if (adc_ntc_heatsink < 0x170) {
        temp_value = 0x54c - ((uint)(adc_ntc_heatsink * 0xf7a) >> 0xc);
    } else {
        if (adc_ntc_heatsink <= 0x1da) {
            temp_value = 0x544 - ((uint)(adc_ntc_heatsink * 0xf1a) >> 0xc);
        } else {
            if (adc_ntc_heatsink < 0x266) {
                temp_value = 0x4da - ((uint)(adc_ntc_heatsink * 0xb85) >> 0xc);
            } else {
                if (adc_ntc_heatsink < 0x31c) {
                    temp_value = 0x471 - ((uint)(adc_ntc_heatsink * 0x8c9) >> 0xc);
                } else {
                    if (adc_ntc_heatsink < 0x3ff) {
                        temp_value =
                            0x419 -
                            ((uint)(adc_ntc_heatsink * 0x808 - adc_ntc_heatsink * 0x101) >> 0xc);
                    } else {
                        if (adc_ntc_heatsink < 0x510) {
                            temp_value = 0x3cf - ((uint)(adc_ntc_heatsink * 0x5dd) >> 0xc);
                        } else {
                            if (adc_ntc_heatsink < 0x642) {
                                temp_value = 0x39d - ((uint)(adc_ntc_heatsink * 0x53d) >> 0xc);
                            } else {
                                if (adc_ntc_heatsink < 0x77c) {
                                    temp_value = 0x38e - ((uint)(adc_ntc_heatsink * 0x518) >> 0xc);
                                } else {
                                    if (adc_ntc_heatsink < 0x8a7) {
                                        temp_value =
                                            0x3ac - ((uint)(adc_ntc_heatsink * 0x558) >> 0xc);
                                    } else {
                                        if (adc_ntc_heatsink < 0x9ae) {
                                            temp_value =
                                                0x412 - ((uint)(adc_ntc_heatsink * 0x615) >> 0xc);
                                        } else {
                                            if (adc_ntc_heatsink < 0xa85) {
                                                temp_value =
                                                    0x4e7 -
                                                    ((uint)(adc_ntc_heatsink * 0x776) >> 0xc);
                                            } else {
                                                if (adc_ntc_heatsink < 0xb1c) {
                                                    temp_value =
                                                        0x6f0 -
                                                        (short)((uint)(adc_ntc_heatsink * 0xa8e) >>
                                                                0xc);
                                                } else {
                                                    if (adc_ntc_heatsink < 0xb87) {
                                                        temp_value =
                                                            0xa09 -
                                                            ((uint)(adc_ntc_heatsink * 0xf04) >>
                                                             0xc);
                                                    } else {
                                                        temp_value =
                                                            0x1072 -
                                                            ((uint)(adc_ntc_heatsink * 0x17ea) >>
                                                             0xc);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ntc_heatsink_acc = ntc_heatsink_acc + temp_value;
    ntc_heatsink_cnt = ntc_heatsink_cnt + 1;
    if (ntc_heatsink_cnt == 0x80) {
        heatsink_temp = (short)(ntc_heatsink_acc >> 7);
        ntc_heatsink_cnt = 0;
        ntc_heatsink_acc = 0;
    }
}

/* Charger temperature, the same way (IR55). */
void ntc_charger_temp_update() {
    short temp_value;

    adc_ntc_charger = ((adc_ntc_charger << 4) - adc_ntc_charger << 1) / 0x21;
    if (adc_ntc_charger < 0x170) {
        temp_value = 0x54c - ((uint)(adc_ntc_charger * 0xf7a) >> 0xc);
    } else {
        if (adc_ntc_charger <= 0x1da) {
            temp_value = 0x544 - ((uint)(adc_ntc_charger * 0xf1a) >> 0xc);
        } else {
            if (adc_ntc_charger < 0x266) {
                temp_value = 0x4da - ((uint)(adc_ntc_charger * 0xb85) >> 0xc);
            } else {
                if (adc_ntc_charger < 0x31c) {
                    temp_value = 0x471 - ((uint)(adc_ntc_charger * 0x8c9) >> 0xc);
                } else {
                    if (adc_ntc_charger < 0x3ff) {
                        temp_value =
                            0x419 -
                            ((uint)(adc_ntc_charger * 0x808 - adc_ntc_charger * 0x101) >> 0xc);
                    } else {
                        if (adc_ntc_charger < 0x510) {
                            temp_value = 0x3cf - ((uint)(adc_ntc_charger * 0x5dd) >> 0xc);
                        } else {
                            if (adc_ntc_charger < 0x642) {
                                temp_value = 0x39d - ((uint)(adc_ntc_charger * 0x53d) >> 0xc);
                            } else {
                                if (adc_ntc_charger < 0x77c) {
                                    temp_value = 0x38e - ((uint)(adc_ntc_charger * 0x518) >> 0xc);
                                } else {
                                    if (adc_ntc_charger < 0x8a7) {
                                        temp_value =
                                            0x3ac - ((uint)(adc_ntc_charger * 0x558) >> 0xc);
                                    } else {
                                        if (adc_ntc_charger < 0x9ae) {
                                            temp_value =
                                                0x412 - ((uint)(adc_ntc_charger * 0x615) >> 0xc);
                                        } else {
                                            if (adc_ntc_charger < 0xa85) {
                                                temp_value =
                                                    0x4e7 -
                                                    ((uint)(adc_ntc_charger * 0x776) >> 0xc);
                                            } else {
                                                if (adc_ntc_charger < 0xb1c) {
                                                    temp_value =
                                                        0x6f0 -
                                                        (short)((uint)(adc_ntc_charger * 0xa8e) >>
                                                                0xc);
                                                } else {
                                                    if (adc_ntc_charger < 0xb87) {
                                                        temp_value =
                                                            0xa09 -
                                                            ((uint)(adc_ntc_charger * 0xf04) >>
                                                             0xc);
                                                    } else {
                                                        temp_value =
                                                            0x1072 -
                                                            ((uint)(adc_ntc_charger * 0x17ea) >>
                                                             0xc);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ntc_charger_acc = ntc_charger_acc + temp_value;
    ntc_charger_cnt = ntc_charger_cnt + 1;
    if (ntc_charger_cnt == 0x80) {
        charger_temp = (short)(ntc_charger_acc >> 7);
        ntc_charger_cnt = 0;
        ntc_charger_acc = 0;
    }
}

/* Empty; called between the two NTC updates. */
void ntc_stub() { return; }

/* Battery temperature from the NTC reading the aux MCU sends (lead acid only), the same table,
   128-reading average into batt_temp (IR56). */
void ntc_battery_temp_update() {
    short temp_value;

    if (adc_ntc_battery < 0x170) {
        temp_value = 0x54c - ((uint)(adc_ntc_battery * 0xf7a) >> 0xc);
    } else {
        if (adc_ntc_battery <= 0x1da) {
            temp_value = 0x544 - ((uint)(adc_ntc_battery * 0xf1a) >> 0xc);
        } else {
            if (adc_ntc_battery < 0x266) {
                temp_value = 0x4da - ((uint)(adc_ntc_battery * 0xb85) >> 0xc);
            } else {
                if (adc_ntc_battery < 0x31c) {
                    temp_value = 0x471 - ((uint)(adc_ntc_battery * 0x8c9) >> 0xc);
                } else {
                    if (adc_ntc_battery < 0x3ff) {
                        temp_value =
                            0x419 -
                            ((uint)(adc_ntc_battery * 0x808 - adc_ntc_battery * 0x101) >> 0xc);
                    } else {
                        if (adc_ntc_battery < 0x510) {
                            temp_value = 0x3cf - ((uint)(adc_ntc_battery * 0x5dd) >> 0xc);
                        } else {
                            if (adc_ntc_battery < 0x642) {
                                temp_value = 0x39d - ((uint)(adc_ntc_battery * 0x53d) >> 0xc);
                            } else {
                                if (adc_ntc_battery < 0x77c) {
                                    temp_value = 0x38e - ((uint)(adc_ntc_battery * 0x518) >> 0xc);
                                } else {
                                    if (adc_ntc_battery < 0x8a7) {
                                        temp_value =
                                            0x3ac - ((uint)(adc_ntc_battery * 0x558) >> 0xc);
                                    } else {
                                        if (adc_ntc_battery < 0x9ae) {
                                            temp_value =
                                                0x412 - ((uint)(adc_ntc_battery * 0x615) >> 0xc);
                                        } else {
                                            if (adc_ntc_battery < 0xa85) {
                                                temp_value =
                                                    0x4e7 -
                                                    ((uint)(adc_ntc_battery * 0x776) >> 0xc);
                                            } else {
                                                if (adc_ntc_battery < 0xb1c) {
                                                    temp_value =
                                                        0x6f0 -
                                                        (short)((uint)(adc_ntc_battery * 0xa8e) >>
                                                                0xc);
                                                } else {
                                                    if (adc_ntc_battery < 0xb87) {
                                                        temp_value =
                                                            0xa09 -
                                                            ((uint)(adc_ntc_battery * 0xf04) >>
                                                             0xc);
                                                    } else {
                                                        temp_value =
                                                            0x1072 -
                                                            ((uint)(adc_ntc_battery * 0x17ea) >>
                                                             0xc);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ntc_battery_acc = ntc_battery_acc + temp_value;
    ntc_battery_cnt = ntc_battery_cnt + 1;
    if (ntc_battery_cnt == 0x80) {
        batt_temp = (short)(ntc_battery_acc >> 7);
        ntc_battery_cnt = 0;
        ntc_battery_acc = 0;
    }
}

/* Lead-acid SOC (HR54 = 0) from the battery voltage, linear between HR97 and the top:

     SOC = (V - HR97) x 100 / (top - HR97), top = HR98 while charging (status 2 or 3), else
           96 % of HR98; capped at 97 while charging and 100 otherwise

   0 at or below HR97 or when the DSP's battery state is 3 or more; 100 when the DSP's "full"
   byte is 1. With HR97 at or above the top the division goes wrong (0 or a huge divisor). */
void batt_soc_from_voltage() {

    if (batt_state_dsp >= 3) {
        batt_soc = 0;
        return;
    }
    if (cfg_settings.battery_low_voltage_limit < batt_voltage) {
        if (charge_status == 2 || charge_status == 3) {
            batt_soc =
                (uint)((batt_voltage - cfg_settings.battery_low_voltage_limit) * 100) /
                (cfg_settings.battery_high_voltage_limit - cfg_settings.battery_low_voltage_limit);
        } else {
            batt_soc = (uint)((batt_voltage - cfg_settings.battery_low_voltage_limit) * 100) /
                       ((uint)(cfg_settings.battery_high_voltage_limit * 96) / 0x64 -
                        cfg_settings.battery_low_voltage_limit);
        }
    } else {
        batt_soc = 0;
    }
    if (charge_status == 2 || charge_status == 3) {
        if (batt_soc > 0x61) {
            batt_soc = 0x61;
        }
    } else {
        if (batt_soc > 0x64) {
            batt_soc = 0x64;
        }
    }
    if (batt_full_flag == 1) {
        batt_soc = 0x64;
    }
    return;
}

/* Turns the DRM inputs (2 bits per mode in drm_bits) into a power limit for AS/NZS 4777 demand
   response (inference from the 2/40/70/100 % steps and the Australian grid codes):

     grid code 3, 20, 21 with HR41: DRM0 from bits 8-9; then, while the inverter exports, the first
         mode at 1 among DRM1/2/3/4 (bits 0-1, 2-3, 4-5, 6-7) gives 2, 40, 70 or 100 %; while it
         imports the same with the bits at 0; DRM4 also sets HR50 to 100
     grid code 2, 8, 12 with HR41: DRM0 always on, and the limit from bits at 0

   With no mode the limit steps 2, 3, 4 and then 100. The callers use other country lists
   (stm32f10x_it.c: 2, 3, 8, 20, 21; dsp_link.c: 2, 3 for the cap), so code 12 is never evaluated
   (docs/findings.md). */
void drm_mode_eval() {
    uchar unused_mode;

    if ((grid_country == 3 || grid_country == 0x14 || grid_country == 0x15) &&
        cfg_settings.enable_drm_rj45 == 1) {
        if (((uint)((ushort)drm_bits) >> 8 & 3) == 0) {
            drm0_active[0] = 0;
        } else {
            drm0_active[0] = 1;
        }
        if (p_inverter_dsp > 0) {
            if ((drm_bits & 3) == 1) {
                drm_power_limit = 2;
            } else {
                if (((uint)drm_bits >> 2 & 3) == 1) {
                    drm_power_limit = 0x28;
                } else {
                    if (((uint)drm_bits >> 4 & 3) == 1) {
                        drm_power_limit = 0x46;
                    } else {
                        if (((uint)drm_bits >> 6 & 3) == 1) {
                            drm_power_limit = 0x64;
                            cfg_settings.active_power_rate = 0x64;
                        } else {
                            if (drm_power_limit < 4) {
                                drm_power_limit = drm_power_limit + 1;
                            } else {
                                drm_power_limit = 0x64;
                            }
                        }
                    }
                }
            }
        } else {
            if ((drm_bits & 3) == 0) {
                drm_power_limit = 2;
            } else {
                if (((uint)drm_bits >> 2 & 3) == 0) {
                    drm_power_limit = 0x28;
                } else {
                    if (((uint)drm_bits >> 4 & 3) == 0) {
                        drm_power_limit = 0x46;
                    } else {
                        if (((uint)drm_bits >> 6 & 3) == 0) {
                            drm_power_limit = 0x64;
                            cfg_settings.active_power_rate = 0x64;
                        } else {
                            if (drm_power_limit < 4) {
                                drm_power_limit = drm_power_limit + 1;
                            } else {
                                drm_power_limit = 0x64;
                            }
                        }
                    }
                }
            }
        }
    }
    if (grid_country == 2 || grid_country == 8 || grid_country == 0xc) {
        if (cfg_settings.enable_drm_rj45 == 1) {
            drm0_active[0] = 1;
            if ((drm_bits & 3) == 0) {
                drm_power_limit = 2;
            } else {
                if (((uint)drm_bits >> 2 & 3) == 0) {
                    drm_power_limit = 0x28;
                } else {
                    if (((uint)drm_bits >> 4 & 3) == 0) {
                        drm_power_limit = 0x46;
                    } else {
                        if (((uint)drm_bits >> 6 & 3) == 0) {
                            drm_power_limit = 0x64;
                        } else {
                            if (drm_power_limit < 4) {
                                drm_power_limit = drm_power_limit + 1;
                            } else {
                                drm_power_limit = 0x64;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Works out the power and energy figures the registers show:
   - without meters (HR7 = 0): grid power is the CT power, grid and inverter energies from the
     ARM's own counters, voltage and frequency from the DSP, current from the CT;
   - with meters: the grid power is meter 1 minus meters 5-8; on each finished meter cycle the
     grid energies come from meter 1 (day = reading - day start), PV energies from meters 2 and
     3, and absent meters are cleared; load = meter 1 + inverter + |meter 2| + |meter 3| +
     meter 4. Meter 1's current goes to f_ac1 (IR13, grid frequency) and its frequency to
     i_grid_port (IR58, grid current): the opposite of the names (docs/findings.md);
   - while the meter fault is set the meter values are zeroed;
   - on grid (sys_mode 1; IR49 says 1 is on grid, 0 off grid) the EPS output power is measured
     from ADC ch0 x ch7 and averaged over 50 windows, and the load is computed; off grid the EPS
     power is the inverter's. */
void power_flow_update() {
    volatile int unused_value;

    unused_value = 0;
    if (cfg_settings.enable_ammeter == 0) {
        batt_power_calc = ct_power;
        __aeabi_memclr((int)meter_data, sizeof(meter_data));
        meter_cycle_done = 0;
        e_inverter_in_day = energy_totals.inv_in_day;
        e_inverter_out_day = energy_totals.inv_out_day;
        e_inverter_in_total = energy_totals.inv_in_total;
        e_inverter_out_total = energy_totals.inv_out_total;
        e_grid_in_day = energy_totals.grid_in_day;
        e_grid_out_day = energy_totals.grid_out_day;
        e_grid_in_total = energy_totals.grid_in_total;
        e_grid_out_total = energy_totals.grid_out_total;
        v_ac1 = v_ac_dsp;
        f_ac1 = f_ac_dsp;
        i_grid_port = ct_i_rms;
        pf_inverter_output = pf_dsp;
        p_grid_apparent = p_apparent_dsp;
        p_load_demand = p_load_calc;
        if (dsp_page0_fresh == 1) {
            dsp_page0_fresh = 0;
            p_battery = p_battery_dsp;
            p_eps_backup = p_eps_avg;
            p_inverter_out = p_inverter_dsp;
            p_grid_out = ct_power;
        }
        __aeabi_memclr((int)&meter_data[1], sizeof(meter_t));
        __aeabi_memclr((int)meter_record_3, sizeof(meter_t));
        meter_power_fresh = 0;
        meter_bad_frequency_mask = 0;
    } else {
        if (cfg_settings.enable_ammeter & 1) {
            p_grid_meter_net = meter_data[0].p_active;
        }
        batt_power_calc = -p_grid_meter_net;
        if (meter_power_fresh == 1) {
            meter_power_fresh = 0;
            if (ct_cycle_count < 250) {
                ct_cycle_count++;
            } else {
                ct_cycle_count = 0;
            }
        }
        if (board_init_done == 1) {
            __aeabi_memclr((int)meter_data, sizeof(meter_data));
            meter_cycle_done = 0;
            p_grid_out = 0;
            v_ac1 = v_ac_dsp;
            i_grid_port = 0;
            p_grid_apparent = 0;
            pf_inverter_output = 0;
            f_ac1 = 0;
            meter_poll_step = 0;
            __aeabi_memclr((int)&meter_data[1], sizeof(meter_t));
            __aeabi_memclr((int)meter_record_3, sizeof(meter_t));
        } else if (meter_cycle_done == 1) {
            meter_cycle_done = 0;
            board_init_done = 0;
            e_inverter_in_day = energy_totals.inv_in_day;
            e_inverter_out_day = energy_totals.inv_out_day;
            e_inverter_in_total = energy_totals.inv_in_total;
            e_inverter_out_total = energy_totals.inv_out_total;
            if (cfg_settings.enable_ammeter & 1) {
                e_grid_in_day = meter_data[0].e_import - energy_totals.meter_in_day_start[0];
                if (e_grid_in_day < 0) {
                    e_grid_in_day = 0;
                    energy_totals.meter_in_day_start[0] = 0;
                }
                e_grid_out_day = meter_data[0].e_export - energy_totals.meter_out_day_start[0];
                if (e_grid_out_day < 0) {
                    e_grid_out_day = 0;
                    energy_totals.meter_out_day_start[0] = 0;
                }
                e_grid_in_total = meter_data[0].e_import;
                e_grid_out_total = meter_data[0].e_export;
                v_ac1 = meter_data[0].voltage;
                f_ac1 = meter_data[0].frequency;
                if (cfg_settings.meter_type == 2) {
                    i_grid_port = (short)meter_data[0].sum_current_x100;
                    pf_inverter_output = (short)meter_data[0].pf_total;
                    p_grid_apparent = meter_data[0].q_total;
                } else {
                    i_grid_port = (short)meter_data[0].current;
                    pf_inverter_output = (short)meter_data[0].pf;
                    p_grid_apparent = meter_data[0].q_reactive;
                }
            } else {
                __aeabi_memclr((int)meter_data, sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 2) == 2) {
                if (cfg_settings.meter_type == 1 || cfg_settings.meter_type == 2) {
                    e_pv_meter2_day = meter_data[1].e_import - energy_totals.meter_in_day_start[1];
                    if (e_pv_meter2_day < 0) {
                        e_pv_meter2_day = 0;
                        energy_totals.meter_in_day_start[1] = 0;
                    }
                    e_pv_meter2_total = meter_data[1].e_import;
                } else {
                    e_pv_meter2_day = meter_data[1].e_export - energy_totals.meter_out_day_start[1];
                    if (e_pv_meter2_day < 0) {
                        e_pv_meter2_day = 0;
                        energy_totals.meter_out_day_start[1] = 0;
                    }
                    e_pv_meter2_total = meter_data[1].e_export;
                }
            } else {
                __aeabi_memclr((int)&meter_data[1], sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 4) == 4) {
                if (cfg_settings.meter_type == 1 || cfg_settings.meter_type == 2) {
                    e_pv_meter3_day = meter_data[2].e_import - energy_totals.meter_in_day_start[2];
                    if (e_pv_meter3_day < 0) {
                        e_pv_meter3_day = 0;
                        energy_totals.meter_in_day_start[2] = 0;
                    }
                    e_pv_meter3_total = meter_data[2].e_import;
                } else {
                    e_pv_meter3_day = meter_data[2].e_export - energy_totals.meter_out_day_start[2];
                    if (e_pv_meter3_day < 0) {
                        e_pv_meter3_day = 0;
                        energy_totals.meter_out_day_start[2] = 0;
                    }
                    e_pv_meter3_total = meter_data[2].e_export;
                }
            } else {
                __aeabi_memclr((int)meter_record_3, sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 8) != 8) {
                __aeabi_memclr((int)(meter_record_3 + sizeof(meter_t)), sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 0x10) != 0x10) {
                __aeabi_memclr((int)&meter_records_5_6[0], sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 0x20) != 0x20) {
                __aeabi_memclr((int)&meter_records_5_6[1], sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 0x40) != 0x40) {
                __aeabi_memclr((int)&meter_records_7_8[0], sizeof(meter_t));
            }
            if ((cfg_settings.enable_ammeter & 0x80) != 0x80) {
                __aeabi_memclr((int)&meter_records_7_8[1], sizeof(meter_t));
            }
        }
        if (dsp_page0_fresh == 1) {
            dsp_page0_fresh = 0;
            p_battery = p_battery_dsp;
            p_eps_backup = p_eps_avg;
            p_inverter_out = p_inverter_dsp;
            p_grid_out = -meter_data[0].p_active;
            if (cfg_block6.meter_protocol_options == 0) {
                p_load_demand = meter_data[0].p_active + p_inverter_out +
                                abs(meter_data[1].p_active) + abs(meter_data[2].p_active) +
                                meter_data[3].p_active;
            } else {
                p_load_demand = meter_data[0].p_active + p_inverter_out;
            }
            if (p_load_demand < 0) {
                p_load_demand = 0;
            }
        }
    }
    i_rms_ch7 = isqrt_div(adc_cycle_sumsq_ch7, adc_cycle_n);
    if (sys_mode == 0) {
        batt_power_calc = 0;
        p_grid_out = 0;
        ct_i_rms = 0;
        p_eps_sample = p_inverter_dsp;
        p_eps_avg = p_inverter_dsp;
        load_power_estimate = p_inverter_dsp;
        p_load_calc = p_inverter_dsp;
        p_eps_avg_sum = 0;
        p_eps_avg_count = 0;
        p_inverter_out = p_inverter_dsp;
        p_battery = p_battery_dsp;
    } else if (sys_mode == 1) {
        p_eps_sample = adc_cycle_sumprod / adc_cycle_n / 100;
        if (p_eps_avg_count < 50) {
            p_eps_avg_count++;
            p_eps_avg_sum = p_eps_avg_sum + p_eps_sample;
        } else {
            p_eps_avg = p_eps_avg_sum / p_eps_avg_count;
            p_eps_avg_sum = 0;
            p_eps_avg_count = 0;
        }
        if (cfg_settings.enable_ammeter == 0) {
            if (p_inverter_out >= 0) {
                load_power_estimate = p_inverter_out - batt_power_calc;
            } else {
                load_power_estimate = p_inverter_out * 1031 / 1000 - batt_power_calc;
            }
            if (p_inverter_out >= 0) {
                p_load_calc = p_inverter_out - p_grid_out;
            } else {
                p_load_calc = p_inverter_out * 1031 / 1000 - p_grid_out;
            }
            load_power_unclamped = p_inverter_dsp - batt_power_calc;
        } else {
            load_power_estimate = meter_data[0].p_active + p_inverter_dsp;
            if (p_inverter_out >= 0) {
                p_load_calc = p_inverter_out - p_grid_out;
            } else {
                p_load_calc = p_inverter_out * 1031 / 1000 - p_grid_out;
            }
            load_power_unclamped = meter_data[0].p_active + p_inverter_dsp;
        }
    }
    if (p_eps_sample < 0)
        p_eps_sample = 0;
    if (p_eps_avg < 0)
        p_eps_avg = 0;
    if (load_power_estimate < 0)
        load_power_estimate = 0;
    if (load_power_estimate < p_eps_avg)
        load_power_estimate = p_eps_avg;
    if (p_eps_backup < 0)
        p_eps_backup = 0;
    if (p_load_calc < 0)
        p_load_calc = 0;
    if (p_load_calc < p_eps_backup)
        p_load_calc = p_eps_backup;
    if (dsp_p4_b02[0] == 1 || dsp_p4_b02[0] == 2 || dsp_p4_b02[0] == 5 || dsp_p4_b02[0] == 6) {
        fault_ac_readback = v_ac1;
    } else if (dsp_p4_b02[0] == 3 || dsp_p4_b02[0] == 4 || dsp_p4_b02[0] == 7 ||
               dsp_p4_b02[0] == 8) {
        fault_ac_readback = f_ac1;
    } else {
        fault_ac_readback = 0;
    }
}

/* Task 5 (priority 5), every 20 ticks:
   - LEDs, NTC temperatures;
   - the aux MCU link: a timeout of 1000 passes (10000 off-grid) sets the aux link fault; when a
     capture is complete it computes the CT power as sum(ADC ch0 x aux waveform) / 0x776 and the CT
     current as the RMS of the aux waveform x 10, and takes the battery NTC or the DRM word from the
     capture's last word;
   - the CT direction: with HR42 = 2 or 3 (automatic), CT power more than 5000 above the inverter
     output for 16 captures flips HR42 between 2 and 3; HR42 0 or 2 means "not reversed";
   - SOC and battery temperature: lithium (HR54 = 1) takes the DSP's SOC and temperature,
     lead acid computes them (both 0 at charge status 7);
   - the power flow, the fault LED (only with HR113 = 1), the fault code;
   - the EEPROM: when the boot load failed, retry the dirty blocks at pass 100 and 200, then save
     over them; otherwise, after the save delay, eeprom_save_dirty; after 6 failed saves the
     changes are dropped. A grid code or factory reset (cfg_changed_flags 1) saves blocks 1, 4
     and 6.
   - once the settings are loaded it gives the other tasks their periods (task 0 1000, 1 150,
     2 20, 3 1500, 4 1500, 6 500, 7 500 ticks for the first wake-up). */
void task_monitor() {
    uchar unused_flag;
    ushort i = 0;

    while (1) {
        led_status_update();
        ntc_heatsink_temp_update();
        ntc_stub();
        ntc_charger_temp_update();
        if (aux_link_timeout < 0x3e8 && sys_mode == 1 ||
            aux_link_timeout < 0x2710 && sys_mode == 0) {
            aux_link_timeout = aux_link_timeout + 1;
        } else {
            aux_link_timeout = 0;
            uart_rx_reset(0, 0x96);
            fault_set(FAULT_AUX_LINK);
        }
        if (charge_status == 7 && cfg_settings.battery_type == 0) {
            batt_soc = 0;
            batt_temp = 0;
        } else {
            if (cfg_settings.battery_type == 1) {
                batt_soc = batt_soc_dsp;
                batt_temp = (short)batt_temp_dsp;
            } else {
                batt_soc_from_voltage();
                if (ntc_battery_valid == 1) {
                    ntc_battery_temp_update();
                }
            }
        }
        if (sys_mode == 0 && adc_wave_idx >= 0x7d0) {
            if (aux_tx_idle_count < 0x32) {
                aux_tx_idle_count = aux_tx_idle_count + 1;
            } else {
                aux_tx_idle_count = 0;
                if (usb_reset_state == 1) {
                    USART_SendData(USART1_BASE, 0xab);
                } else {
                    USART_SendData(USART1_BASE, 0);
                }
            }
        }
        if (aux_tx_restart_count < 0xfa) {
            aux_tx_restart_count = aux_tx_restart_count + 1;
        } else {
            aux_tx_restart_count = 0;
            aux_tx_byte = 0x30;
            USART_SendData(USART1_BASE, aux_tx_byte);
            aux_pages_seen = 0;
            aux_tx_repeat_count = 0;
            adc_wave_idx = 0;
        }
        if (adc_wave_idx >= 0x3e8 && adc_wave_idx < 0x7d0) {
            if (aux_tx_last_byte == aux_tx_byte) {
                if (aux_tx_repeat_count < 0xa) {
                    aux_tx_repeat_count = aux_tx_repeat_count + 1;
                } else {
                    aux_tx_repeat_count = 0;
                    USART_SendData(USART1_BASE, aux_tx_byte);
                }
            } else {
                aux_tx_repeat_count = 0;
            }
            aux_tx_last_byte = aux_tx_byte;
        }
        if (aux_rx_state == 2) {
            aux_link_timeout = 0;
            fault_clear(FAULT_AUX_LINK);
            g_200000D0 = 0;
            aux_rx_state = 0;
            for (i = 0; i < 0xbf; i++) {
                ct_p_acc = adc_wave_buf[i] * aux_wave_buf[i] + ct_p_acc;
                ct_i2_acc = aux_wave_buf[i] * aux_wave_buf[i] + ct_i2_acc;
            }
            if (aux_rx_bit == 1) {
                if ((ushort)aux_wave_buf[191] < 0x8000) {
                    adc_ntc_battery = aux_wave_buf[191];
                    adc_ntc_battery = ((adc_ntc_battery << 4) - adc_ntc_battery << 1) / 0x21;
                    ntc_battery_valid = 1;
                } else {
                    *(ushort *)&drm_bits = aux_wave_buf[191];
                    if ((grid_country == 3 || grid_country == 0x14 || grid_country == 0x15 ||
                         grid_country == 8 || grid_country == 2) &&
                        cfg_settings.enable_drm_rj45 == 1) {
                        drm_mode_eval();
                    }
                }
            }
            ct_power = ct_p_acc / 0x776;
            ct_power = ct_power * ((0 - ct_reversed) * 2 + 1);
            if (ct_power - p_inverter_out > 0x1388) {
                if (ct_reverse_debounce < 0xf) {
                    ct_reverse_debounce = ct_reverse_debounce + 1;
                } else {
                    ct_reverse_debounce = 0;
                    if (cfg_settings.enable_reverse_ct_clamp > 1) {
                        if (cfg_settings.enable_reverse_ct_clamp == 2) {
                            cfg_settings.enable_reverse_ct_clamp = 3;
                        } else {
                            if (cfg_settings.enable_reverse_ct_clamp == 3) {
                                cfg_settings.enable_reverse_ct_clamp = 2;
                            }
                        }
                        eeprom_dirty = eeprom_dirty | 1;
                    }
                }
            } else {
                ct_reverse_debounce = 0;
            }
            if (cfg_settings.enable_reverse_ct_clamp == 0 ||
                cfg_settings.enable_reverse_ct_clamp == 2) {
                ct_reversed = 0;
            } else {
                ct_reversed = 1;
            }
            ct_i_rms = (short)(isqrt_div(ct_i2_acc, 0xbf) * 10);
            ct_p_acc = 0;
            ct_i2_acc = 0;
            adc_wave_idx = 0x7d0;
            if (ct_cycle_count < 0xfa) {
                ct_cycle_count = ct_cycle_count + 1;
            } else {
                ct_cycle_count = 0;
            }
        }
        power_flow_update();
        if (usb_sn_written == 1) {
            led_fault_set(1);
        } else {
            if (inv_run_mode != 4) {
                fault_led_armed = 1;
                if (cfg_settings.hr113 == 1) {
                    fault_led_update();
                } else {
                    led_fault_set(0);
                }
            } else {
                if (fault_led_armed == 1) {
                    fault_led_armed = 0;
                    led_fault_set(0);
                }
            }
        }
        if (eeprom_load_ok == 2) {
            fault_code_update();
            if (fault_code_settle < 0x2ee) {
                fault_code_settle = fault_code_settle + 1;
            }
            if (fault_code_settle == 0x2ee && fault_code > 0) {
                if (fault_code_prev != fault_code) {
                    fault_code_prev = fault_code;
                } else {
                    if ((fault_code == 0x1e || fault_code == 0x1f || fault_code == 0x19) &&
                        fault_recover_window == 1) {
                        fault_recover_window = 0;
                    }
                }
            }
            if (inv_run_mode == 1) {
                dsp_ignored_mode_byte = 2;
            } else {
                g_200001B2 = 0;
            }
        }
        if (inv_run_mode == 4) {
            eeprom_dirty = 0;
            save_req_pending = 0;
        } else {
            if (cfg_changed_flags == 1 && cfg_reset_mode == 0) {
                eeprom_dirty = eeprom_dirty | 0x29;
                save_req_pending = 1;
            }
        }
        if ((eeprom_dirty != 0 || save_req_pending == 1) && cfg_changed_flags != 3) {
            if (eeprom_load_failed == 1) {
                if (eeprom_reload_count <= 0xc8) {
                    eeprom_reload_count = eeprom_reload_count + 1;
                }
                if (eeprom_reload_count == 0x64 || eeprom_reload_count == 0xc8) {
                    eeprom_busy = 1;
                    eeprom_busy_2 = 1;
                    eeprom_reload_flagged();
                    eeprom_busy = 0;
                    eeprom_busy_2 = 0;
                }
                if (eeprom_dirty == 0) {
                    eeprom_load_failed = 0;
                    eeprom_reload_count = 0;
                    eeprom_load_ok = 1;
                }
                if (eeprom_reload_count == 0xc8) {
                    eeprom_load_failed = 0;
                    eeprom_reload_count = 0;
                    eeprom_load_ok = 1;
                }
            } else {
                if (eeprom_load_failed == 0 && save_req_pending == 1) {
                    if (eeprom_save_delay > 0) {
                        eeprom_save_delay = eeprom_save_delay - 1;
                    } else {
                        eeprom_busy = 1;
                        eeprom_busy_2 = 1;
                        eeprom_save_result = eeprom_save_dirty(eeprom_dirty);
                        if (eeprom_save_result == 0) {
                            eeprom_dirty = 0;
                            eeprom_save_retries = 0;
                            eeprom_save_result = 0;
                            eeprom_busy = 0;
                            eeprom_busy_2 = 0;
                            save_req_pending = 0;
                            if (eeprom_transaction_state == 1) {
                                eeprom_transaction_state = 2;
                            }
                            if (cfg_changed_flags & 1) {
                                os_task_delay((uchar)os_task_dly_idx, 0xbb8, 0x14);
                                cfg_changed_flags = cfg_changed_flags | 2;
                                settings_commit_flags = (settings_commit_flags & ~2) + 2;
                            }
                        } else {
                            eeprom_save_retries = eeprom_save_retries + 1;
                            if (eeprom_save_retries > 5) {
                                eeprom_busy = 0;
                                eeprom_busy_2 = 0;
                                if (eeprom_transaction_state == 1) {
                                    eeprom_transaction_state = 3;
                                }
                                eeprom_dirty = 0;
                                eeprom_save_retries = 0;
                                eeprom_save_result = 0;
                                save_req_pending = 0;
                            }
                        }
                    }
                }
            }
        } else {
            if (cfg_changed_flags == 1) {
                cfg_changed_flags = cfg_changed_flags | 2;
            }
        }
        if (eeprom_load_ok == 1) {
            eeprom_load_ok = 2;
            task_tbl[TASK_MEASURE].count = 0;
            task_tbl[TASK_MEASURE].period = 0x3e8;
            task_tbl[TASK_MODBUS].count = 0;
            task_tbl[TASK_MODBUS].period = 0x5dc;
            task_tbl[TASK_DSP_TX].count = 0;
            task_tbl[TASK_DSP_TX].period = 0x14;
            task_tbl[TASK_DSP_RX].count = 0;
            task_tbl[TASK_DSP_RX].period = 0x96;
            task_tbl[TASK_SCHEDULE].count = 0;
            task_tbl[TASK_SCHEDULE].period = 0x1f4;
            task_tbl[TASK_USB].count = 0;
            task_tbl[TASK_USB].period = 0x1f4;
            task_tbl[TASK_METER].count = 0;
            task_tbl[TASK_METER].period = 0x5dc;
        }
        task_tbl[TASK_MONITOR].count = 0;
        task_tbl[TASK_MONITOR].period = 0x14;
        os_flags_pend(0);
    }
}
