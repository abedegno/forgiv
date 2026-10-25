// clang-format off
/* at: inv_output_enable_ctrl 0x08027018, calib_apply_q10 0x0802709C, bms_poll 0x080270B4, bms_update_fail_flag_load 0x0802780A, task_measure 0x08027872 */
/* at: output_enable_count 0x200000DC, bms_fail_load_count 0x200000E8, measure_flags 0x200000E9, adc_rms_ch0 0x20000150, adc_sum_ch0 0x20000178, adc_sum_ch1 0x2000017C, adc_sum_ch5 0x20000180, adc_offset_state 0x20000184, adc_offset_ch0 0x20000186, adc_offset_ch1 0x20000188, adc_offset_ch5 0x2000018A, grid_country 0x200001AB, inv_run_mode 0x200001AE, v_ac_dsp 0x200001B4, i_rms_ch5 0x200001BC, dsp_output_permitted 0x200001D2, output_enabled 0x200001D8, eeprom_dirty 0x20000260, bms_update_failed 0x200003AA, dsp_bms_raw 0x20000716, bms_modules 0x200007B0, cfg_settings 0x20001ABA, cal_gains 0x20001B8D, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* measure.c: task_measure, the highest-priority task: the inverter output enable, the RMS values
   of the ADC windows, and the battery pack poll that rides on the DSP link.

   Entry points: task_measure (task 0), and what it calls: inv_output_enable_ctrl, bms_poll,
   bms_update_fail_flag_load. Dead: calib_apply_q10.
   Data: bms_modules[6] (the pack records, battery.h), the poll state bms_poll_*, the output
   enable counter, measure_flags (the event flags the task woke with).
   Neighbours: monitor.c before, crc32_helpers.c after; comm/dsp_link.c (dsp_bms_raw is the
   mailbox: page 6 of the ARM's frames carries the request out, page 5 of the DSP's frames brings
   the reply), comm/modbus_slave.c (serves bms_modules as IR60-119 of slaves 0x32-0x36),
   stm32f10x_it.c (flag 2 and the ADC sums).

   dsp_bms_raw (0x20000716), as bms_poll and the DSP pages use it:
     +0x02..+0x83  the BMS's FC4 reply as the DSP relays it: [device][fc][start hi][start lo]
                   [data...] (GivEnergy's reply echoes the start where Modbus has a byte count)
     +0x84..+0x89  the request: device, 4, start, count (big-endian)
     +0x98         request pending (bms_poll sets it, dsp_build_page page 6 sends and clears it)
     +0x99         reply ready (dsp_parse_page page 5 sets it after sub-page 4)

   The poll and the checks on every field are in docs/battery.md, section 2.

   name: descriptive (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/battery.h"
#include "../../include/board.h"
#include "../../include/dsp_link.h"
#include "../../include/eeprom.h"
#include "../../include/faults.h"
#include "../../include/measure.h"
#include "../../include/modbus.h"
#include "../../include/settings.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"

/* Decides output_enabled: off at once while the DSP's frame byte 9 is 0, or HR53 = 0 with grid
   code 4, or (in run mode 1) a fault is set; otherwise on after 20 passes. buzzer_on/off follow
   it (TIM3_IRQHandler drives PB5 while output_enabled is 1). Inference: the output relay or its
   drive. */
void inv_output_enable_ctrl() {

    if (dsp_output_permitted == 0 || cfg_settings.inverter_state == 0 && grid_country == 4) {
        buzzer_off();
        output_enable_count = 0;
        output_enabled = 0;
    } else {
        if (inv_run_mode == 1) {
            if (fault_check_mode() != 0) {
                buzzer_off();
                output_enable_count = 0;
                output_enabled = 0;
            } else {
                buzzer_on();
                if (output_enable_count < 0x14) {
                    output_enable_count = output_enable_count + 1;
                } else {
                    output_enabled = 1;
                }
            }
        } else {
            buzzer_on();
            if (output_enable_count < 0x14) {
                output_enable_count = output_enable_count + 1;
            } else {
                output_enabled = 1;
            }
        }
    }
}

/* Dead. (x x gain + offset) >> 10, 0 when not positive. */
uint calib_apply_q10(int sample, int gain, int offset) {
    uint scaled_value = 0;

    scaled_value = sample * gain + offset >> 10;
    if (!((int)scaled_value <= 0)) {
        return scaled_value & 0xffff;
    }
    return 0;
}

typedef __packed struct {
    ushort f00;
    uchar f02[21];
    uchar pad17[21];
    uchar f2c;
    ushort f2d;
    int f2f;
    ushort f33;
    int f35;
    uint f39;
    uint f3d;
    uint f41;
    uchar f45;
    uchar f46;
    uchar f47;
    uchar f48;
    uchar f49;
    uchar f4a;
    uchar f4b;
    uchar f4c;
    uchar f4d;
    uchar f4e;
    ushort f4f;
    ushort minimum_cell_voltage;
    ushort f53[16];
    short f73;
    short f75;
    ushort f77;
    short f79;
    short f7b;
    short f7d;
    short f7f;
    short f81;
} bms_pack_signed_view_t;
typedef struct {
    uchar hdr[2];
    union {
        uchar rx[0x82];
        struct {
            uchar a[4];
            uchar data[0x7e];
        } s;
        struct {
            uchar a[4];
            struct {
                uchar hi;
                uchar lo;
            } c[0x3f];
        } t;
    } u;
    uchar tx[0x14];
    uchar f98;
    uchar f99;
} bms_mailbox_view_t;
#define BMS_MAILBOX (*(bms_mailbox_view_t *)dsp_bms_raw)
/* match: BMS_PACKS is bms_modules with the temperatures signed and the status bytes as fields; written
   as bms_modules[i] fields the bytes of bms_poll differ (each store tried) */
#define BMS_PACKS ((bms_pack_signed_view_t *)((uchar *)bms_modules))

/* The battery pack poll, once per task_measure pass (about every 20 ms, from the ADC window).
   Every 500th call it moves to the next request (start 0 count 21, start 21 count 19, start 40
   count 20, then start 0 again for the next device 1-5) and stages it for the DSP, unless the
   previous request has not gone yet (then that request is skipped). When the DSP has delivered a
   reply, it checks the device (1-5), echoed start (0, 21 or 40), and CRC, then stores each field into
   bms_modules[device - 1] only when it is in range, and clears the reply area. There is no function-code or matching-request-device check and no timeout.
   A DSP battery-fault indication clears all pack records. Last it counts packs whose status word has bit 0
   set into bms_packs_online (records 0-5; record 5 is never filled).
   Field layout and ranges: docs/battery.md, section 2. */
void bms_poll() {
    ushort cmd = 0;
    uchar i = 0;
    uchar cnt = 0;
    ushort cell_voltage_sum = 0;
    short pack_temp = 0;
    int pack_voltage = 0;
    uint capacity = 0;
    uchar soc_pct = 0;
    ushort reply_crc = 0;

    if (bms_poll_timer++ >= 500) {
        bms_poll_timer = 0;
        switch (bms_poll_step) {
        case 0:
            bms_poll_reg = 0;
            bms_poll_count = 21;
            bms_poll_step++;
            break;
        case 1:
            bms_poll_reg = 21;
            bms_poll_count = 19;
            bms_poll_step++;
            break;
        case 2:
            bms_poll_reg = 40;
            bms_poll_count = 20;
            bms_poll_step++;
            break;
        case 3:
            bms_poll_step = 0;
            bms_poll_reg = 0;
            bms_poll_count = 21;
            bms_poll_addr++;
            if (bms_poll_addr > 5) {
                bms_poll_addr = 1;
            }
            break;
        }
        if (BMS_MAILBOX.f98 == 0) {
            BMS_MAILBOX.f98 = 1;
            BMS_MAILBOX.tx[0] = bms_poll_addr;
            BMS_MAILBOX.tx[1] = 4;
            BMS_MAILBOX.tx[2] = bms_poll_reg >> 8;
            BMS_MAILBOX.tx[3] = bms_poll_reg;
            BMS_MAILBOX.tx[4] = bms_poll_count >> 8;
            BMS_MAILBOX.tx[5] = bms_poll_count;
        }
    }
    if (BMS_MAILBOX.f99 == 1) {
        BMS_MAILBOX.f99 = 0;
        if (BMS_MAILBOX.u.rx[0] >= 1 && BMS_MAILBOX.u.rx[0] <= 5) {
            bms_rx_pack = BMS_MAILBOX.u.rx[0] - 1;
            cmd = BMS_MAILBOX.u.rx[3] + BMS_MAILBOX.u.rx[2] * 256;
            if (cmd == 0) {
                reply_crc = BMS_MAILBOX.u.rx[46] + BMS_MAILBOX.u.rx[47] * 256;
            } else if (cmd == 21) {
                reply_crc = BMS_MAILBOX.u.rx[42] + BMS_MAILBOX.u.rx[43] * 256;
            } else if (cmd == 40) {
                reply_crc = BMS_MAILBOX.u.rx[44] + BMS_MAILBOX.u.rx[45] * 256;
            }
            if (cmd == 0 && crc16_modbus(BMS_MAILBOX.u.rx, 46) == reply_crc) {
                for (i = 0; i < 21; i++) {
                    bms_modules[bms_rx_pack].serial[i] = BMS_MAILBOX.u.s.data[i];
                }
                pack_temp = BMS_MAILBOX.u.rx[0x1b] + BMS_MAILBOX.u.rx[0x1a] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f79 = pack_temp;
                pack_temp = BMS_MAILBOX.u.rx[0x1d] + BMS_MAILBOX.u.rx[0x1c] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f7b = pack_temp;
                pack_temp = BMS_MAILBOX.u.rx[0x1f] + BMS_MAILBOX.u.rx[0x1e] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f7d = pack_temp;
                pack_temp = BMS_MAILBOX.u.rx[0x21] + BMS_MAILBOX.u.rx[0x20] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f7f = pack_temp;
                pack_temp = BMS_MAILBOX.u.rx[0x23] + BMS_MAILBOX.u.rx[0x22] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f81 = pack_temp;
                bms_modules[bms_rx_pack].pack_status_word = BMS_MAILBOX.u.rx[0x25] + BMS_MAILBOX.u.rx[0x24] * 256;
            }
            if (cmd == 21 && crc16_modbus(BMS_MAILBOX.u.rx, 42) == reply_crc) {
                bms_modules[bms_rx_pack].num_cells = BMS_MAILBOX.u.rx[4];
                bms_modules[bms_rx_pack].num_cycles = BMS_MAILBOX.u.rx[6] + BMS_MAILBOX.u.rx[5] * 256;
                pack_voltage = (BMS_MAILBOX.u.rx[7] << 24) + (BMS_MAILBOX.u.rx[8] << 16) + (BMS_MAILBOX.u.rx[9] << 8) +
                               BMS_MAILBOX.u.rx[10];
                if (pack_voltage >= 0 && pack_voltage < 60000)
                    bms_modules[bms_rx_pack].v_out = pack_voltage;
                cell_voltage_sum = BMS_MAILBOX.u.rx[12] + BMS_MAILBOX.u.rx[11] * 256;
                if ((cell_voltage_sum > 0 || cell_voltage_sum == 0) && cell_voltage_sum < 60000)
                    bms_modules[bms_rx_pack].v_cells_sum = cell_voltage_sum;
                pack_voltage = (BMS_MAILBOX.u.rx[13] << 24) | (BMS_MAILBOX.u.rx[14] << 16) |
                               (BMS_MAILBOX.u.rx[15] << 8) | BMS_MAILBOX.u.rx[16];
                bms_modules[bms_rx_pack].pack_current_raw = pack_voltage;
                capacity = (BMS_MAILBOX.u.rx[17] << 24) + (BMS_MAILBOX.u.rx[18] << 16) + (BMS_MAILBOX.u.rx[19] << 8) +
                           BMS_MAILBOX.u.rx[20];
                if ((capacity > 0 || capacity == 0) && capacity < 200000)
                    bms_modules[bms_rx_pack].cap_calibrated = capacity;
                capacity = (BMS_MAILBOX.u.rx[21] << 24) + (BMS_MAILBOX.u.rx[22] << 16) + (BMS_MAILBOX.u.rx[23] << 8) +
                           BMS_MAILBOX.u.rx[24];
                if ((capacity > 0 || capacity == 0) && capacity < 200000)
                    bms_modules[bms_rx_pack].cap_design = capacity;
                capacity = (BMS_MAILBOX.u.rx[25] << 24) + (BMS_MAILBOX.u.rx[26] << 16) + (BMS_MAILBOX.u.rx[27] << 8) +
                           BMS_MAILBOX.u.rx[28];
                if ((capacity > 0 || capacity == 0) && capacity < 200000)
                    bms_modules[bms_rx_pack].cap_remaining = capacity;
                soc_pct = BMS_MAILBOX.u.rx[29];
                if ((soc_pct > 0 || soc_pct == 0) && soc_pct <= 100)
                    bms_modules[bms_rx_pack].soc = soc_pct;
                bms_modules[bms_rx_pack].status[0] = BMS_MAILBOX.u.rx[30];
                BMS_PACKS[bms_rx_pack].f47 = BMS_MAILBOX.u.rx[31];
                BMS_PACKS[bms_rx_pack].f48 = BMS_MAILBOX.u.rx[32];
                BMS_PACKS[bms_rx_pack].f49 = BMS_MAILBOX.u.rx[33];
                BMS_PACKS[bms_rx_pack].f4a = BMS_MAILBOX.u.rx[34];
                BMS_PACKS[bms_rx_pack].f4b = BMS_MAILBOX.u.rx[35];
                BMS_PACKS[bms_rx_pack].f4c = BMS_MAILBOX.u.rx[36];
                BMS_PACKS[bms_rx_pack].f4d = BMS_MAILBOX.u.rx[37];
                BMS_PACKS[bms_rx_pack].f4e = BMS_MAILBOX.u.rx[38];
                bms_modules[bms_rx_pack].fw_version = BMS_MAILBOX.u.rx[40] + BMS_MAILBOX.u.rx[39] * 256;
            } else if (cmd == 40 && crc16_modbus(BMS_MAILBOX.u.rx, 44) == reply_crc) {
                for (i = 0; i < 16; i++) {
                    cell_voltage_sum = (BMS_MAILBOX.u.rx + 5)[i * 2] + ((BMS_MAILBOX.u.rx + 4)[i * 2] << 8);
                    if (cell_voltage_sum > 2200 && cell_voltage_sum < 3700)
                        bms_modules[bms_rx_pack].v_cell[i] = cell_voltage_sum;
                }
                pack_temp = BMS_MAILBOX.u.rx[0x25] + BMS_MAILBOX.u.rx[0x24] * 256;
                if (pack_temp > -300 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f73 = pack_temp;
                pack_temp = BMS_MAILBOX.u.rx[0x27] + BMS_MAILBOX.u.rx[0x26] * 256;
                if (pack_temp > -200 && pack_temp < 700)
                    BMS_PACKS[bms_rx_pack].f75 = pack_temp;
                cell_voltage_sum = BMS_MAILBOX.u.rx[0x29] + BMS_MAILBOX.u.rx[0x28] * 256;
                if ((cell_voltage_sum > 0 || cell_voltage_sum == 0) && cell_voltage_sum < 700)
                    bms_modules[bms_rx_pack].maximum_cell_voltage = cell_voltage_sum;
                cell_voltage_sum = BMS_MAILBOX.u.rx[0x2b] + BMS_MAILBOX.u.rx[0x2a] * 256;
                if ((cell_voltage_sum > 0 || cell_voltage_sum == 0) && cell_voltage_sum < 700)
                    bms_modules[bms_rx_pack].minimum_cell_voltage = cell_voltage_sum;
            }
            __aeabi_memclr(BMS_MAILBOX.u.rx, 0x82);
        }
    }
    if ((dsp_fault_words[7] >> 1 & 1) != 0) {
        __aeabi_memclr(bms_modules, sizeof(bms_modules));
    }
    cnt = 0;
    for (i = 0; i <= 5; i++) {
        if (bms_modules[i].pack_status_word & 1) {
            cnt = cnt + 1;
        }
    }
    bms_packs_online[0] = cnt;
}
#undef BMS_MAILBOX
#undef BMS_PACKS

/* On the 10th pass after boot: copies the stored "battery firmware update failed" word (block 2
   +0x34) into bms_update_failed (1 only when it is exactly 1), then clears the stored word and
   marks block 2 for saving. So a failed update blocks another try only until the next restart. */
void bms_update_fail_flag_load() {
    ushort unused_flags;

    if (bms_fail_load_count < 0x14) {
        bms_fail_load_count = bms_fail_load_count + 1;
    }
    if (bms_fail_load_count == 0xa) {
        if (cal_gains.bms_update_failed == 1) {
            bms_update_failed = 1;
        } else {
            bms_update_failed = 0;
        }
        cal_gains.bms_update_failed = 0;
        eeprom_dirty = eeprom_dirty | 2;
    }
}

/* Task 0 (priority 0), every 25 ticks or on flag 2 (end of an ADC window): output enable; sets
   PC3; on flag 2, finishes the ADC zero offsets once (sum / 0x300) and computes the RMS of
   channels 0 and 5 for the window (channel 0 forced to 0 while the DSP reports no grid voltage);
   then the pack poll and the update-failed flag. */
void task_measure() {
    int unused_value;

    while (1) {
        inv_output_enable_ctrl();
        gpio_pc3_set(1);
        if ((measure_flags[0] & 2) != 0) {
            if (adc_offset_state == 1) {
                adc_offset_ch0 = (uint)adc_sum_ch0 / 0x300;
                adc_offset_ch1 = (uint)adc_sum_ch1 / 0x300;
                adc_offset_ch5 = (uint)adc_sum_ch5 / 0x300;
                adc_offset_state = 2;
            }
            adc_rms_ch0 = isqrt_div(adc_cycle_sumsq_ch0, adc_cycle_n);
            i_rms_ch5 = 0xffff & isqrt_div(adc_cycle_sumsq_ch5, adc_cycle_n) * 10;
            if (v_ac_dsp == 0) {
                adc_rms_ch0 = 0;
            }
        }
        bms_poll();
        bms_update_fail_flag_load();
        task_tbl[TASK_MEASURE].count = 0;
        task_tbl[TASK_MEASURE].period = 0x19;
        measure_flags[0] = os_flags_pend(0);
    }
}
