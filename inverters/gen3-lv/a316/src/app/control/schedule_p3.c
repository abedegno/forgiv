// clang-format off
/* at: crc32_table_init 0x080274A0, crc32_update 0x080274DE, fault_lowest_bit 0x08027506, energy_accumulate 0x0802752C, max3_u32 0x08027A80, sched_slot_soc_targets 0x08027A9E, sched_charge_slots_eval 0x0802887C, sched_discharge_slots_eval 0x08028E60, sched_halfhour_table_eval 0x08029460, batt_night_recovery_charge 0x08029548, status_outputs_update 0x0802965E, sys_tick_1s 0x08029712, sched_window_value 0x0802993E, sched_charge_discharge_eval 0x080299D0, batt_auto_judge_type 0x08029A3A, task_schedule 0x08029AA6 */
/* at: g_200000EC 0x200000EC, rtc_ready 0x200000ED, g_200000F4 0x200000F4, night_charge_active 0x200000F6, night_charge_qual 0x200000FA, g_200000FC 0x200000FC, g_200000FD 0x200000FD, g_200000FE 0x200000FE, g_20000100 0x20000100, g_20000101 0x20000101, g_20000102 0x20000102, g_20000103 0x20000103, g_20000104 0x20000104, g_20000106 0x20000106, g_20000107 0x20000107, g_20000108 0x20000108, g_2000010A 0x2000010A, eeprom_load_ok 0x200001A7, save_req_pending 0x200001A8, g_200001AA 0x200001AA, inv_run_mode 0x200001AE, g_200001B0 0x200001B0, p_apparent_dsp 0x200001C0, pv1_voltage 0x200001E0, pv2_voltage 0x200001E4, pv1_power 0x200001F4, pv2_power 0x200001F8, fault_code 0x2000023E, eeprom_dirty 0x20000260, p_inverter_dsp 0x2000026C, dsp_p2_d17 0x2000027C, fault_recover_window 0x20000288, eeprom_busy 0x2000028C, eeprom_busy_2 0x2000028D, g_200002B8 0x200002B8, usb_reset_state 0x200002BC, batt_power_calc 0x200002C8, sys_mode 0x200002D2, charge_status 0x200002D5, charge_active 0x200002D8, discharge_active 0x200002D9, hold_active 0x200002DA, batt_voltage 0x200002DC, p_battery_dsp 0x200002E0, batt_soc 0x200002E2, batt_soc_dsp 0x200002E3, usb_device_inserted 0x200002EE, usb_start_delay 0x20000306, usb_class_code 0x2000030A, rtc_set_pending 0x2000030C, usb_cdc_timer 0x20000310, sched_window_out 0x2000038C, dsp_p3_b19 0x20000398, discharge_min_soc_now 0x2000039E, charge_target_soc_now 0x2000039F, dsp_flash_mode 0x200003A4, dsp_p4_b16 0x200003A9, hr200_request 0x200003AB, bms_present 0x200003AE, force_state_hr329 0x200003C8, crc32_table 0x20000AB8, cfg_settings 0x20001AAE, energy_totals 0x20001BC5, cfg_block6 0x20001C82, cfg_hr360_table 0x20001F66, meter_data 0x20003C7A, task_tbl 0x20007C6C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* schedule_p3.c: task_schedule, the 500 ms task: the clock, the charge and discharge decisions
   the ARM sends to the DSP (time slots, the half-hour table mode, the calibration, the night
   recovery charge), the SOC targets, the energy counters and the panel indicators. Also the rest
   of the CRC-32 code and the integer helpers.

   Entry points: task_schedule (task 6); crc32_update (bms_fw_update_process); fault_lowest_bit
   (fault_code_update). Dead: crc32_table_init (so crc32_table stays all zero), max3_u32,
   batt_auto_judge_type.
   Data: its .data holds many of the shared application globals; the scheduler state
   night_charge_*, the counters g_200000F4..g_2000010A.
   Neighbours: schedule_p1.c before, usb/usb_bsp.c after; it writes charge_active,
   discharge_active, hold_active, charge_target_soc_now and discharge_min_soc_now, which
   comm/dsp_link.c sends to the DSP (frame bytes 4-6, pages 0, 4 and 7).

   Rules of the slot code: times are HHMM (hour x 100 + minute) compared with the RTC; a slot is
   active from its start to its end minute inclusive; start == end disables it; start > end wraps
   past midnight. There are 10 charge slots (slot 1 is HR94/95 with its SOC in HR242, then b1-b9
   in block 6) and 10 discharge slots (HR56/57, HR44/45 with HR272/HR275, then a1-a8). A charge
   always wins over a discharge.

   name: descriptive (map/filenames.tsv). See schedule_p1.c for why the two parts stay apart. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/battery.h"
#include "../../include/board.h"
#include "../../include/dsp_link.h"
#include "../../include/eeprom.h"
#include "../../include/energy.h"
#include "../../include/faults.h"
#include "../../include/measure.h"
#include "../../include/meter.h"
#include "../../include/modbus.h"
#include "../../include/rtc.h"
#include "../../include/schedule.h"
#include "../../include/settings.h"
#include "../../include/stdperiph.h"
#include "../../include/sys.h"
#include "../../include/tasks.h"
#include "../../include/usb.h"

/* Dead. Would build the 256-entry reflected CRC-32 table at crc32_table from a polynomial. With
   no caller the table stays zero, so crc32_update does not compute a CRC (docs/findings.md). */
void crc32_table_init(int polynomial) {
    int table_index;
    int remainder;
    int bit_index;

    polynomial = bit_reflect(polynomial, 0x20);
    for (table_index = 0; table_index <= 0xff; table_index++) {
        remainder = table_index;
        for (bit_index = 0; bit_index < 8; bit_index++) {
            if ((remainder & 1) != 0) {
                remainder = polynomial ^ (uint)remainder >> 1;
            } else {
                remainder = (uint)remainder >> 1;
            }
        }
        crc32_table[table_index] = remainder;
    }
}

/* Table-driven CRC-32 update over n bytes. With crc32_table all zero (see crc32_table_init)
   every step is crc >> 8, so after four bytes the value is 0 whatever the data. */
void crc32_update(uint *crc, byte *data, int length) {
    uint index = 0;

    for (index = 0; (int)index < length; index = index + 1 & 0xffff) {
        *crc = crc32_table[(byte)(*data++ ^ (byte)*crc)] ^ *crc >> 8;
    }
    return;
}

/* The number (1-32) of the lowest set bit of the fault flags, 0 when none is set. */
int fault_lowest_bit(int flags) {
    uchar bit_number;
    int bit_mask;

    if (flags == 0) {
        return 0;
    }
    bit_number = 1;
    bit_mask = 1;
    while (bit_number <= 0x20) {
        if ((flags & bit_mask) != 0) {
            return bit_number;
        }
        bit_number++;
        bit_mask = bit_mask << 1;
    }
    return 0;
}

/* Every 500 ms: integrates the powers into the energy counters of block 4. Each accumulator gets
   power / 2 per call and every 3,600,000 in it moves one unit into the day and total counters
   (0.1 kWh with powers in 0.1 W, inference). At a new RTC date the day counters are cleared and
   each meter's readings become the day's start values.
   - grid import and export from batt_power_calc (the CT or meter power) at all times;
   - in run mode 1 (or with dsp_p3_b19 = 1): PV1 and PV2 at 96 % of the DSP's PV power, the
     inverter in and out, battery charge and discharge (from the DSP's battery power x 10,
     positive = discharge), dsp_p2_d17 and g_200002B8, and the work time;
   - block 4 is saved every 2400 calls (20 min) while running, at a new day, and when the
     inverter stops after running more than 60 s.
   Day counters stop at 32700 (PV at 65530). */
void energy_accumulate() {
    uchar i = 0;

    if (energy_totals.last_day != rtc_time.date) {
        energy_totals.last_day = rtc_time.date;
        energy_totals.inv_out_day = 0;
        energy_totals.inv_in_day = 0;
        energy_totals.p2d17_day = 0;
        energy_totals.pv1_day = 0;
        energy_totals.pv2_day = 0;
        energy_totals.batt_charge_day = 0;
        energy_totals.batt_discharge_day = 0;
        energy_totals.grid_out_day = 0;
        energy_totals.grid_in_day = 0;
        energy_totals.calc6_day = 0;
        for (i = 0; i < 8; i++) {
            energy_totals.meter_in_day_start[i] = meter_data[i].e_import;
            energy_totals.meter_out_day_start[i] = meter_data[i].e_export;
        }
        g_200000EC[0] = 1;
    }
    if (batt_power_calc >= 0) {
        energy_totals.grid_out_acc = energy_totals.grid_out_acc + batt_power_calc / 2;
        if (energy_totals.grid_out_acc >= 3600000) {
            energy_totals.grid_out_day = energy_totals.grid_out_day + 1;
            if (energy_totals.grid_out_day >= 0x7fbc)
                energy_totals.grid_out_day = 0x7fbc;
            energy_totals.grid_out_total = energy_totals.grid_out_total + 1;
            energy_totals.grid_out_acc = energy_totals.grid_out_acc - 3600000;
        }
    } else {
        energy_totals.grid_in_acc = energy_totals.grid_in_acc + batt_power_calc / 2;
        if (energy_totals.grid_in_acc <= -3600000) {
            energy_totals.grid_in_day = energy_totals.grid_in_day + 1;
            if (energy_totals.grid_in_day >= 0x7fbc)
                energy_totals.grid_in_day = 0x7fbc;
            energy_totals.grid_in_total = energy_totals.grid_in_total + 1;
            energy_totals.grid_in_acc = energy_totals.grid_in_acc + 3600000;
        }
    }
    if (inv_run_mode == 1 || dsp_p3_b19[0] == 1) {
        g_200000F4 = g_200000F4 + 1;
        if (g_200000F4 > 2400) {
            eeprom_dirty = eeprom_dirty | 8;
            save_req_pending = 1;
            g_200000F4 = 0;
        }
        if (g_200000EC[0] == 1) {
            eeprom_dirty = eeprom_dirty | 8;
            g_200000EC[0] = 0;
        }
        energy_totals.pv1_acc = (uint)pv1_power * 96 / 200 + energy_totals.pv1_acc;
        if (energy_totals.pv1_acc >= 3600000) {
            energy_totals.pv1_day = energy_totals.pv1_day + 1;
            if (energy_totals.pv1_day >= 0xfffa)
                energy_totals.pv1_day = 0xfffa;
            energy_totals.pv1_total = energy_totals.pv1_total + 1;
            energy_totals.pv_total = energy_totals.pv_total + 1;
            energy_totals.pv1_acc = energy_totals.pv1_acc - 3600000;
        }
        energy_totals.pv2_acc = (uint)pv2_power * 96 / 200 + energy_totals.pv2_acc;
        if (energy_totals.pv2_acc >= 3600000) {
            energy_totals.pv2_day = energy_totals.pv2_day + 1;
            if (energy_totals.pv2_day >= 0xfffa)
                energy_totals.pv2_day = 0xfffa;
            energy_totals.pv2_total = energy_totals.pv2_total + 1;
            energy_totals.pv_total = energy_totals.pv_total + 1;
            energy_totals.pv2_acc = energy_totals.pv2_acc - 3600000;
        }
        if (dsp_p2_d17 > 0 || dsp_p2_d17 == 0) {
            energy_totals.p2d17_acc = energy_totals.p2d17_acc + dsp_p2_d17 / 2;
            if (energy_totals.p2d17_acc >= 3600000) {
                energy_totals.p2d17_day = energy_totals.p2d17_day + 1;
                if (energy_totals.p2d17_day >= 0xfffa)
                    energy_totals.p2d17_day = 0xfffa;
                energy_totals.p2d17_total = energy_totals.p2d17_total + 1;
                energy_totals.p2d17_acc = energy_totals.p2d17_acc - 3600000;
            }
        }
        if (p_inverter_dsp > 0 || p_inverter_dsp == 0) {
            energy_totals.inv_out_acc = energy_totals.inv_out_acc + p_inverter_dsp / 2;
            if (energy_totals.inv_out_acc >= 3600000) {
                energy_totals.inv_out_day = energy_totals.inv_out_day + 1;
                if (energy_totals.inv_out_day >= 0x7fbc)
                    energy_totals.inv_out_day = 0x7fbc;
                energy_totals.inv_out_total = energy_totals.inv_out_total + 1;
                energy_totals.inv_out_acc = energy_totals.inv_out_acc - 3600000;
            }
        } else {
            energy_totals.inv_in_acc = energy_totals.inv_in_acc + p_inverter_dsp / 2;
            if (energy_totals.inv_in_acc <= -3600000) {
                energy_totals.inv_in_day = energy_totals.inv_in_day + 1;
                if (energy_totals.inv_in_day >= 0x7fbc)
                    energy_totals.inv_in_day = 0x7fbc;
                energy_totals.inv_in_total = energy_totals.inv_in_total + 1;
                energy_totals.inv_in_acc = energy_totals.inv_in_acc + 3600000;
            }
        }
        if (p_battery_dsp >= 0) {
            energy_totals.batt_discharge_acc =
                energy_totals.batt_discharge_acc + p_battery_dsp * 10 / 2;
            if (energy_totals.batt_discharge_acc >= 3600000) {
                energy_totals.batt_discharge_day = energy_totals.batt_discharge_day + 1;
                if (energy_totals.batt_discharge_day >= 0x7fbc)
                    energy_totals.batt_discharge_day = 0x7fbc;
                energy_totals.batt_discharge_total = energy_totals.batt_discharge_total + 1;
                energy_totals.batt_discharge_acc = energy_totals.batt_discharge_acc - 3600000;
            }
        } else {
            energy_totals.batt_charge_acc = energy_totals.batt_charge_acc + p_battery_dsp * 10 / 2;
            if (energy_totals.batt_charge_acc <= -3600000) {
                energy_totals.batt_charge_day = energy_totals.batt_charge_day + 1;
                if (energy_totals.batt_charge_day >= 0x7fbc)
                    energy_totals.batt_charge_day = 0x7fbc;
                energy_totals.batt_charge_total = energy_totals.batt_charge_total + 1;
                energy_totals.batt_charge_acc = energy_totals.batt_charge_acc + 3600000;
            }
        }
        energy_totals.calc6_acc = energy_totals.calc6_acc + g_200002B8 / 2;
        if (energy_totals.calc6_acc >= 3600000) {
            energy_totals.calc6_day = energy_totals.calc6_day + 1;
            if (energy_totals.calc6_day >= 0x7fbc)
                energy_totals.calc6_day = 0x7fbc;
            energy_totals.calc6_total = energy_totals.calc6_total + 1;
            energy_totals.calc6_acc = energy_totals.calc6_acc - 3600000;
        }
        energy_totals.work_time = energy_totals.work_time + 1;
    } else {
        p_inverter_dsp = 0;
        p_apparent_dsp = 0;
        if (g_200000F4 > 120) {
            eeprom_dirty = eeprom_dirty | 8;
            save_req_pending = 1;
        }
        g_200000F4 = 0;
    }
}

/* Dead. The largest of three values, compared unsigned. */
int max3_u32(int first, int second, int third) {

    if ((uint)first >= second && (uint)first >= third) {
        return first;
    }
    if ((uint)second >= first && second >= third) {
        return second;
    }
    return third;
}

/* Sets the two SOC targets the DSP gets: charge_target_soc_now (page 7) is the SOC of the active
   charge slot (slot 1: HR242), else HR310; discharge_min_soc_now (page 4) is the SOC of the
   active discharge slot (slots 1, 2: HR272, HR275), else HR110. In remote mode (HR494 = 1) they
   are HR310 and HR114. When slots overlap the later one in the list wins. */
void sched_slot_soc_targets() {
    ushort t = t;
    uchar charge_hit1 = 0;
    uchar charge_hit2 = 0;
    uchar charge_hit3 = 0;
    uchar charge_hit4 = 0;
    uchar charge_hit5 = 0;
    uchar charge_hit6 = 0;
    uchar charge_hit7 = 0;
    uchar charge_hit8 = 0;
    uchar charge_hit9 = 0;
    uchar charge_hit10 = 0;
    uchar discharge_hit1 = 0;
    uchar discharge_hit2 = 0;
    uchar discharge_hit3 = 0;
    uchar discharge_hit4 = 0;
    uchar discharge_hit5 = 0;
    uchar discharge_hit6 = 0;
    uchar discharge_hit7 = 0;
    uchar discharge_hit8 = 0;
    uchar discharge_hit9 = 0;
    uchar discharge_hit10 = 0;

    t = rtc_time.min + rtc_time.hour * 100;
    if (cfg_block6.hr494 == 1) {
        charge_target_soc_now[0] = cfg_settings.hr310;
        discharge_min_soc_now = cfg_settings.battery_discharge_min_power_reserve;
        return;
    }
    if (cfg_settings.charge_slot_1_start != cfg_settings.charge_slot_1_end) {
        if (cfg_settings.charge_slot_1_start < cfg_settings.charge_slot_1_end &&
            cfg_settings.charge_slot_1_start <= t && cfg_settings.charge_slot_1_end >= t) {
            charge_target_soc_now[0] = cfg_block6.hr242;
            charge_hit1 = 1;
        } else if (cfg_settings.charge_slot_1_start > cfg_settings.charge_slot_1_end &&
                   (cfg_settings.charge_slot_1_start <= t && t <= 2400 ||
                    cfg_settings.charge_slot_1_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.hr242;
            charge_hit1 = 1;
        }
    }
    if (cfg_block6.slot_b1_start != cfg_block6.slot_b1_end) {
        if (cfg_block6.slot_b1_start < cfg_block6.slot_b1_end && cfg_block6.slot_b1_start <= t &&
            cfg_block6.slot_b1_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b1_soc;
            charge_hit2 = 1;
        } else if (cfg_block6.slot_b1_start > cfg_block6.slot_b1_end &&
                   (cfg_block6.slot_b1_start <= t && t <= 2400 || cfg_block6.slot_b1_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b1_soc;
            charge_hit2 = 1;
        }
    }
    if (cfg_block6.slot_b2_start != cfg_block6.slot_b2_end) {
        if (cfg_block6.slot_b2_start < cfg_block6.slot_b2_end && cfg_block6.slot_b2_start <= t &&
            cfg_block6.slot_b2_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b2_soc;
            charge_hit3 = 1;
        } else if (cfg_block6.slot_b2_start > cfg_block6.slot_b2_end &&
                   (cfg_block6.slot_b2_start <= t && t <= 2400 || cfg_block6.slot_b2_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b2_soc;
            charge_hit3 = 1;
        }
    }
    if (cfg_block6.slot_b3_start != cfg_block6.slot_b3_end) {
        if (cfg_block6.slot_b3_start < cfg_block6.slot_b3_end && cfg_block6.slot_b3_start <= t &&
            cfg_block6.slot_b3_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b3_soc;
            charge_hit4 = 1;
        } else if (cfg_block6.slot_b3_start > cfg_block6.slot_b3_end &&
                   (cfg_block6.slot_b3_start <= t && t <= 2400 || cfg_block6.slot_b3_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b3_soc;
            charge_hit4 = 1;
        }
    }
    if (cfg_block6.slot_b4_start != cfg_block6.slot_b4_end) {
        if (cfg_block6.slot_b4_start < cfg_block6.slot_b4_end && cfg_block6.slot_b4_start <= t &&
            cfg_block6.slot_b4_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b4_soc;
            charge_hit5 = 1;
        } else if (cfg_block6.slot_b4_start > cfg_block6.slot_b4_end &&
                   (cfg_block6.slot_b4_start <= t && t <= 2400 || cfg_block6.slot_b4_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b4_soc;
            charge_hit5 = 1;
        }
    }
    if (cfg_block6.slot_b5_start != cfg_block6.slot_b5_end) {
        if (cfg_block6.slot_b5_start < cfg_block6.slot_b5_end && cfg_block6.slot_b5_start <= t &&
            cfg_block6.slot_b5_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b5_soc;
            charge_hit6 = 1;
        } else if (cfg_block6.slot_b5_start > cfg_block6.slot_b5_end &&
                   (cfg_block6.slot_b5_start <= t && t <= 2400 || cfg_block6.slot_b5_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b5_soc;
            charge_hit6 = 1;
        }
    }
    if (cfg_block6.slot_b6_start != cfg_block6.slot_b6_end) {
        if (cfg_block6.slot_b6_start < cfg_block6.slot_b6_end && cfg_block6.slot_b6_start <= t &&
            cfg_block6.slot_b6_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b6_soc;
            charge_hit7 = 1;
        } else if (cfg_block6.slot_b6_start > cfg_block6.slot_b6_end &&
                   (cfg_block6.slot_b6_start <= t && t <= 2400 || cfg_block6.slot_b6_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b6_soc;
            charge_hit7 = 1;
        }
    }
    if (cfg_block6.slot_b7_start != cfg_block6.slot_b7_end) {
        if (cfg_block6.slot_b7_start < cfg_block6.slot_b7_end && cfg_block6.slot_b7_start <= t &&
            cfg_block6.slot_b7_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b7_soc;
            charge_hit8 = 1;
        } else if (cfg_block6.slot_b7_start > cfg_block6.slot_b7_end &&
                   (cfg_block6.slot_b7_start <= t && t <= 2400 || cfg_block6.slot_b7_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b7_soc;
            charge_hit8 = 1;
        }
    }
    if (cfg_block6.slot_b8_start != cfg_block6.slot_b8_end) {
        if (cfg_block6.slot_b8_start < cfg_block6.slot_b8_end && cfg_block6.slot_b8_start <= t &&
            cfg_block6.slot_b8_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b8_soc;
            charge_hit9 = 1;
        } else if (cfg_block6.slot_b8_start > cfg_block6.slot_b8_end &&
                   (cfg_block6.slot_b8_start <= t && t <= 2400 || cfg_block6.slot_b8_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b8_soc;
            charge_hit9 = 1;
        }
    }
    if (cfg_block6.slot_b9_start != cfg_block6.slot_b9_end) {
        if (cfg_block6.slot_b9_start < cfg_block6.slot_b9_end && cfg_block6.slot_b9_start <= t &&
            cfg_block6.slot_b9_end >= t) {
            charge_target_soc_now[0] = cfg_block6.slot_b9_soc;
            charge_hit10 = 1;
        } else if (cfg_block6.slot_b9_start > cfg_block6.slot_b9_end &&
                   (cfg_block6.slot_b9_start <= t && t <= 2400 || cfg_block6.slot_b9_end >= t)) {
            charge_target_soc_now[0] = cfg_block6.slot_b9_soc;
            charge_hit10 = 1;
        }
    }
    if (charge_hit1 == 0 && charge_hit2 == 0 && charge_hit3 == 0 && charge_hit4 == 0 &&
        charge_hit5 == 0 && charge_hit6 == 0 && charge_hit7 == 0 && charge_hit8 == 0 &&
        charge_hit9 == 0 && charge_hit10 == 0) {
        charge_target_soc_now[0] = cfg_settings.hr310;
    }
    if (cfg_settings.discharge_slot_1_start != cfg_settings.discharge_slot_1_end) {
        if (cfg_settings.discharge_slot_1_start < cfg_settings.discharge_slot_1_end &&
            cfg_settings.discharge_slot_1_start <= t && cfg_settings.discharge_slot_1_end >= t) {
            discharge_min_soc_now = cfg_block6.hr272;
            discharge_hit1 = 1;
        } else if (cfg_settings.discharge_slot_1_start > cfg_settings.discharge_slot_1_end &&
                   (cfg_settings.discharge_slot_1_start <= t && t <= 2400 ||
                    cfg_settings.discharge_slot_1_end >= t)) {
            discharge_min_soc_now = cfg_block6.hr272;
            discharge_hit1 = 1;
        }
    }
    if (cfg_settings.discharge_slot_2_start != cfg_settings.discharge_slot_2_end) {
        if (cfg_settings.discharge_slot_2_start < cfg_settings.discharge_slot_2_end &&
            cfg_settings.discharge_slot_2_start <= t && cfg_settings.discharge_slot_2_end >= t) {
            discharge_min_soc_now = cfg_block6.hr275;
            discharge_hit2 = 1;
        } else if (cfg_settings.discharge_slot_2_start > cfg_settings.discharge_slot_2_end &&
                   (cfg_settings.discharge_slot_2_start <= t && t <= 2400 ||
                    cfg_settings.discharge_slot_2_end >= t)) {
            discharge_min_soc_now = cfg_block6.hr275;
            discharge_hit2 = 1;
        }
    }
    if (cfg_block6.slot_a1_start != cfg_block6.slot_a1_end) {
        if (cfg_block6.slot_a1_start < cfg_block6.slot_a1_end && cfg_block6.slot_a1_start <= t &&
            cfg_block6.slot_a1_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a1_soc;
            discharge_hit3 = 1;
        } else if (cfg_block6.slot_a1_start > cfg_block6.slot_a1_end &&
                   (cfg_block6.slot_a1_start <= t && t <= 2400 || cfg_block6.slot_a1_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a1_soc;
            discharge_hit3 = 1;
        }
    }
    if (cfg_block6.slot_a2_start != cfg_block6.slot_a2_end) {
        if (cfg_block6.slot_a2_start < cfg_block6.slot_a2_end && cfg_block6.slot_a2_start <= t &&
            cfg_block6.slot_a2_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a2_soc;
            discharge_hit4 = 1;
        } else if (cfg_block6.slot_a2_start > cfg_block6.slot_a2_end &&
                   (cfg_block6.slot_a2_start <= t && t <= 2400 || cfg_block6.slot_a2_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a2_soc;
            discharge_hit4 = 1;
        }
    }
    if (cfg_block6.slot_a3_start != cfg_block6.slot_a3_end) {
        if (cfg_block6.slot_a3_start < cfg_block6.slot_a3_end && cfg_block6.slot_a3_start <= t &&
            cfg_block6.slot_a3_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a3_soc;
            discharge_hit5 = 1;
        } else if (cfg_block6.slot_a3_start > cfg_block6.slot_a3_end &&
                   (cfg_block6.slot_a3_start <= t && t <= 2400 || cfg_block6.slot_a3_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a3_soc;
            discharge_hit5 = 1;
        }
    }
    if (cfg_block6.slot_a4_start != cfg_block6.slot_a4_end) {
        if (cfg_block6.slot_a4_start < cfg_block6.slot_a4_end && cfg_block6.slot_a4_start <= t &&
            cfg_block6.slot_a4_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a4_soc;
            discharge_hit6 = 1;
        } else if (cfg_block6.slot_a4_start > cfg_block6.slot_a4_end &&
                   (cfg_block6.slot_a4_start <= t && t <= 2400 || cfg_block6.slot_a4_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a4_soc;
            discharge_hit6 = 1;
        }
    }
    if (cfg_block6.slot_a5_start != cfg_block6.slot_a5_end) {
        if (cfg_block6.slot_a5_start < cfg_block6.slot_a5_end && cfg_block6.slot_a5_start <= t &&
            cfg_block6.slot_a5_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a5_soc;
            discharge_hit7 = 1;
        } else if (cfg_block6.slot_a5_start > cfg_block6.slot_a5_end &&
                   (cfg_block6.slot_a5_start <= t && t <= 2400 || cfg_block6.slot_a5_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a5_soc;
            discharge_hit7 = 1;
        }
    }
    if (cfg_block6.slot_a6_start != cfg_block6.slot_a6_end) {
        if (cfg_block6.slot_a6_start < cfg_block6.slot_a6_end && cfg_block6.slot_a6_start <= t &&
            cfg_block6.slot_a6_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a6_soc;
            discharge_hit8 = 1;
        } else if (cfg_block6.slot_a6_start > cfg_block6.slot_a6_end &&
                   (cfg_block6.slot_a6_start <= t && t <= 2400 || cfg_block6.slot_a6_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a6_soc;
            discharge_hit8 = 1;
        }
    }
    if (cfg_block6.slot_a7_start != cfg_block6.slot_a7_end) {
        if (cfg_block6.slot_a7_start < cfg_block6.slot_a7_end && cfg_block6.slot_a7_start <= t &&
            cfg_block6.slot_a7_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a7_soc;
            discharge_hit9 = 1;
        } else if (cfg_block6.slot_a7_start > cfg_block6.slot_a7_end &&
                   (cfg_block6.slot_a7_start <= t && t <= 2400 || cfg_block6.slot_a7_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a7_soc;
            discharge_hit9 = 1;
        }
    }
    if (cfg_block6.slot_a8_start != cfg_block6.slot_a8_end) {
        if (cfg_block6.slot_a8_start < cfg_block6.slot_a8_end && cfg_block6.slot_a8_start <= t &&
            cfg_block6.slot_a8_end >= t) {
            discharge_min_soc_now = cfg_block6.slot_a8_soc;
            discharge_hit10 = 1;
        } else if (cfg_block6.slot_a8_start > cfg_block6.slot_a8_end &&
                   (cfg_block6.slot_a8_start <= t && t <= 2400 || cfg_block6.slot_a8_end >= t)) {
            discharge_min_soc_now = cfg_block6.slot_a8_soc;
            discharge_hit10 = 1;
        }
    }
    if (discharge_hit1 == 0 && discharge_hit2 == 0 && discharge_hit3 == 0 && discharge_hit4 == 0 &&
        discharge_hit5 == 0 && discharge_hit6 == 0 && discharge_hit7 == 0 && discharge_hit8 == 0 &&
        discharge_hit9 == 0 && discharge_hit10 == 0) {
        discharge_min_soc_now = cfg_settings.battery_soc_reserve;
    }
}

/* charge_active = 1 while any of the 10 charge slots is active, with HR96 = 1 and on grid
   (sys_mode 1). */
void sched_charge_slots_eval() {
    int slot1_active;
    int slot2_active;
    int slot3_active;
    int slot4_active;
    int slot5_active;
    int slot6_active;
    int slot7_active;
    int slot8_active;
    int slot9_active;
    int slot10_active;
    uchar unused_slot;
    int time_hhmm = 0;

    slot1_active = 0;
    slot2_active = 0;
    slot3_active = 0;
    slot4_active = 0;
    slot5_active = 0;
    slot6_active = 0;
    slot7_active = 0;
    slot8_active = 0;
    slot9_active = 0;
    slot10_active = 0;
    if (cfg_settings.enable_charge == 0 || sys_mode == 0) {
        charge_active = 0;
        return;
    }
    time_hhmm = rtc_time.min + rtc_time.hour * 100;
    if (cfg_settings.charge_slot_1_start <= cfg_settings.charge_slot_1_end) {
        if (cfg_settings.charge_slot_1_start <= time_hhmm &&
            cfg_settings.charge_slot_1_end >= time_hhmm &&
            cfg_settings.charge_slot_1_start != cfg_settings.charge_slot_1_end) {
            charge_active = 1;
            slot1_active = 1;
        } else {
            slot1_active = 0;
        }
    } else {
        if (cfg_settings.charge_slot_1_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_settings.charge_slot_1_end >= time_hhmm) {
            charge_active = 1;
            slot1_active = 1;
        } else {
            slot1_active = 0;
        }
    }
    if (cfg_block6.slot_b1_start <= cfg_block6.slot_b1_end) {
        if (cfg_block6.slot_b1_start <= time_hhmm && cfg_block6.slot_b1_end >= time_hhmm &&
            cfg_block6.slot_b1_start != cfg_block6.slot_b1_end) {
            charge_active = 1;
            slot2_active = 1;
        } else {
            slot2_active = 0;
        }
    } else {
        if (cfg_block6.slot_b1_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b1_end >= time_hhmm) {
            charge_active = 1;
            slot2_active = 1;
        } else {
            slot2_active = 0;
        }
    }
    if (cfg_block6.slot_b2_start <= cfg_block6.slot_b2_end) {
        if (cfg_block6.slot_b2_start <= time_hhmm && cfg_block6.slot_b2_end >= time_hhmm &&
            cfg_block6.slot_b2_start != cfg_block6.slot_b2_end) {
            charge_active = 1;
            slot3_active = 1;
        } else {
            slot3_active = 0;
        }
    } else {
        if (cfg_block6.slot_b2_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b2_end >= time_hhmm) {
            charge_active = 1;
            slot3_active = 1;
        } else {
            slot3_active = 0;
        }
    }
    if (cfg_block6.slot_b3_start <= cfg_block6.slot_b3_end) {
        if (cfg_block6.slot_b3_start <= time_hhmm && cfg_block6.slot_b3_end >= time_hhmm &&
            cfg_block6.slot_b3_start != cfg_block6.slot_b3_end) {
            charge_active = 1;
            slot4_active = 1;
        } else {
            slot4_active = 0;
        }
    } else {
        if (cfg_block6.slot_b3_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b3_end >= time_hhmm) {
            charge_active = 1;
            slot4_active = 1;
        } else {
            slot4_active = 0;
        }
    }
    if (cfg_block6.slot_b4_start <= cfg_block6.slot_b4_end) {
        if (cfg_block6.slot_b4_start <= time_hhmm && cfg_block6.slot_b4_end >= time_hhmm &&
            cfg_block6.slot_b4_start != cfg_block6.slot_b4_end) {
            charge_active = 1;
            slot5_active = 1;
        } else {
            slot5_active = 0;
        }
    } else {
        if (cfg_block6.slot_b4_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b4_end >= time_hhmm) {
            charge_active = 1;
            slot5_active = 1;
        } else {
            slot5_active = 0;
        }
    }
    if (cfg_block6.slot_b5_start <= cfg_block6.slot_b5_end) {
        if (cfg_block6.slot_b5_start <= time_hhmm && cfg_block6.slot_b5_end >= time_hhmm &&
            cfg_block6.slot_b5_start != cfg_block6.slot_b5_end) {
            charge_active = 1;
            slot6_active = 1;
        } else {
            slot6_active = 0;
        }
    } else {
        if (cfg_block6.slot_b5_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b5_end >= time_hhmm) {
            charge_active = 1;
            slot6_active = 1;
        } else {
            slot6_active = 0;
        }
    }
    if (cfg_block6.slot_b6_start <= cfg_block6.slot_b6_end) {
        if (cfg_block6.slot_b6_start <= time_hhmm && cfg_block6.slot_b6_end >= time_hhmm &&
            cfg_block6.slot_b6_start != cfg_block6.slot_b6_end) {
            charge_active = 1;
            slot7_active = 1;
        } else {
            slot7_active = 0;
        }
    } else {
        if (cfg_block6.slot_b6_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b6_end >= time_hhmm) {
            charge_active = 1;
            slot7_active = 1;
        } else {
            slot7_active = 0;
        }
    }
    if (cfg_block6.slot_b7_start <= cfg_block6.slot_b7_end) {
        if (cfg_block6.slot_b7_start <= time_hhmm && cfg_block6.slot_b7_end >= time_hhmm &&
            cfg_block6.slot_b7_start != cfg_block6.slot_b7_end) {
            charge_active = 1;
            slot8_active = 1;
        } else {
            slot8_active = 0;
        }
    } else {
        if (cfg_block6.slot_b7_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b7_end >= time_hhmm) {
            charge_active = 1;
            slot8_active = 1;
        } else {
            slot8_active = 0;
        }
    }
    if (cfg_block6.slot_b8_start <= cfg_block6.slot_b8_end) {
        if (cfg_block6.slot_b8_start <= time_hhmm && cfg_block6.slot_b8_end >= time_hhmm &&
            cfg_block6.slot_b8_start != cfg_block6.slot_b8_end) {
            charge_active = 1;
            slot9_active = 1;
        } else {
            slot9_active = 0;
        }
    } else {
        if (cfg_block6.slot_b8_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b8_end >= time_hhmm) {
            charge_active = 1;
            slot9_active = 1;
        } else {
            slot9_active = 0;
        }
    }
    if (cfg_block6.slot_b9_start <= cfg_block6.slot_b9_end) {
        if (cfg_block6.slot_b9_start <= time_hhmm && cfg_block6.slot_b9_end >= time_hhmm &&
            cfg_block6.slot_b9_start != cfg_block6.slot_b9_end) {
            charge_active = 1;
            slot10_active = 1;
        } else {
            slot10_active = 0;
        }
    } else {
        if (cfg_block6.slot_b9_start <= time_hhmm && time_hhmm <= 0x960 ||
            cfg_block6.slot_b9_end >= time_hhmm) {
            charge_active = 1;
            slot10_active = 1;
        } else {
            slot10_active = 0;
        }
    }
    if (slot1_active == 0 && slot2_active == 0 && slot3_active == 0 && slot4_active == 0 &&
        slot5_active == 0 && slot6_active == 0 && slot7_active == 0 && slot8_active == 0 &&
        slot9_active == 0 && slot10_active == 0) {
        charge_active = 0;
    }
    return;
}

/* discharge_active = 1 while any of the 10 discharge slots is active, with HR59 = 1, on grid, no
   charge active, and not (HR110 = 100 with HR27 = 1). */
void sched_discharge_slots_eval() {
    int slot1_active;
    int slot2_active;
    int slot3_active;
    int slot4_active;
    int slot5_active;
    int slot6_active;
    int slot7_active;
    int slot8_active;
    int slot9_active;
    int slot10_active;
    uchar unused_slot;
    int time_hhmm = 0;

    slot1_active = 0;
    slot2_active = 0;
    slot3_active = 0;
    slot4_active = 0;
    slot5_active = 0;
    slot6_active = 0;
    slot7_active = 0;
    slot8_active = 0;
    slot9_active = 0;
    slot10_active = 0;
    if (cfg_settings.enable_discharge == 0 || sys_mode == 0 || charge_active == 1) {
        discharge_active = 0;
        return;
    }
    time_hhmm = rtc_time.min + rtc_time.hour * 100;
    if (cfg_settings.enable_discharge == 1 &&
        (cfg_settings.battery_soc_reserve != 0x64 || cfg_settings.battery_power_mode != 1)) {
        if (cfg_settings.discharge_slot_1_start <= cfg_settings.discharge_slot_1_end) {
            if (cfg_settings.discharge_slot_1_start <= time_hhmm &&
                cfg_settings.discharge_slot_1_end >= time_hhmm &&
                cfg_settings.discharge_slot_1_start != cfg_settings.discharge_slot_1_end) {
                discharge_active = 1;
                slot1_active = 1;
            } else {
                slot1_active = 0;
            }
        } else {
            if (cfg_settings.discharge_slot_1_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_settings.discharge_slot_1_end >= time_hhmm) {
                discharge_active = 1;
                slot1_active = 1;
            } else {
                slot1_active = 0;
            }
        }
        if (cfg_settings.discharge_slot_2_start <= cfg_settings.discharge_slot_2_end) {
            if (cfg_settings.discharge_slot_2_start <= time_hhmm &&
                cfg_settings.discharge_slot_2_end >= time_hhmm &&
                cfg_settings.discharge_slot_2_start != cfg_settings.discharge_slot_2_end) {
                discharge_active = 1;
                slot2_active = 1;
            } else {
                slot2_active = 0;
            }
        } else {
            if (cfg_settings.discharge_slot_2_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_settings.discharge_slot_2_end >= time_hhmm) {
                discharge_active = 1;
                slot2_active = 1;
            } else {
                slot2_active = 0;
            }
        }
        if (cfg_block6.slot_a1_start <= cfg_block6.slot_a1_end) {
            if (cfg_block6.slot_a1_start <= time_hhmm && cfg_block6.slot_a1_end >= time_hhmm &&
                cfg_block6.slot_a1_start != cfg_block6.slot_a1_end) {
                discharge_active = 1;
                slot3_active = 1;
            } else {
                slot3_active = 0;
            }
        } else {
            if (cfg_block6.slot_a1_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a1_end >= time_hhmm) {
                discharge_active = 1;
                slot3_active = 1;
            } else {
                slot3_active = 0;
            }
        }
        if (cfg_block6.slot_a2_start <= cfg_block6.slot_a2_end) {
            if (cfg_block6.slot_a2_start <= time_hhmm && cfg_block6.slot_a2_end >= time_hhmm &&
                cfg_block6.slot_a2_start != cfg_block6.slot_a2_end) {
                discharge_active = 1;
                slot4_active = 1;
            } else {
                slot4_active = 0;
            }
        } else {
            if (cfg_block6.slot_a2_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a2_end >= time_hhmm) {
                discharge_active = 1;
                slot4_active = 1;
            } else {
                slot4_active = 0;
            }
        }
        if (cfg_block6.slot_a3_start <= cfg_block6.slot_a3_end) {
            if (cfg_block6.slot_a3_start <= time_hhmm && cfg_block6.slot_a3_end >= time_hhmm &&
                cfg_block6.slot_a3_start != cfg_block6.slot_a3_end) {
                discharge_active = 1;
                slot5_active = 1;
            } else {
                slot5_active = 0;
            }
        } else {
            if (cfg_block6.slot_a3_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a3_end >= time_hhmm) {
                discharge_active = 1;
                slot5_active = 1;
            } else {
                slot5_active = 0;
            }
        }
        if (cfg_block6.slot_a4_start <= cfg_block6.slot_a4_end) {
            if (cfg_block6.slot_a4_start <= time_hhmm && cfg_block6.slot_a4_end >= time_hhmm &&
                cfg_block6.slot_a4_start != cfg_block6.slot_a4_end) {
                discharge_active = 1;
                slot6_active = 1;
            } else {
                slot6_active = 0;
            }
        } else {
            if (cfg_block6.slot_a4_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a4_end >= time_hhmm) {
                discharge_active = 1;
                slot6_active = 1;
            } else {
                slot6_active = 0;
            }
        }
        if (cfg_block6.slot_a5_start <= cfg_block6.slot_a5_end) {
            if (cfg_block6.slot_a5_start <= time_hhmm && cfg_block6.slot_a5_end >= time_hhmm &&
                cfg_block6.slot_a5_start != cfg_block6.slot_a5_end) {
                discharge_active = 1;
                slot7_active = 1;
            } else {
                slot7_active = 0;
            }
        } else {
            if (cfg_block6.slot_a5_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a5_end >= time_hhmm) {
                discharge_active = 1;
                slot7_active = 1;
            } else {
                slot7_active = 0;
            }
        }
        if (cfg_block6.slot_a6_start <= cfg_block6.slot_a6_end) {
            if (cfg_block6.slot_a6_start <= time_hhmm && cfg_block6.slot_a6_end >= time_hhmm &&
                cfg_block6.slot_a6_start != cfg_block6.slot_a6_end) {
                discharge_active = 1;
                slot8_active = 1;
            } else {
                slot8_active = 0;
            }
        } else {
            if (cfg_block6.slot_a6_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a6_end >= time_hhmm) {
                discharge_active = 1;
                slot8_active = 1;
            } else {
                slot8_active = 0;
            }
        }
        if (cfg_block6.slot_a7_start <= cfg_block6.slot_a7_end) {
            if (cfg_block6.slot_a7_start <= time_hhmm && cfg_block6.slot_a7_end >= time_hhmm &&
                cfg_block6.slot_a7_start != cfg_block6.slot_a7_end) {
                discharge_active = 1;
                slot9_active = 1;
            } else {
                slot9_active = 0;
            }
        } else {
            if (cfg_block6.slot_a7_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a7_end >= time_hhmm) {
                discharge_active = 1;
                slot9_active = 1;
            } else {
                slot9_active = 0;
            }
        }
        if (cfg_block6.slot_a8_start <= cfg_block6.slot_a8_end) {
            if (cfg_block6.slot_a8_start <= time_hhmm && cfg_block6.slot_a8_end >= time_hhmm &&
                cfg_block6.slot_a8_start != cfg_block6.slot_a8_end) {
                discharge_active = 1;
                slot10_active = 1;
            } else {
                slot10_active = 0;
            }
        } else {
            if (cfg_block6.slot_a8_start <= time_hhmm && time_hhmm <= 0x960 ||
                cfg_block6.slot_a8_end >= time_hhmm) {
                discharge_active = 1;
                slot10_active = 1;
            } else {
                slot10_active = 0;
            }
        }
    }
    if (slot1_active == 0 && slot2_active == 0 && slot3_active == 0 && slot4_active == 0 &&
        slot5_active == 0 && slot6_active == 0 && slot7_active == 0 && slot8_active == 0 &&
        slot9_active == 0 && slot10_active == 0) {
        discharge_active = 0;
    }
    return;
}

/* The table mode (HR322 = 1): for the current half hour (index 0-47) it reads a from the first
   table (HR420-467) and b from the second (HR360-407), a value over 10000 meaning 10000 - value.
   It discharges when a is above both HR325 and HR323, else charges when b is below HR324, else
   holds (hold_active, which makes the DSP get HR27 = 1). Inference: a price table, with a the
   export and b the import price. Neither table is saved, so after a restart both read 0 and with
   HR324 > 0 every half hour charges until they are written again (docs/findings.md). */
void sched_halfhour_table_eval() {
    ushort idx = idx;
    int t = t;
    short a = 0;
    short b = 0;

    t = rtc_time.min + rtc_time.hour * 100;
    idx = t % 100 / 30 + t / 100 * 2;
    if (idx >= 48) {
        return;
    }
    if (cfg_hr360_table[0][idx] > 10000) {
        a = 10000 - cfg_hr360_table[0][idx];
    } else {
        a = cfg_hr360_table[0][idx];
    }
    if (cfg_hr360_table[1][idx] > 10000) {
        b = 10000 - cfg_hr360_table[1][idx];
    } else {
        b = cfg_hr360_table[1][idx];
    }
    if ((short)cfg_block6.hr325 < a && (short)cfg_block6.hr323 < a) {
        hold_active = 0;
        discharge_active = 1;
        charge_active = 0;
    } else if ((short)cfg_block6.hr324 > b) {
        hold_active = 0;
        discharge_active = 0;
        charge_active = 1;
    } else {
        hold_active = 1;
        discharge_active = 0;
        charge_active = 0;
    }
}

/* Night recovery charge, between 01:00 and 04:59 on grid with no charge active. It starts after
   60 passes (30 s) of a flat battery: lithium with SOC 0 or SOC + 10 <= HR114, lead acid with V
   < HR97 - 5.00 V. It then sets charge_active for HR108 x 120 passes (HR108 minutes); for
   lithium it stops as soon as SOC >= HR114. Once it has run it does not start again before the
   next night. */
void batt_night_recovery_charge() {
    if (sys_mode == 0) {
        return;
    }
    if (charge_active == 0 && rtc_time.hour >= 1 && rtc_time.hour <= 4) {
        if (night_charge_active == 0) {
            if ((cfg_settings.battery_low_voltage_limit > batt_voltage + 500 &&
                     cfg_settings.battery_type == 0 ||
                 (batt_soc == 0 ||
                  cfg_settings.battery_discharge_min_power_reserve >= batt_soc + 10) &&
                     cfg_settings.battery_type == 1) &&
                night_charge_timer == 0) {
                if (night_charge_qual < 60) {
                    night_charge_qual = night_charge_qual + 1;
                } else {
                    night_charge_qual = 0;
                    night_charge_active = 1;
                }
            } else {
                night_charge_qual = 0;
            }
        } else {
            night_charge_qual = 0;
            if (cfg_settings.battery_type == 1 &&
                cfg_settings.battery_discharge_min_power_reserve <= batt_soc) {
                night_charge_timer = cfg_settings.hr108 * 120;
            }
            if (night_charge_timer < cfg_settings.hr108 * 120) {
                night_charge_timer = night_charge_timer + 1;
            } else {
                night_charge_active = 0;
            }
        }
        charge_active = night_charge_active;
    } else {
        if (rtc_time.hour < 1 || rtc_time.hour > 4) {
            night_charge_timer = 0;
        }
        night_charge_active = 0;
        night_charge_qual = 0;
    }
}

/* The panel indicators: PC9 load above 50 W, PA8 PV voltage above 100.0 V, PB14/PB15 battery
   charging or discharging by the sign of batt_power_calc (both off when off grid), PC6/PC7 the
   charge status (1-4 or not; both off at 7). */
void status_outputs_update() {

    if (g_200002B8 > 0x1f4) {
        panel_led_pc9_set(1);
    } else {
        panel_led_pc9_set(0);
    }
    if (pv1_voltage > 0x3e8 || pv2_voltage > 0x3e8) {
        panel_led_pv_set(1);
    } else {
        panel_led_pv_set(0);
    }
    if (sys_mode == 0) {
        panel_led_batt_charge_set(0);
        panel_led_batt_discharge_set(0);
    } else {
        if (batt_power_calc >= 0) {
            panel_led_batt_charge_set(1);
            panel_led_batt_discharge_set(0);
        } else {
            panel_led_batt_charge_set(0);
            panel_led_batt_discharge_set(1);
        }
    }
    if (charge_status == 7) {
        panel_led_pc7_set(0);
        panel_led_pc6_set(0);
    } else {
        if (charge_status >= 1 && charge_status <= 4) {
            panel_led_pc7_set(0);
            panel_led_pc6_set(1);
        } else {
            panel_led_pc7_set(1);
            panel_led_pc6_set(0);
        }
    }
}

/* Despite the name it runs every 500 ms pass of task_schedule:
   - PC9 to 1 after the first 3 s (status_outputs_update then sets it again);
   - the clock: RTC_Get every second pass, unless a clock write is pending or an update runs; a
     pending clock write is given up after 60 s;
   - the dongle watchdog (USB CDC, not a stick): usb_cdc_timer counts passes since the last
     good request; at 120 (60 s) it sends 302 bytes of an uninitialised stack buffer to the dongle;
     at 1440 (12 min) it switches the OTG FS clock off and restarts USB (usb_reset_state);
   - a save request every 360 passes (3 min);
   - fault codes 25, 30, 31 held for 6 passes open fault_recover_window;
   - the energy counters once the RTC runs. */
void sys_tick_1s() {
    uchar cdc_data[304];

    if (g_20000103 < 6) {
        g_20000103 = g_20000103 + 1;
    } else {
        gpio_pc9_set(1);
    }
    if (usb_start_delay < 0x14) {
        usb_start_delay = usb_start_delay + 1;
    }
    if (g_20000102 == 0) {
        g_20000102 = 1;
    } else {
        g_20000102 = 0;
        if (rtc_set_pending == 0 && inv_run_mode != 4) {
            RTC_Get();
        }
    }
    if (usb_class_code != 8) {
        if (usb_cdc_timer < 0x5a0) {
            usb_cdc_timer = usb_cdc_timer + 1;
            if (usb_cdc_timer == 0x78 && usb_device_inserted == 2) {
                CDC_SendData(cdc_data, 0x12e);
            }
        } else {
            usb_cdc_timer = 0;
            usb_reset_state = 1;
            RCC_AHBPeriphClockCmd(0x1000, 0);
            g_20000101 = 0;
        }
        if (usb_reset_state == 1) {
            if (g_20000101 < 0xd) {
                g_20000101 = g_20000101 + 1;
            } else {
                g_20000101 = 0;
                usb_reset_state = 2;
                usb_cdc_timer = 0;
            }
        }
    } else {
        usb_reset_state = 0;
        usb_cdc_timer = 0;
    }
    if (rtc_set_pending > 0) {
        if (g_20000100 < 0x78) {
            g_20000100 = g_20000100 + 1;
        } else {
            g_20000100 = 0;
            rtc_set_pending = 0;
        }
    } else {
        g_20000100 = 0;
    }
    g_200000FE = g_200000FE + 1;
    if (g_200000FE >= 0x168) {
        g_200000FE = 0;
        save_req_pending = 1;
    }
    if (fault_code == 0x1e || fault_code == 0x1f || fault_code == 0x19) {
        if (g_200000FC < 8) {
            g_200000FC = g_200000FC + 1;
        }
        if (g_200000FC == 6) {
            fault_recover_window = 1;
        }
    } else {
        g_200000FC = 0;
        fault_recover_window = 0;
    }
    if (g_200001B0 == 0x64) {
        if (g_200000FD <= 2) {
            g_200000FD = g_200000FD + 1;
        }
        if (g_200000FD > 2) {
            g_200001B0 = 1;
        }
    }
    if (rtc_ready[0] == 1) {
        energy_accumulate();
    }
}

/* sched_window_out (page 7) = HR318 between the times HR319 and HR320 (end exclusive, may wrap),
   else 0. */
void sched_window_value() {
    uchar unused_value;

    g_20000104 = rtc_time.min + rtc_time.hour * 100;
    if (cfg_settings.hr319 <= cfg_settings.hr320) {
        if (cfg_settings.hr319 <= g_20000104 && cfg_settings.hr320 > g_20000104) {
            sched_window_out[0] = cfg_settings.hr318;
        } else {
            sched_window_out[0] = 0;
        }
    } else {
        if (cfg_settings.hr319 <= g_20000104 && g_20000104 <= 0x960 ||
            cfg_settings.hr320 > g_20000104) {
            sched_window_out[0] = cfg_settings.hr318;
        } else {
            sched_window_out[0] = 0;
        }
    }
}

/* The normal mode: the charge and discharge slots, then the calibration (HR29 non-zero, on
   grid): bit 1 clear means discharge, bit 1 set and bit 2 clear means charge (task_dsp_rx moves
   the bits on); without a calibration the night recovery charge. A charge cancels a discharge. */
void sched_charge_discharge_eval() {

    sched_charge_slots_eval();
    sched_discharge_slots_eval();
    if ((cfg_settings.soc_force_adjust > 0) && (sys_mode == 1)) {
        if ((cfg_settings.soc_force_adjust & 2) == 0) {
            discharge_active = 1;
            charge_active = '\0';
        } else if ((cfg_settings.soc_force_adjust & 4) == 0) {
            discharge_active = 0;
            charge_active = '\x01';
        }
    }
    if (cfg_settings.soc_force_adjust == 0) {
        batt_night_recovery_charge();
    }
    if (charge_active == '\x01') {
        discharge_active = 0;
    }
    return;
}

/* Dead: no caller and no pointer in the image. With HR58 = 1 it would switch to lead acid (HR54
   = 0, 125 Ah, HR111 = 20, HR29 = 0) when the DSP reports no BMS, and back to lithium (HR111 =
   50) when it reports one. So HR58 does nothing in A316, and the battery type only changes by
   HR54 or HR332 writes. */
void batt_auto_judge_type() {

    if (cfg_settings.enable_auto_judge_battery_type == '\x01') {
        if (bms_present == 0) {
            if (cfg_settings.battery_type == '\x01') {
                cfg_settings.battery_type = '\0';
                cfg_settings.soc_force_adjust = 0;
                cfg_settings.battery_charge_limit = 0x14;
                cfg_settings.battery_capacity = 0x7d;
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else if ((bms_present == 1) && (cfg_settings.battery_type == '\0')) {
            cfg_settings.battery_type = '\x01';
            cfg_settings.battery_charge_limit = 0x32;
            eeprom_dirty = eeprom_dirty | 1;
        }
    }
    return;
}

/* Task 6 (priority 6), every 500 ticks:
   - DSP page 4 byte 16 set for 5 passes starts the battery firmware relay (dsp_flash_mode = 1);
   - HR200 is cleared after 240 passes;
   - HR329 forcing is cleared after 14400 passes (2 h) with the DSP SOC below 10;
   - the RTC is started once the settings are loaded;
   - the charge and discharge decision: table mode (HR322 = 1), else the slots and calibration,
     else nothing at charge status 7;
   - the SOC targets, sys_tick_1s, the panel indicators, sched_window_value. */
void task_schedule() {

    while (1) {
        if (dsp_p4_b16 >= 1) {
            if (g_20000107++ >= 4) {
                g_20000107 = 4;
                dsp_flash_mode = 1;
            }
        } else {
            g_20000107 = 0;
        }
        if (hr200_request == 1) {
            if (g_20000108++ >= 0xf0) {
                g_20000108 = 0;
                hr200_request = 0;
            }
        } else {
            g_20000108 = 0;
        }
        if (force_state_hr329 == 1 && batt_soc_dsp < 0xa) {
            g_2000010A = g_2000010A + 1;
            if (g_2000010A > 0x3840) {
                force_state_hr329 = 0;
            }
        } else {
            g_2000010A = 0;
        }
        if (usb_reset_state >= 2 && usb_reset_state < 0x14) {
            usb_reset_state = usb_reset_state + 1;
        }
        if (rtc_ready[0] == 0 && eeprom_busy == 0 && eeprom_load_ok == 2) {
            eeprom_busy_2 = 1;
            if (RTC_Init() != 0) {
                eeprom_busy_2 = 0;
                g_200001AA = 1;
                rtc_ready[0] = 1;
            } else {
                g_20000106 = g_20000106 + 1;
                if (g_20000106 > 1) {
                    rtc_ready[0] = 1;
                    g_20000106 = 0;
                    g_200001AA = 1;
                    eeprom_busy_2 = 0;
                }
            }
        }
        if (cfg_block6.hr322 == 1) {
            sched_halfhour_table_eval();
        } else {
            if (charge_status != 7) {
                hold_active = 0;
                sched_charge_discharge_eval();
            } else {
                hold_active = 0;
                discharge_active = 0;
                charge_active = 0;
            }
        }
        sched_slot_soc_targets();
        sys_tick_1s();
        status_outputs_update();
        sched_window_value();
        task_tbl[TASK_SCHEDULE].count = 0;
        task_tbl[TASK_SCHEDULE].period = 0x1f4;
        os_flags_pend(0);
    }
}
