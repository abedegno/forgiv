// clang-format off
/* at: fault_debounce_count 0x08015378 */
/* at: dsp_link_settle_count 0x2000004C, calibration_release_count 0x20000050, bms_hr4_retry_count 0x20000052, adc_offset_state 0x20000184, grid_country_byte_a 0x2000019A, grid_country_byte_b 0x2000019B, cfg_changed_flags 0x2000019C, dsp_reserved_page4_word 0x200001A4, dsp_ignored_mode_byte 0x200001B0, dsp_pv_bus_mode_byte 0x200001D7, hr4000_value 0x20000206, p_eps_sample 0x200002B0, ct_cycle_count 0x200002D0, hold_active 0x200002DA, peek_addr 0x200002F2, charger_temp 0x200002FC, heatsink_temp 0x20000300, batt_temp 0x20000304, drm0_active 0x20000315, sched_window_out 0x2000038C, board_init_done 0x20000396, discharge_min_soc_now 0x2000039E, charge_target_soc_now 0x2000039F, bms_update_pending 0x200003A8, hr200_request 0x200003AB, hr201_request 0x200003CE, cal_gains 0x20001B8D, dsp_bms_page_valid 0x2000004E, dsp_bms_subpage 0x2000004F, dsp_tx_flags 0x20000054, dsp_tx_crc 0x20000056, dsp_rx_timeout 0x20000058, dsp_rx_flags 0x20000059, dsp_rx_byte 0x2000005A, dsp_rx_crc 0x2000005C, dsp_rx_crc_errs 0x2000005E, dsp_rx_ok_count 0x2000005F, pv_power_override 0x20000066, fault_flags 0x2000011C, fault_code_word 0x20000120, dsp_page0_fresh 0x20000124, dsp_p3_w14 0x20000168, inv_countdown 0x2000018E, dsp_hr4_hr5 0x20000190, save_req_pending 0x200001A8, grid_country 0x200001AB, inv_run_mode 0x200001AE, v_ac_dsp 0x200001B4, grid_current 0x200001B8, dsp_p3_w11 0x200001BA, p_apparent_dsp 0x200001C0, f_ac_dsp 0x200001CA, dsp_output_permitted 0x200001D2, pf_dsp 0x200001D4, dsp_p0_w17 0x200001DA, bus_p_voltage 0x200001DC, pv1_voltage 0x200001E0, pv2_voltage 0x200001E4, pv1_current 0x200001EC, pv2_current 0x200001F0, pv1_power 0x200001F4, pv2_power 0x200001F8, dsp_p2_b01 0x20000219, dsp_rx_idx 0x2000022B, g_2000022C 0x2000022C, dsp_tx_page 0x2000022D, dsp_rx_pages_seen 0x2000022E, dsp_fault_data1 0x20000230, dsp_fault_data2 0x20000232, dsp_p3_w07 0x20000234, dsp_p2_w14 0x20000236, dsp_p3_w03 0x20000238, dsp_p3_w05 0x2000023A, dsp_p2_w12 0x2000023C, fault_code 0x2000023E, dsp_version_hr19 0x20000240, dsp_fw_version 0x20000244, auto_test_state 0x20000247, dsp_p4_b02 0x20000248, dsp_p4_w03 0x2000024A, dsp_p4_w07 0x2000024C, dsp_p4_w14 0x20000250, dsp_p4_w12 0x20000252, dsp_p4_w05 0x20000254, dsp_p4_w10 0x20000256, dsp_p4_b09 0x2000025A, dsp_p4_w19 0x2000025E, eeprom_dirty 0x20000260, p_inverter_dsp 0x2000026C, dsp_p2_d17 0x2000027C, dsp_p1_b21 0x2000028E, batt_power_calc 0x200002C8, sys_mode 0x200002D2, charge_status 0x200002D5, dsp_warning_word 0x200002D6, charge_active 0x200002D8, discharge_active 0x200002D9, batt_state_dsp 0x200002DB, batt_voltage 0x200002DC, batt_current 0x200002DE, p_battery_dsp 0x200002E0, batt_soc_dsp 0x200002E3, batt_capacity_bms 0x200002E4, batt_temp_dsp 0x200002E6, bus_hv_voltage 0x200002E8, eps_voltage 0x200002EA, eps_frequency 0x200002EC, peek_value 0x200002F4, arm_update_state 0x20000307, dsp_flash_step 0x20000308, batt_full_flag 0x20000313, drm_power_limit 0x20000314, batt_cal_request 0x20000397, dsp_p3_b19 0x20000398, bms_chip_version 0x2000039A, bms_fw_version 0x200003A2, dsp_flash_mode 0x200003A4, dsp_p4_b16 0x200003A9, bms_present 0x200003AE, force_state_hr329 0x200003C8, batt_power_remote 0x200003CA, bms_charge_limit_a 0x200003CC, bms_discharge_limit_a 0x200003CD, os_task_dly_idx 0x200004D8, OSTCBCur 0x200004E0, dsp_bms_raw 0x20000716, dsp_rx_frame 0x20001319, dsp_tx_frame 0x20001346, cfg_settings 0x20001ABA, cfg_block6 0x20001C8E, bms_serial_bytes 0x2000236C, dsp_fault_words 0x20002376, task_tbl 0x20007EB0 */
/* at: bms_request_flags 0x2000004A, bms_hr1_request_pending 0x200003D2 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* dsp_link.c: the serial link to the TI C2000 DSP on UART4 (9600 8N1). The DSP runs the power
   stage and is the only thing that talks to the battery's BMS; the ARM sends it settings and
   commands and gets back measurements, faults and relayed BMS data.

   Entry points: task_dsp_tx (task 2, every 60 ticks), task_dsp_rx (task 1, woken by flag 8 when
   a 45-byte frame is in, else every 200 ticks); dsp_build_page and dsp_parse_page are theirs.
   Dead: fault_debounce_count.
   Data: dsp_tx_frame (45 bytes, and up to 1034 when the firmware relays reuse it), dsp_rx_frame,
   dsp_tx_page (which of the 8 settings pages goes next), and the many globals the pages fill
   (the dsp_pN_* names give the page and offset of fields whose meaning I could not tell).
   Neighbours: drivers/uart.c (port 3), control/monitor.c (fault helpers, which use what this
   file receives), usb/usbh_usr.c (the DSP and BMS firmware relays run inside task_dsp_rx),
   control/measure.c (bms_poll, which reads and writes dsp_bms_raw).

   Both directions are 45-byte frames with a CRC16/MODBUS over bytes 0-42, sent high byte first
   at 43-44. ARM to DSP (0x43):

     0      0x43
     1      run mode (inv_run_mode), or 0x0A for grid code 4 with HR53 = 0 outside mode 4
     2      fault_flags >> 4
     3      active power rate HR50, capped by drm_power_limit (grid codes 2, 3, HR41 = 1)
     4      battery state: 2 if HR329 forcing, else 1 if charging or HR175, else 2 if
            discharging, else 0
     5-6    battery power setpoint: HR495 when HR494 = 1, else -batt_power_calc / 10
     7      settings page number 0..7, rotating
     8-42   the page (dsp_build_page)

   DSP to ARM (0x44):

     0      0x44
     1      run mode          2  charge status       3-6  fault word (32 bits)
     7-8    warning word (IR57; forced to 0 by HR351 = 1)
     9      dsp_output_permitted (0 keeps the output off, inv_output_enable_ctrl)
     10-11  peek_value (HR25 when HR23 selects the DSP)
     12     telemetry page number 0..6
     13-42  the page (dsp_parse_page)

   name: descriptive (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/application_types.h"
#include "../../include/battery.h"
#include "../../include/meter.h"
#include "../../include/board.h"
#include "../../include/dsp_link.h"
#include "../../include/eeprom.h"
#include "../../include/faults.h"
#include "../../include/measure.h"
#include "../../include/modbus.h"
#include "../../include/schedule.h"
#include "../../include/settings.h"
#include "../../include/stdperiph.h"
#include "../../include/sys.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"
#include "../../include/usb.h"

/* Dead. A debounce counter: while value > limit it counts up to max and then sets fault bit
   0x100; otherwise returns 0. */
int fault_debounce_count(int unused_fault, uchar count, int threshold, int value, int limit) {

    if (value > limit) {
        if (count >= threshold) {
            fault_set(0x100);
        } else {
            count++;
        }
        return count;
    }
    return 0;
}

/* Counts dsp_link_settle_count up to 500 once the ADC offsets are known (adc_offset_state 2).
   task_dsp_rx calls it after 10 good frames with pages 0-3 seen. The count's reader is not in
   this file. */
void dsp_link_settle_tick() {
    if (adc_offset_state != 2) {
        return;
    }
    if (dsp_link_settle_count < 0x1f4) {
        dsp_link_settle_count = dsp_link_settle_count + 1;
    } else {
        return;
    }
}

/* Unpacks the telemetry page that starts at dsp_rx_frame[off] (off is 12). Offsets below are
   from the page number byte; words are big-endian.

     page 0: 2 PV1 V, 4 PV1 I, 6-9 PV1 P, 10 PV2 V, 12 PV2 I, 14 inv_countdown, 16 sys_mode,
             17 dsp_p0_w17, 19 inverter power / 10 (signed), 21 SOC (batt_soc_dsp),
             22 battery V (0.01 V), 24 battery I (0.01 A), 26 battery P, 28 battery temperature
     page 1: 1 grid current, 3 PF, 5 grid frequency, 7 grid voltage, (9-12 skipped),
             13 bus voltage, 15-18 apparent power, 19 DSP version (HR19), 21 dsp_p1_b21,
             22 high bus voltage, 24 EPS voltage, 26 EPS frequency
     page 2: 1 dsp_p2_b01, 2-5 PV2 P, 6-9 HR4/HR5, 10 BMS capacity (HR55 read-back),
             12/14 two words, 16 battery "full" byte, 17-20 dsp_p2_d17, 21 BMS chip version
             (HR46), 23 BMS firmware version (HR18)
     page 3: 1 fault data 2, 3/5/7 words, 9 fault data 1, 11 word, 13 fault code,
             14 word, 16 DSP firmware version, 18 battery state, 19 dsp_p3_b19,
             20-29 the BMS serial (BMS HR5-9; served as HR8-12)
     page 4: 1 auto test state, 2-15 words and bytes (IR225-234), 16 dsp_p4_b16 (asks for the
             battery firmware relay, task_schedule), 17 BMS-present word, 19 word,
             21-30 DSP fault words bytes 0-9 (CF12-CF16; 6 and 7 zeroed while HR351 = 1)
     page 5: 1 "BMS data valid"; when 1: 2 sub-page 0..4, 3-22 twenty raw bytes of the BMS's
             FC4 reply into dsp_bms_raw + 2 + 20 x sub-page; sub-page 4 (or above) sets the
             "reply ready" byte for bms_poll; frame bytes 0x23/0x24 the BMS charge and discharge limits in A
     page 6: 1-6 DSP fault words bytes 10-15

   PV1 and PV2 power are replaced by pv_power_override / 2 while that is above 300. Pages 0-3 set
   bits 1, 2, 4, 8 of dsp_rx_pages_seen. Every page clears the DSP link fault. */
void dsp_parse_page(uchar frame_idx) {
    uchar byte_idx;
    short inverter_power;

    byte_idx = 0;
    inverter_power = 0;
    if (dsp_rx_frame[frame_idx] == 0) {
        frame_idx = (uchar)(frame_idx + 1);
        frame_idx = (uchar)(frame_idx + 1);
        pv1_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        pv1_current = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        pv1_power =
            (0xffff0000 & dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100 << 0x10) +
            (dsp_rx_frame[frame_idx + 3] + dsp_rx_frame[frame_idx + 2] * 0x100);
        frame_idx = (uchar)(frame_idx + 4);
        if (pv_power_override > 0x12c) {
            pv1_power = pv_power_override / 2;
        }
        pv2_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        pv2_current = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        inv_countdown = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        sys_mode = dsp_rx_frame[frame_idx];
        frame_idx = (uchar)(frame_idx + 1);
        dsp_p0_w17 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        inverter_power = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        p_inverter_dsp = inverter_power * 10;
        frame_idx = (uchar)(frame_idx + 2);
        batt_soc_dsp = dsp_rx_frame[frame_idx];
        frame_idx = (uchar)(frame_idx + 1);
        batt_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        batt_current = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        p_battery_dsp = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        batt_temp_dsp = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
        frame_idx = (uchar)(frame_idx + 2);
        dsp_page0_fresh = 1;
        dsp_rx_pages_seen = dsp_rx_pages_seen | 1;
        fault_clear(FAULT_DSP_LINK);
    } else {
        if (dsp_rx_frame[frame_idx] == 1) {
            frame_idx = (uchar)(frame_idx + 1);
            grid_current = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            pf_dsp = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            f_ac_dsp = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            v_ac_dsp = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            frame_idx = (uchar)(frame_idx + 4);
            bus_p_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            p_apparent_dsp =
                (0xffff0000 & dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100
                                  << 0x10) +
                (dsp_rx_frame[frame_idx + 3] + dsp_rx_frame[frame_idx + 2] * 0x100);
            frame_idx = (uchar)(frame_idx + 4);
            dsp_version_hr19 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            dsp_p1_b21 = dsp_rx_frame[frame_idx];
            frame_idx = (uchar)(frame_idx + 1);
            bus_hv_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            eps_voltage = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            eps_frequency = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
            frame_idx = (uchar)(frame_idx + 2);
            dsp_rx_pages_seen = dsp_rx_pages_seen | 2;
            fault_clear(FAULT_DSP_LINK);
        } else {
            if (dsp_rx_frame[frame_idx] == 2) {
                frame_idx = (uchar)(frame_idx + 1);
                dsp_p2_b01 = dsp_rx_frame[frame_idx];
                frame_idx = (uchar)(frame_idx + 1);
                pv2_power =
                    (0xffff0000 & dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100
                                      << 0x10) +
                    (dsp_rx_frame[frame_idx + 3] + dsp_rx_frame[frame_idx + 2] * 0x100);
                frame_idx = (uchar)(frame_idx + 4);
                if (pv_power_override > 0x12c) {
                    pv2_power = pv_power_override / 2;
                }
                dsp_hr4_hr5 =
                    (0xffff0000 & dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100
                                      << 0x10) +
                    (dsp_rx_frame[frame_idx + 3] + dsp_rx_frame[frame_idx + 2] * 0x100);
                frame_idx = (uchar)(frame_idx + 4);
                batt_capacity_bms = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                frame_idx = (uchar)(frame_idx + 2);
                dsp_p2_w12 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                frame_idx = (uchar)(frame_idx + 2);
                dsp_p2_w14 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                frame_idx = (uchar)(frame_idx + 2);
                batt_full_flag = dsp_rx_frame[frame_idx];
                frame_idx = (uchar)(frame_idx + 1);
                dsp_p2_d17 =
                    (0xffff0000 & dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100
                                      << 0x10) +
                    (dsp_rx_frame[frame_idx + 3] + dsp_rx_frame[frame_idx + 2] * 0x100);
                frame_idx = (uchar)(frame_idx + 4);
                bms_chip_version = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                frame_idx = (uchar)(frame_idx + 2);
                bms_fw_version = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                frame_idx = (uchar)(frame_idx + 2);
                dsp_rx_pages_seen = dsp_rx_pages_seen | 4;
                fault_clear(FAULT_DSP_LINK);
            } else {
                if (dsp_rx_frame[frame_idx] == 3) {
                    frame_idx = (uchar)(frame_idx + 1);
                    dsp_fault_data2 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_p3_w03 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_p3_w05 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_p3_w07 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_fault_data1 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_p3_w11 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    fault_code = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    dsp_p3_w14 = 0;
                    dsp_p3_w14 =
                        (ushort)(dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100);
                    frame_idx = (uchar)(frame_idx + 2);
                    dsp_fw_version = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                    frame_idx = (uchar)(frame_idx + 2);
                    batt_state_dsp = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    dsp_p3_b19[0] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[0] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[1] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[2] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[3] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[4] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[5] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[6] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[7] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[8] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    bms_serial_bytes[9] = dsp_rx_frame[frame_idx];
                    frame_idx = (uchar)(frame_idx + 1);
                    dsp_rx_pages_seen = dsp_rx_pages_seen | 8;
                    fault_clear(FAULT_DSP_LINK);
                } else {
                    if (dsp_rx_frame[frame_idx] == 4) {
                        frame_idx = (uchar)(frame_idx + 1);
                        auto_test_state = dsp_rx_frame[frame_idx];
                        frame_idx = (uchar)(frame_idx + 1);
                        dsp_p4_b02[0] = dsp_rx_frame[frame_idx];
                        frame_idx = (uchar)(frame_idx + 1);
                        dsp_p4_w03 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_w05 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_w07 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_b09[0] = dsp_rx_frame[frame_idx];
                        frame_idx = (uchar)(frame_idx + 1);
                        dsp_p4_w10 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_w12 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_w14 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_b16 = dsp_rx_frame[frame_idx];
                        frame_idx = (uchar)(frame_idx + 1);
                        bms_present = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_p4_w19 = dsp_rx_frame[frame_idx + 1] + dsp_rx_frame[frame_idx] * 0x100;
                        frame_idx = (uchar)(frame_idx + 2);
                        dsp_fault_words[0] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[1] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[2] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[3] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[4] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[5] = dsp_rx_frame[frame_idx++];
                        if (cfg_block6.hr351 == 1) {
                            dsp_fault_words[6] = 0;
                            frame_idx = (uchar)(frame_idx + 1);
                            dsp_fault_words[7] = 0;
                            frame_idx = (uchar)(frame_idx + 1);
                        } else {
                            dsp_fault_words[6] = dsp_rx_frame[frame_idx++];
                            dsp_fault_words[7] = dsp_rx_frame[frame_idx++];
                        }
                        dsp_fault_words[8] = dsp_rx_frame[frame_idx++];
                        dsp_fault_words[9] = dsp_rx_frame[frame_idx++];
                        fault_clear(FAULT_DSP_LINK);
                    } else {
                        if (dsp_rx_frame[frame_idx] == 5) {
                            frame_idx = (uchar)(frame_idx + 1);
                            dsp_bms_page_valid = dsp_rx_frame[frame_idx];
                            frame_idx = (uchar)(frame_idx + 1);
                            if (dsp_bms_page_valid == 1) {
                                dsp_bms_raw[153] = 0;
                                dsp_bms_subpage = dsp_rx_frame[frame_idx];
                                frame_idx = (uchar)(frame_idx + 1);
                                if (dsp_bms_subpage <= 4) {
                                    for (byte_idx = 0; byte_idx <= 0x13; byte_idx++) {
                                        (dsp_bms_raw + 2)[byte_idx + dsp_bms_subpage * 20] =
                                            dsp_rx_frame[frame_idx];
                                        frame_idx = (uchar)(frame_idx + 1);
                                    }
                                }
                                if (dsp_bms_subpage >= 4) {
                                    dsp_bms_raw[153] = 1;
                                }
                            }
                            frame_idx = 0x23;
                            bms_charge_limit_a = dsp_rx_frame[frame_idx];
                            frame_idx++;
                            bms_discharge_limit_a = dsp_rx_frame[frame_idx];
                            frame_idx++;
                            fault_clear(FAULT_DSP_LINK);
                        } else {
                            if (dsp_rx_frame[frame_idx] == 6) {
                                frame_idx = (uchar)(frame_idx + 1);
                                dsp_fault_words[10] = dsp_rx_frame[frame_idx++];
                                dsp_fault_words[11] = dsp_rx_frame[frame_idx++];
                                dsp_fault_words[12] = dsp_rx_frame[frame_idx++];
                                dsp_fault_words[13] = dsp_rx_frame[frame_idx++];
                                dsp_fault_words[14] = dsp_rx_frame[frame_idx++];
                                dsp_fault_words[15] = dsp_rx_frame[frame_idx++];
                                frame_idx = 24;
                                bms_request_flags = dsp_rx_frame[frame_idx++];
                                if ((bms_request_flags >> 2 & 1) != 0)
                                    bms_hr1_request_pending = 0;
                                fault_clear(FAULT_DSP_LINK);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Writes settings page dsp_tx_page into dsp_tx_frame from byte off (7): the page number, then
   (offsets from the page number byte; words big-endian):

     page 0: 1 HR2, 3 HR61, 5 HR60 PV start voltage (1550 while HR175 = 1), 7 HR125, 8 HR1,
             10 HR50 (DRM cap), 11 HR51, 12 HR52, 14 HR114, 15 charge_active,
             16 discharge_active, 17 HR27 (1 while hold_active), 18 HR53 (or drm0_active for
             grid codes 2, 3, 20, 21 with HR41), 19 dsp_ignored_mode_byte, 20 HR122, 21 dsp_pv_bus_mode_byte,
             22 HR62, 24 0, 25 HR33, 26 settings +0x74, 27 HR54, 28 HR55, 30 charger,
             32 heatsink, 34 battery temperature
     page 1: 1-4 p_eps_sample, 5 HR97, 7 HR98, 9-34 HR64, 63, 66, 65, 72, 71, 74, 73, 80, 79,
             68, 67, 70, 35 auto test state (0 or 1)
     page 2: 1-14 HR69, 76, 75, 78, 77, 82, 81, 15 cfg_changed_flags, 16 grid country byte a,
             17 HR128, 18 HR123, 19 HR83, 21 HR121, 22 HR143, 23 HR142, 24 HR144, 26 HR145,
             28 HR26 (6000 while HR29 != 0), 30 HR28, 31 HR29, 32 HR332 (+0xD2),
             33 ct_cycle_count, 34 HR328
     page 3: 1-24 HR130-141, 25 grid country byte b, 26 HR321, 28 HR24 peek address,
             (30-31 left as zero), 32 HR326, 34 HR327
     page 4: 1 ct_cycle_count, 2 HR124, 3 dsp_reserved_page4_word, 5 +0x69, 7 +0x6B, 9 HR175, 10 HR109,
             11 HR176, 12 HR111, 13 HR112, 14 discharge stop SOC now (4 while HR329 forcing),
             15 HR115, 16 pv_power_override, 18 HR177, 19 bms_update_pending, 20 HR200,
             21 meter link state (board_init_done with meters, else aux link fault bit)
     page 5: 1-34 calibration gains and offsets 0-4, gain 5, 11, 9, 10; 35 HR201 request
     page 6: 1-12 gains and offsets 6-8, 13 BMS request flag, 14-20 the BMS request (device,
             4, start, count; staged by bms_poll), 23 HR300-303, 31 HR178, 32 HR179, 33 HR305
     page 7: 1 HR306, 3 HR307, 5 HR308, 7 HR309, 9 charge target SOC now, 10 HR101,
             11 HR312, 13 HR313, 14 HR314, 15 HR315, 17 HR316, 19 HR102, 21 sched_window_out,
             22 HR4000, 24 HR20, 25 HR116

   Side effects: page 4 clears HR175 (batt_cal_request) after more than 300 page-4 frames in a
   row with batt_voltage > 4800; the former battery-current condition is absent in 319; page 5 sends the HR201 request in 5 frames and then clears it; page 6 clears the BMS
   request flag, so each pack poll request goes out once. */
typedef struct {
    ushort other_low : 11;
    ushort bms_hr1_request : 1;
    ushort other_high : 4;
} dsp_request_word_t;

void dsp_build_page(uchar frame_idx, int unused_page) {
    ushort word_value;
    short battery_power_target;

    word_value = 0;
    battery_power_target = 0;
    dsp_tx_frame[frame_idx++] = dsp_tx_page;
    if (dsp_tx_page == 0) {
        dsp_tx_frame[frame_idx++] = cfg_settings.inverter_module_l >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.inverter_module_l;
        dsp_tx_frame[frame_idx++] = cfg_settings.inverter_start_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.inverter_start_time;
        if (batt_cal_request == 1) {
            word_value = 0x60e;
            dsp_tx_frame[frame_idx++] = word_value >> 8;
            dsp_tx_frame[frame_idx++] = word_value;
        } else {
            dsp_tx_frame[frame_idx++] = cfg_settings.v_pv_input_start >> 8;
            dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_pv_input_start;
        }
        dsp_tx_frame[frame_idx++] = cfg_settings.hr125;
        dsp_tx_frame[frame_idx++] = cfg_settings.inverter_module_h >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.inverter_module_h;
        if ((grid_country == 3 || grid_country == 2) && cfg_settings.enable_drm_rj45 == 1 &&
            cfg_settings.active_power_rate > drm_power_limit) {
            dsp_tx_frame[frame_idx++] = drm_power_limit;
        } else {
            dsp_tx_frame[frame_idx++] = cfg_settings.active_power_rate;
        }
        dsp_tx_frame[frame_idx++] = cfg_settings.reactive_power_rate;
        dsp_tx_frame[frame_idx++] = cfg_settings.power_factor >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.power_factor;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_discharge_min_power_reserve;
        dsp_tx_frame[frame_idx++] = charge_active;
        dsp_tx_frame[frame_idx++] = discharge_active;
        if (hold_active == 1) {
            dsp_tx_frame[frame_idx++] = 1;
        } else {
            dsp_tx_frame[frame_idx++] = cfg_settings.battery_power_mode;
        }
        if ((grid_country == 3 || grid_country == 0x14 || grid_country == 0x15 ||
             grid_country == 2) &&
            cfg_settings.enable_drm_rj45 == 1 && cfg_settings.inverter_state == 1) {
            dsp_tx_frame[frame_idx++] = drm0_active[0];
        } else {
            dsp_tx_frame[frame_idx++] = cfg_settings.inverter_state;
        }
        dsp_tx_frame[frame_idx++] = dsp_ignored_mode_byte;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr122;
        dsp_tx_frame[frame_idx++] = dsp_pv_bus_mode_byte;
        dsp_tx_frame[frame_idx++] = cfg_settings.inverter_restart_delay >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.inverter_restart_delay;
        dsp_tx_frame[frame_idx++] = 0;
        dsp_tx_frame[frame_idx++] = cfg_settings.user_code;
        dsp_tx_frame[frame_idx++] = cfg_settings.f74;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_type;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_capacity >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.battery_capacity;
        dsp_tx_frame[frame_idx++] = charger_temp >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)charger_temp;
        dsp_tx_frame[frame_idx++] = heatsink_temp >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)heatsink_temp;
        dsp_tx_frame[frame_idx++] = batt_temp >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)batt_temp;
    } else if (dsp_tx_page == 1) {
        dsp_tx_frame[frame_idx++] = (uint)p_eps_sample >> 0x18;
        dsp_tx_frame[frame_idx++] = (uint)p_eps_sample >> 0x10 & 0xff;
        dsp_tx_frame[frame_idx++] = (short)p_eps_sample >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)p_eps_sample;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_low_voltage_limit >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.battery_low_voltage_limit;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_high_voltage_limit >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.battery_high_voltage_limit;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_high_out >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_high_out;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_low_out >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_low_out;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_high_out >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_high_out;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_low_out >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_low_out;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_high_in >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_high_in;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_low_in >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_low_in;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_high_in >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_high_in;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_low_in >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_low_in;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_high_c >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_high_c;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_low_c >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_low_c;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_high_out_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_high_out_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_low_out_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_low_out_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_high_out_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_high_out_time;
        if (auto_test_state <= 1) {
            dsp_tx_frame[frame_idx++] = auto_test_state;
        } else {
            dsp_tx_frame[frame_idx++] = 0;
        }
    } else if (dsp_tx_page == 2) {
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_low_out_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_low_out_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_high_in_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_high_in_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_ac_low_in_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_ac_low_in_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_high_in_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_high_in_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_low_in_time >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_low_in_time;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_high_c >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_high_c;
        dsp_tx_frame[frame_idx++] = cfg_settings.f_ac_low_c >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f_ac_low_c;
        dsp_tx_frame[frame_idx++] = cfg_changed_flags;
        dsp_tx_frame[frame_idx++] = grid_country_byte_a;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr128;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr123;
        dsp_tx_frame[frame_idx++] = cfg_settings.v_10_min_protection >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.v_10_min_protection;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr121;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr143;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr142;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr144 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr144;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr145 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr145;
        if (cfg_settings.soc_force_adjust == 0) {
            dsp_tx_frame[frame_idx++] = cfg_settings.grid_port_max_power >> 8;
            dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.grid_port_max_power;
        } else {
            dsp_tx_frame[frame_idx++] = 0x17;
            dsp_tx_frame[frame_idx++] = 0x70;
        }
        dsp_tx_frame[frame_idx++] = cfg_settings.enable_60hz_freq_mode;
        dsp_tx_frame[frame_idx++] = cfg_settings.soc_force_adjust;
        dsp_tx_frame[frame_idx++] = cfg_settings.fD2;
        dsp_tx_frame[frame_idx++] = ct_cycle_count;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr328 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr328;
    } else if (dsp_tx_page == 3) {
        dsp_tx_frame[frame_idx++] = cfg_settings.hr130 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr130;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr131 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr131;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr132 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr132;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr133 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr133;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr134 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr134;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr135 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr135;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr136 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr136;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr137 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr137;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr138 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr138;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr139 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr139;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr140 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr140;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr141 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr141;
        dsp_tx_frame[frame_idx++] = grid_country_byte_b;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr321 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr321;
        dsp_tx_frame[frame_idx++] = peek_addr >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)peek_addr;
        frame_idx++;
        frame_idx++;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr326 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr326;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr327;
    } else if (dsp_tx_page == 4) {
        dsp_tx_frame[frame_idx++] = ct_cycle_count;
        dsp_tx_frame[frame_idx++] = cfg_block6.hr124;
        dsp_tx_frame[frame_idx++] = dsp_reserved_page4_word >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)dsp_reserved_page4_word;
        dsp_tx_frame[frame_idx++] = cfg_settings.f69 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f69;
        dsp_tx_frame[frame_idx++] = cfg_settings.f6B >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.f6B;
        if (batt_voltage > 4800) {
            if (++calibration_release_count > 0x12c) {
                calibration_release_count = 0;
                batt_cal_request = 0;
            }
        } else {
            calibration_release_count = 0;
        }
        dsp_tx_frame[frame_idx++] = batt_cal_request;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr109;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr176;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_charge_limit;
        dsp_tx_frame[frame_idx++] = cfg_settings.battery_discharge_limit;
        if (force_state_hr329 != 0) {
            dsp_tx_frame[frame_idx++] = 4;
        } else {
            dsp_tx_frame[frame_idx++] = discharge_min_soc_now;
        }
        dsp_tx_frame[frame_idx++] = cfg_settings.hr115;
        dsp_tx_frame[frame_idx++] = pv_power_override >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)pv_power_override;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr177;
        dsp_tx_frame[frame_idx++] = bms_update_pending;
        dsp_tx_frame[frame_idx++] = hr200_request;
        if (cfg_settings.enable_ammeter >= 1) {
            dsp_tx_frame[frame_idx++] = board_init_done;
        } else if (((uchar)fault_flags & 0x40) != 0) {
            dsp_tx_frame[frame_idx++] = 1;
        } else {
            dsp_tx_frame[frame_idx++] = 0;
        }
        if (cfg_settings.meter_type == 2) {
            battery_power_target = meter_data[0].p_l1 / 10;
        } else {
            if (cfg_block6.hr494 == 1) {
                battery_power_target = batt_power_remote;
            } else {
                battery_power_target = -batt_power_calc / 10;
            }
        }
        dsp_tx_frame[frame_idx++] = battery_power_target >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = battery_power_target;
    } else if (dsp_tx_page == 5) {
        dsp_tx_frame[frame_idx++] = cal_gains.gain0 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain0;
        dsp_tx_frame[frame_idx++] = cal_gains.offset0 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset0;
        dsp_tx_frame[frame_idx++] = cal_gains.gain1 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain1;
        dsp_tx_frame[frame_idx++] = cal_gains.offset1 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset1;
        dsp_tx_frame[frame_idx++] = cal_gains.gain2 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain2;
        dsp_tx_frame[frame_idx++] = cal_gains.offset2 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset2;
        dsp_tx_frame[frame_idx++] = cal_gains.gain3 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain3;
        dsp_tx_frame[frame_idx++] = cal_gains.offset3 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset3;
        dsp_tx_frame[frame_idx++] = cal_gains.gain4 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain4;
        dsp_tx_frame[frame_idx++] = cal_gains.offset4 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset4;
        dsp_tx_frame[frame_idx++] = cal_gains.gain5 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain5;
        dsp_tx_frame[frame_idx++] = cal_gains.gain11 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain11;
        dsp_tx_frame[frame_idx++] = cal_gains.offset11 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset11;
        dsp_tx_frame[frame_idx++] = cal_gains.gain9 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain9;
        dsp_tx_frame[frame_idx++] = cal_gains.offset9 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset9;
        dsp_tx_frame[frame_idx++] = cal_gains.gain10 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain10;
        dsp_tx_frame[frame_idx++] = cal_gains.offset10 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset10;
        dsp_tx_frame[0x2a] = hr201_request;
        if (hr201_request == 1) {
            if (bms_hr4_retry_count++ >= 4) {
                bms_hr4_retry_count = 0;
                hr201_request = 0;
            }
        } else {
            bms_hr4_retry_count = 0;
        }
    } else if (dsp_tx_page == 6) {
        dsp_tx_frame[frame_idx++] = cal_gains.gain6 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain6;
        dsp_tx_frame[frame_idx++] = cal_gains.offset6 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset6;
        dsp_tx_frame[frame_idx++] = cal_gains.gain7 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain7;
        dsp_tx_frame[frame_idx++] = cal_gains.offset7 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset7;
        dsp_tx_frame[frame_idx++] = cal_gains.gain8 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.gain8;
        dsp_tx_frame[frame_idx++] = cal_gains.offset8 >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)cal_gains.offset8;
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[152];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[132];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[133];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[134];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[135];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[136];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[137];
        dsp_tx_frame[frame_idx++] = dsp_bms_raw[138];
        dsp_bms_raw[152] = 0;
        dsp_tx_frame[frame_idx++] = 0;
        dsp_tx_frame[frame_idx++] = 0;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr300 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr300;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr301 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr301;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr302 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr302;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr303 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr303;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr178;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr179;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr305;
        dsp_tx_frame[frame_idx++] = 0;
        dsp_tx_frame[frame_idx++] = 0;
    } else if (dsp_tx_page == 7) {
        dsp_tx_frame[frame_idx++] = cfg_settings.hr306 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr306;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr307 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr307;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr308 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr308;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr309 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr309;
        dsp_tx_frame[frame_idx++] = charge_target_soc_now[0];
        dsp_tx_frame[frame_idx++] = cfg_settings.hr101;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr312 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr312;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr313;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr314;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr315 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr315;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr316 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr316;
        ((dsp_request_word_t *)&cfg_settings.hr102)->bms_hr1_request = bms_hr1_request_pending == 1;
        dsp_tx_frame[frame_idx++] = cfg_settings.hr102 >> 8;
        dsp_tx_frame[frame_idx++] = (uchar)cfg_settings.hr102;
        dsp_tx_frame[frame_idx++] = sched_window_out[0];
        dsp_tx_frame[frame_idx++] = hr4000_value >> 8 & 0xff;
        dsp_tx_frame[frame_idx++] = (uchar)hr4000_value;
        dsp_tx_frame[frame_idx++] = cfg_settings.enable_charge_target;
        dsp_tx_frame[frame_idx++] = cfg_settings.charge_target_soc;
    }
}

/* Task 2 (priority 2), every 60 ticks: when the last frame has gone (flag 0x10 from the UART4
   transmit interrupt) and no firmware relay is running, builds and sends one 0x43 frame (layout
   in the file header) with the next settings page. fault_code_update runs first, so byte 2 has
   the current faults. */
void task_dsp_tx() {
    uchar i;
    short v;
    int st = 0;

    task_tbl[TASK_DSP_TX].FlagsRdy |= 0x10;
    /* match: the low byte of FlagsRdy, loaded signed; (sbyte)OSTCBCur->FlagsRdy loads the halfword */
    dsp_tx_flags = *(sbyte *)&OSTCBCur->FlagsRdy;
    while (1) {
        if (dsp_flash_mode != 1 && dsp_flash_step < 2 && (dsp_tx_flags & 0x10) != 0) {
            for (i = 0; i < 0x2d; i++) {
                dsp_tx_frame[i] = 0;
            }
            fault_code_update();
            dsp_tx_frame[0] = DSP_FRAME_TX;
            if (inv_run_mode != 4 && cfg_settings.inverter_state == 0 && grid_country == 4) {
                dsp_tx_frame[1] = 0xa;
            } else {
                dsp_tx_frame[1] = inv_run_mode;
            }
            dsp_tx_frame[2] = fault_flags >> 4;
            if (force_state_hr329 != 0) {
                st = 2;
            } else if (charge_active == 1 || batt_cal_request == 1) {
                st = 1;
            } else if (discharge_active != 0) {
                st = 2;
            } else {
                st = 0;
            }
            if ((grid_country == 3 || grid_country == 2) && cfg_settings.enable_drm_rj45 == 1 &&
                cfg_settings.active_power_rate > drm_power_limit) {
                dsp_tx_frame[3] = drm_power_limit;
            } else {
                dsp_tx_frame[3] = cfg_settings.active_power_rate;
            }
            dsp_tx_frame[4] = st;
            if (cfg_block6.hr494 == 1) {
                v = batt_power_remote;
            } else {
                v = -batt_power_calc / 10;
            }
            dsp_tx_frame[5] = v >> 8;
            dsp_tx_frame[6] = v;
            dsp_build_page(7, dsp_tx_page);
            if (dsp_tx_page < 7) {
                dsp_tx_page++;
            } else {
                dsp_tx_page = 0;
            }
            dsp_tx_crc = crc16_modbus(dsp_tx_frame, 0x2b);
            dsp_tx_frame[0x2b] = dsp_tx_crc / 256;
            dsp_tx_frame[0x2c] = dsp_tx_crc % 256;
            g_2000022C = 0;
            uart_tx_start(3, dsp_tx_frame, 0x2d);
            task_tbl[TASK_DSP_TX].FlagsRdy &= ~0x10;
        }
        task_tbl[TASK_DSP_TX].count = 0;
        task_tbl[TASK_DSP_TX].period = 0x3c;
        dsp_tx_flags = os_flags_pend(0);
    }
}

/* Task 1 (priority 1). Three jobs:
   - the battery firmware relay (dsp_flash_mode 1): bms_fw_update_process every 100 ticks;
   - the DSP firmware update (dsp_flash_step > 0) and the ARM self-update flag: save the energy
     counters, set run mode 4, stop tasks 0, 5 and 6 and TIM3, and run dsp_fw_update_process every
     10 ticks;
   - otherwise, on flag 8 (45 bytes of a 0x44 frame in), check the CRC and take run mode, charge
     status, faults (the DSP's bits with the ARM's own bits 6-11 kept), the warning word, then the
     page. 50 bad frames, or 50 wake-ups without a frame, set the DSP link fault (0x80).

   The calibration (HR29 soc_force_adjust) is advanced here, on every good frame while HR29 is
   non-zero: warning bit 0x800 sets HR29 bit 1 (the "empty" end, inference); otherwise bit 0x400
   or bit 0x002 sets bit 2 (the "full" end); HR29 >= 7 ends it (0). Each change is saved. Which
   BMS bits the DSP maps to 0x800, 0x400 and 0x002 is in the DSP (docs/battery.md, section 5).

   The CRC is checked on every flag-8 wake-up over whatever is in dsp_rx_frame; dsp_rx_idx is
   then reset whether or not a whole frame came in. */
void task_dsp_rx() {

    uchar page_idx = 0;
    int fault_word = 0;

    dsp_rx_flags = *(sbyte *)&OSTCBCur->FlagsRdy;
    uart_rx_reset(3, 0x2d);
    while (1) {
        if (dsp_flash_mode == 1) {
            bms_fw_update_process();
            task_tbl[TASK_DSP_RX].count = 0;
            task_tbl[TASK_DSP_RX].period = 0x64;
        } else {
            if (arm_update_state > 0 && (dsp_flash_step == 0 || dsp_flash_step == 5)) {
                arm_fw_update_arm_bootflag();
            }
            if (dsp_flash_step > 0) {
                if (inv_run_mode != 4) {
                    eeprom_dirty = eeprom_dirty | 8;
                    save_req_pending = 1;
                    for (page_idx = 0; page_idx < 5; page_idx++) {
                        os_task_delay((uchar)os_task_dly_idx, 0x7d0, 0xa);
                        usb_read_msn_file();
                        if (eeprom_dirty == 0) {
                            break;
                        }
                    }
                }
                inv_run_mode = 4;
            }
            if (inv_run_mode == 4) {
                task_tbl[TASK_MEASURE].period = 0;
                task_tbl[TASK_MONITOR].period = 0;
                task_tbl[TASK_SCHEDULE].period = 0;
                TIM_ITConfig(TIM3_BASE, 1, 0);
                TIM_Cmd(TIM3_BASE, 0);
                dsp_fw_update_process();
                task_tbl[TASK_DSP_RX].FlagsRdy = task_tbl[TASK_DSP_RX].FlagsRdy & -9;
                task_tbl[TASK_DSP_RX].count = 0;
                task_tbl[TASK_DSP_RX].period = 0xa;
            } else {
                if ((dsp_rx_flags & 8) != 0) {
                    while (uart_rx_getc(3, &dsp_rx_byte) == 0) {
                        if (dsp_rx_idx == 0) {
                            if (dsp_rx_byte == DSP_FRAME_RX) {
                                dsp_rx_frame[0] = dsp_rx_byte;
                                dsp_rx_idx = dsp_rx_idx + 1;
                            }
                        } else if (dsp_rx_idx < 0x2d) {
                            dsp_rx_frame[dsp_rx_idx] = dsp_rx_byte;
                            dsp_rx_idx = dsp_rx_idx + 1;
                        }
                    }
                    dsp_rx_crc = dsp_rx_frame[44] + dsp_rx_frame[43] * 0x100;
                    if (crc16_modbus(&dsp_rx_frame[0], 0x2b) == dsp_rx_crc) {
                        inv_run_mode = dsp_rx_frame[1];
                        charge_status = dsp_rx_frame[2];
                        fault_word =
                            (0xffff0000 & dsp_rx_frame[4] + dsp_rx_frame[3] * 0x100 << 0x10) +
                            (dsp_rx_frame[6] + dsp_rx_frame[5] * 0x100);
                        fault_code_word = (fault_word & -4033) + (fault_flags & 0xfc0);
                        dsp_warning_word = dsp_rx_frame[8] + dsp_rx_frame[7] * 0x100;
                        if (cfg_block6.hr351 == 1) {
                            dsp_warning_word = 0;
                        }
                        fault_check_mode();
                        if (cfg_settings.soc_force_adjust > 0) {
                            if (dsp_warning_test(0x800) != 0) {
                                cfg_settings.soc_force_adjust = cfg_settings.soc_force_adjust | 2;
                                eeprom_dirty = eeprom_dirty | 1;
                                save_req_pending = 1;
                            } else {
                                if (dsp_warning_test(0x400) != 0 || dsp_warning_test(2) != 0) {
                                    cfg_settings.soc_force_adjust =
                                        cfg_settings.soc_force_adjust | 4;
                                    eeprom_dirty = eeprom_dirty | 1;
                                    save_req_pending = 1;
                                }
                            }
                            if (cfg_settings.soc_force_adjust >= 7) {
                                cfg_settings.soc_force_adjust = 0;
                                eeprom_dirty = eeprom_dirty | 1;
                                save_req_pending = 1;
                            }
                        }
                        dsp_output_permitted = dsp_rx_frame[9];
                        peek_value = (short)(dsp_rx_frame[11] + dsp_rx_frame[10] * 0x100);
                        dsp_parse_page(0xc);
                        dsp_rx_crc_errs = 0;
                        if (dsp_rx_ok_count < 0xa) {
                            dsp_rx_ok_count = dsp_rx_ok_count + 1;
                        }
                    } else {
                        dsp_rx_crc_errs = dsp_rx_crc_errs + 1;
                        if (dsp_rx_crc_errs > 0x32) {
                            uart_rx_reset(3, 0x2d);
                            dsp_rx_crc_errs = 0;
                            fault_set(FAULT_DSP_LINK);
                        }
                    }
                    dsp_rx_timeout = 0;
                    dsp_rx_idx = 0;
                } else {
                    dsp_rx_timeout = dsp_rx_timeout + 1;
                    if (dsp_rx_timeout > 0x32) {
                        uart_rx_reset(3, 0x2d);
                        dsp_rx_timeout = 0;
                        fault_set(FAULT_DSP_LINK);
                        dsp_rx_idx = 0;
                    }
                }
                if (dsp_rx_ok_count >= 0xa && dsp_rx_pages_seen >= 0xf) {
                    dsp_link_settle_tick();
                }
                task_tbl[TASK_DSP_RX].FlagsRdy = task_tbl[TASK_DSP_RX].FlagsRdy & -9;
                task_tbl[TASK_DSP_RX].count = 0;
                task_tbl[TASK_DSP_RX].period = 0xc8;
            }
        }
        dsp_rx_flags = os_flags_pend(0);
    }
}
