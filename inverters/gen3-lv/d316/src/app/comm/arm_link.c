// clang-format off
/* arm_link.c: RTOS task 5, the serial link to the ARM on SCI-A (9600 8N1), and the handlers
   for the settings pages 0..5 that the ARM sends. The ARM (the A316 firmware, dsp_link.c in
   inverters/gen3-lv/a316) is the inverter's main controller; the DSP gets settings and
   commands from it and sends back measurements, faults and the BMS data it reads on SCI-B.

   Entry points: task5_arm_link (created by main, every 60 ticks and on EV5_FRAME from sci_rx);
   arm_frame_parse and arm_link_frame are its helpers; arm_page0..arm_page5 are called through
   arm_page_handlers (pages 6 and 7 are in arm_page_write.c).
   Data: the counters and flags at 0xDAC0..0xDAD9 (between the ebss markers below):
     arm_tx_stop, arm_tx_halted  the send stops for good after the first frame in OP_UPDATE
     arm_crc_err_cnt             bad frames; more than 5 empty the receive ring
     arm_rx_ok_cnt               good frames, up to 40; at 40 the link fault (CF18 bit 2) clears
     arm_rx_fail_cnt             bad frames and wake-ups with no frame; at 300 the fault is set
     arm_rx_idle_cnt             wake-ups with no frame; more than 5 empty the ring
     arm_tx_tick                 tick events; every 4th sends a frame
     run_cmd4_cnt                frames with run command 4 (update); the 6th posts EV0_UPDATE
     peek_addr                   the RAM word sent in bytes 10-11 (0xC000..0xE000, set by page 3)
     bms_fc4_subpage, bms_fc4_idx  where page 5 is in the 100-byte BMS reply
     autotest_done_cnt, cal_highv_cnt, cal_lowv_cnt  counters of page 4 (both directions)
     arm_fault_word              the 32-bit fault word of bytes 3-6
     bms_serial[10]              the BMS serial (bms_link.c fills it), sent on page 3
   The frame buffers are arm_rx_frame (0xCFC0) and arm_tx_frame (0xDB00), 45 words each with one
   byte per word; sci_tx_buf0 (0xCF80) is the copy sci_send transmits.
   Neighbours: drivers/sci.c (the receive ring, sci_send, EV5_FRAME), crc16.c, arm_page_write.c,
   battery/bms_link.c (bms_relay_active, the FC4 reply, the serial), state/app_init.c
   (arm_run_mode_set) and state/op_states.c (EV0_STOP, EV0_UPDATE).
   name: descriptive (map/filenames.tsv; the A316 side calls its file dsp_link.c).
   at: task5_arm_link 0x3EAABB .. arm_page5 0x3EB977 (CRC16 is in crc16.c); arm_page_handlers
   0x3F3E58; run_cmd4_cnt .. bms_serial */
// clang-format on
/* The frames. Both directions are 45 bytes, one byte per 16-bit word, with a Modbus CRC16 of
   bytes 0..42 in bytes 43-44, high byte first. The ARM sends a 0x43 frame every 60 ms; the DSP
   answers with a 0x44 frame every 240 ms (every 4th tick of this task). The names in brackets
   are the A316 decomp's (dsp_link.c, docs/battery.md).

   ARM to DSP, 0x43 ('C', ARM_FRAME_FROM_ARM), read by arm_frame_parse:
     0      0x43; sci_rx keeps only frames that start with it
     1      run command 0..10 (inv_run_mode): arm_run_command, 4 = update, 10 = stop
     2      ARM fault flags (fault_flags >> 4): bit 2 -> CF18 bit 8, bit 4 -> CF18 bit 9
     3      active_power_rate in % (HR50 capped by the DRM limit); 105 and above are ignored
     4      batt_state_cmd 0..2 (0 idle, 1 charge, 2 discharge)
     5-6    arm_batt_setpoint = the signed word x 10 (HR495, or -batt_power_calc / 10)
     7      the settings page 0..7 in bytes 8-42 (dsp_tx_page); above 7 counts as 7
     8-42   the page: arm_page_handlers[page](i) runs for i = 1..42, see arm_page0..arm_page7
     43-44  CRC16

   DSP to ARM, 0x44 ('D', ARM_FRAME_TO_ARM), built by task5_arm_link and arm_link_frame:
     0      0x44
     1      arm_run_mode(): 0 idle or starting, 1 running, 3 fault, 4 update (inv_run_mode)
     2      arm_charge_status(): 0..7 (charge_status)
     3-6    arm_fault_word, high byte first (the 32-bit fault word)
     7      bms_alarm, low byte     the ARM reads 7-8 as one warning word (dsp_warning_word,
     8      batt_warning, low byte  IR57)
     9      run_status_bits bit 3 (g_200001D2: 0 keeps the inverter output off)
     10-11  the RAM word at peek_addr (peek_value, HR25)
     12     the telemetry page 0..6, in turn
     13-42  the page, see arm_link_frame
     43-44  CRC16 */
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
#include "boot.h"
#include "power_limits.h"
#include "ram_view.h"
#include "F2806x_Device.h"
#include "F2806x_Sci.h"

// clang-format off
/* ebss: tools/climb/ebss.py */
unsigned int arm_tx_stop = 0;
unsigned int arm_crc_err_cnt = 0;
unsigned int cal_highv_cnt = 0;
unsigned int arm_tx_halted;
unsigned int cal_lowv_cnt = 0;
unsigned int peek_addr = 49152;
unsigned int arm_rx_ok_cnt = 0;
unsigned int bms_fc4_subpage = 0;
unsigned int bms_fc4_idx = 0;
unsigned int arm_rx_fail_cnt = 0;
unsigned int run_cmd4_cnt = 0;
unsigned int arm_rx_idle_cnt = 0;
unsigned int arm_tx_tick = 0;
unsigned int autotest_done_cnt = 0;
unsigned long arm_fault_word = 0;
unsigned int bms_serial[10] = {0};
/* end ebss */
// clang-format on

/* RTOS task 5, the ARM link. While bms_relay_active is set (the battery firmware relay, see
   sci.c) the task only waits: the ARM's bytes go straight to the BMS then, so no frames are made.
   EV5_FRAME (45 bytes in the ring): copies the ring into arm_rx_frame from the first 0x43 and
   checks the CRC. A good frame is parsed; 40 good frames clear CF18 bit 2 (ARM link lost). A bad
   frame counts: over 5 empty the ring, and 300 bad frames or empty wake-ups set CF18 bit 2 again
   (cleared instead while the relay runs). Every wake-up without a frame counts as idle; over 5
   empty the ring.
   EV_TICK (every 60 ms): every 4th one sends a 0x44 frame, when SCI-A is not busy and its baud
   high byte is not 2 (it is always 1, so that test never stops it), pages 0..6 in turn. In
   OP_UPDATE the first frame sent sets arm_tx_stop, and the next tick stops the sending for good
   (arm_tx_halted), so that the boot loader can have SCI-A. Each tick also rebuilds
   arm_fault_word. */
void task5_arm_link(void) {
    unsigned int ev, c, i, n = 0, rx, t, *p;
    arm_tx_halted = 0;
    for (;;) {
        do {
            ev = os_wait(0);
        } while (bms_relay_active != 0);
        if (ev & EV5_FRAME_M) {
            while (ring_read(0, &c) == 0) {
                if (arm_rx_len == 0) {
                    if (c == ARM_FRAME_FROM_ARM) {
                        arm_rx_len++;
                        arm_rx_frame[0] = c;
                    }
                } else if (arm_rx_len < 45) {
                    arm_rx_frame[arm_rx_len] = c;
                    arm_rx_len++;
                }
            }
            rx = (arm_rx_frame[43] << 8) + arm_rx_frame[44];
            if (rx == CRC16(arm_rx_frame, 43)) {
                arm_frame_parse();
                arm_rx_ok_cnt++;
                arm_rx_fail_cnt = 0;
                if (arm_rx_ok_cnt >= 40) {
                    arm_rx_ok_cnt = 40;
                    fault_stop2.all &= 0xFFFB;
                }
            } else {
                if (++arm_crc_err_cnt > 5) {
                    ring_reset(0, 150);
                    arm_crc_err_cnt = 0;
                }
                if (++arm_rx_fail_cnt >= 300) {
                    arm_rx_ok_cnt = 0;
                    arm_rx_fail_cnt = 0;
                    if (bms_relay_active == 0)
                        fault_stop2.all |= 4;
                    else
                        fault_stop2.all &= 0xFFFB;
                }
            }
            arm_rx_idle_cnt = 0;
            arm_rx_len = 0;
        } else {
            if (++arm_rx_idle_cnt > 5) {
                arm_rx_idle_cnt = 0;
                ring_reset(0, 150);
                arm_rx_len = 0;
            }
            if (++arm_rx_fail_cnt >= 300) {
                arm_rx_ok_cnt = 0;
                arm_rx_fail_cnt = 0;
                if (bms_relay_active == 0)
                    fault_stop2.all |= 4;
                else
                    fault_stop2.all &= 0xFFFB;
            }
        }
        if (ev & EV_TICK_M) {
            if (++arm_tx_tick > 3) {
                arm_tx_tick = 0;
                if (arm_tx_halted == 0 && SciaRegs.SCIHBAUD != 2 && sci_tx_done == 1) {
                    if (arm_tx_stop == 1) {
                        arm_tx_halted = 1;
                        continue;
                    }
                    for (i = 0; i < 45; i++)
                        arm_tx_frame[i] = 0;
                    p = arm_tx_frame;
                    p[0] = ARM_FRAME_TO_ARM;
                    p[1] = arm_run_mode();
                    p[2] = arm_charge_status();
                    p[3] = arm_fault_word >> 24;
                    p[4] = (arm_fault_word >> 16) & 0xFF;
                    p[5] = (arm_fault_word >> 8) & 0xFF;
                    p[6] = arm_fault_word & 0xFF;
                    p[7] = bms_alarm & 0xFF;
                    p[8] = batt_warning & 0xFF;
                    p[9] = (run_status_bits.all & 8) >> 3;
                    t = *(unsigned int *)
                            peek_addr; /* any RAM word the ARM asks for (page 3, bytes 35-36) */
                    p[10] = t >> 8;
                    p[11] = t & 0xFF;
                    arm_link_frame(p, 12, n);
                    if (++n > 6)
                        n = 0;
                    t = CRC16(p, 43);
                    p[43] = t >> 8;
                    p[44] = t & 0xFF;
                    if (sci_send(0, 45, &sci_tx_buf0, p) == 0) {
                        sci_tx_done = 0;
                        if (op_state == OP_UPDATE)
                            arm_tx_stop = 1;
                    }
                }
            }
            arm_fault_word_build();
        }
    }
}

/* Builds arm_fault_word, frame bytes 3-6, from the stop faults:
     CF17 bit 11 -> bit 17          CF18 bit 6 (calibration) -> bit 19
     CF18 bit 2 (ARM link) -> 21    CF17 bit 3 (bus over-voltage) -> 22
     CF17 bit 1 (insulation) -> 25  CF17 bit 0 (DC injection) -> 27
     CF17 bit 4 (PV over-voltage) -> 28   CF17 bit 7 (heatsink over-temperature) -> 31
   The final mask 0xFFFFF03F clears bits 6..11, which nothing here sets. */
void arm_fault_word_build(void) {
    unsigned long v = 0;
    if (fault_stop1.all & 0x800)
        v = 0x20000;
    if (fault_stop2.all & 0x40)
        v |= 0x80000;
    else
        v &= ~0x80000;
    if (fault_stop1.all & 0x1)
        v |= 0x8000000;
    else
        v &= ~0x8000000;
    if (fault_stop2.all & 0x4)
        v |= 0x200000;
    else
        v &= ~0x200000;
    if (fault_stop1.all & 0x8)
        v |= 0x400000;
    else
        v &= ~0x400000;
    if (fault_stop1.all & 0x2)
        v |= 0x2000000;
    else
        v &= ~0x2000000;
    if (fault_stop1.all & 0x10)
        v |= 0x10000000;
    else
        v &= ~0x10000000;
    if (fault_stop1.all & 0x80)
        v |= 0x80000000;
    else
        v &= ~0x80000000;
    arm_fault_word = v & 0xFFFFF03F;
}

/* Frame byte 1 for the ARM: 0 in OP_IDLE and OP_START, 3 in OP_FAULT, 4 in OP_UPDATE and 1 in
   the run states 5..8. */
int arm_run_mode(void) {
    if ((op_state & 0xFFFE) == 0)
        return 0;
    if (op_state == OP_FAULT)
        return 3;
    if (op_state == OP_UPDATE)
        return 4;
    return 1;
}

/* Frame byte 2 for the ARM, the battery's state, first match wins:
     7 battery fault CF15 bit 1, 6 any batt_warning, 0 no converter mode (conv_mode 0),
     5 discharging (the current reference below 0), 4 SOC 98 and up, 3 SOC above 90,
     2 SOC above 4, else 1. */
int arm_charge_status(void) {
    int r;
    if (fault_batt.all & 2)
        r = 7;
    else if (batt_warning != 0)
        r = 6;
    else if (conv_mode == 0)
        r = 0;
    else if (cc_batt_current_ref < 0.0f)
        r = 5;
    else if (bms_soc >= 98)
        r = 4;
    else if (bms_soc > 90)
        r = 3;
    else if (bms_soc > 4)
        r = 2;
    else
        r = 1;
    return r;
}

/* Writes the telemetry page number into p[len] (byte 12) and the page into arm_tx_frame from
   byte 13. Bytes not listed stay 0 (task5_arm_link clears the frame first). Words go high byte
   first. The A316 names, in brackets, are at the same byte (dsp_parse_page counts from byte 12).

   page 0:  13 0 (skipped)                14-15 pv1_voltage          16-17 pv1_current
            18-21 pv1_power x 0.01, 32 bits                          22-23 pv2_voltage
            24-25 pv2_current             26-27 inv_countdown_ticks / 50 (inv_countdown)
            28 1 when has_grid_fault() is 0, else 0 (sys_mode)       29-30 load_percent (dsp_p0_w17)
            31-32 eps_power x 0.1 in the EPS states, else inv_power x 0.1
            33 bms_soc_raw (batt_soc_dsp)                            34-35 batt_voltage x 10
            36-37 batt_power x 100 / batt_voltage in states 5..8, else 0 (batt_current)
            38-39 batt_power x 0.1, x 1.03 when it is 0 or more, else x 0.97 (p_battery_dsp)
            40-41 bms_temp (batt_temp_dsp)
   page 1:  13-14 eps_current x 10 in the EPS states, else grid_current x 10 (grid_current)
            15-16 power_factor (pf_dsp)   17-18 grid_freq (f_ac_dsp)  19-20 grid_voltage (v_ac_dsp)
            21-24 0 (skipped)             25-26 bus_voltage (bus_p_voltage)
            27-30 apparent_power, 32 bits (p_apparent_dsp)
            31-32 0x01 0x3C, the constant 316 (dsp_version_hr19)     33 0 (dsp_p1_b21)
            34-35 bus_hv_voltage
            36-37 eps_voltage in the EPS states or while eps_active, else 0 while fault_grid2
                  bit 15 is set, else grid_voltage (eps_voltage)
            38-39 grid_freq_nominal in the EPS states, else 0 while fault_grid2 bit 15 or
                  eps_active, else grid_freq (eps_frequency)
   page 2:  13 pv_input_mode (dsp_p2_b01) 14-17 pv2_power x 0.01, 32 bits
            18-21 grid_power_max_pv, 32 bits (dsp_hr4_hr5)
            22-23 bms_capacity when battery_type is 1 and enable_bms_read is set, else 0
                  (batt_capacity_bms)
            24-27 0 (dsp_p2_w12, dsp_p2_w14)  28 batt_blocks bit 8, battery full (batt_full_flag)
            29-32 reactive_power, signed 32 bits (dsp_p2_d17)
            33-34 bms_hr0 (bms_chip_version, BMS HR46)   35-36 bms_fw_version (BMS HR18)
   page 3:  13-22 0 (dsp_fault_data2, dsp_p3_w03, w05, w07, dsp_fault_data1)
            23-24 rms_d3d2 (dsp_p3_w11)   25 0 (fault_code)   26-27 |dc_injection| (dsp_p3_w14)
            28-29 version, 316 (dsp_fw_version)   30 0 (batt_state_dsp)   31 0 (dsp_p3_b19)
            32-41 bms_serial[0..9], low bytes (bms_serial_bytes; the ARM serves them as HR8-12)
   page 4:  13 auto_test_state; 14-27 stay 0 while it is 0:
            14 autotest_step (dsp_p4_b02)            15-16 autotest_ramp_cnt (dsp_p4_w03)
            17-18 autotest_value (dsp_p4_w05)        19-20 autotest_trip_ms (dsp_p4_w07)
            21 autotest_result (dsp_p4_b09)          22-23 autotest_limit (dsp_p4_w10)
            24-25 autotest_trip_value (dsp_p4_w12)   26-27 autotest_expect_time (dsp_p4_w14)
            28 1 when bms_relay_request (settings 0xD8E5, the BMS's HR14) is 1, else 0; this also
               sets or clears bms_relay_active (dsp_p4_b16: the battery firmware relay may start)
            29-30 bms_present             31-32 autotest_trip_value2 (dsp_p4_w19)
            33-42 dsp_fault_words bytes 0-9: fault_grid1, fault_grid2, fault_grid_eps, fault_batt,
                  fault_pv (CF12..CF16), each low byte first
            autotest_result 5 clears auto_test_state after 3 such pages; 3 or 4 clears it at once
            and sets CF17 bit 9.
   page 5:  13 bms_fc4_ready (dsp_bms_page_valid); when it is 1:
            14 bms_fc4_subpage 0..4 (dsp_bms_subpage)
            15-34 bms_fc4_buf[sub-page x 20 ..] (dsp_bms_raw): the BMS's reply to the request the
                  ARM sent on page 6; after sub-page 4, bms_fc4_ready goes back to 0
            35 bms_lim_hr26 x 0.01 (bms_charge_limit_a)   36 bms_lim_hr27 x 0.01
               (bms_discharge_limit_a); sent on every page 5
   page 6:  13-18 fault_stop1, fault_stop2, batt_blocks (CF17..CF19), low byte first
                  (dsp_fault_words bytes 10-15) */
void arm_link_frame(unsigned int *p, unsigned int len, int page) {
    unsigned int *buf, x, u;
    unsigned long l;
    long sl;
    float f;
    if (len > 12)
        return;
    p[len] = page;
    if (page == 0) {
        len = 13;
        arm_tx_frame[len++] = 0;
        x = pv1_voltage;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = pv1_current;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        l = pv1_power * 0.01f;
        arm_tx_frame[len++] = l >> 24;
        arm_tx_frame[len++] = (l >> 16) & 0xFF;
        arm_tx_frame[len++] = (l >> 8) & 0xFF;
        arm_tx_frame[len++] = l & 0xFF;
        x = pv2_voltage;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = pv2_current;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = inv_countdown_ticks / 50;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (has_grid_fault() == 0)
            x = 1;
        else
            x = 0;
        arm_tx_frame[len++] = x & 0xFF;
        x = load_percent;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT)
            x = (int)(eps_power * 0.1f);
        else
            x = (int)(inv_power * 0.1f);
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        arm_tx_frame[len++] = bms_soc_raw & 0xFF;
        x = batt_voltage * 10.0f;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT || op_state == OP_GRID_PV ||
            op_state == OP_GRID_BATT) {
            if (batt_voltage != 0.0f)
                x = (int)(batt_power * 100.0f / batt_voltage);
        } else {
            x = 0;
        }
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (batt_power >= 0.0f)
            x = (int)(batt_power * 0.1f * 1.03f);
        else
            x = (int)(batt_power * 0.1f * 0.97f);
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = bms_temp;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
    } else if (page == 1) {
        len = 13;
        if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT)
            x = (unsigned int)eps_current * 10;
        else
            x = (unsigned int)grid_current * 10;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = power_factor;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = grid_freq;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        u = grid_voltage;
        arm_tx_frame[len++] = u >> 8;
        arm_tx_frame[len++] = u & 0xFF;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        x = bus_voltage;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        l = apparent_power;
        arm_tx_frame[len++] = l >> 24;
        arm_tx_frame[len++] = (l >> 16) & 0xFF;
        arm_tx_frame[len++] = (l >> 8) & 0xFF;
        arm_tx_frame[len++] = l & 0xFF;
        arm_tx_frame[len++] = 1;
        arm_tx_frame[len++] = 60;
        arm_tx_frame[len++] = 0;
        x = bus_hv_voltage;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT || eps_active == 1)
            x = eps_voltage;
        else if (fault_grid2.bit.b15)
            x = 0;
        else
            x = u;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        if (op_state == OP_EPS_PV || op_state == OP_EPS_BATT)
            x = grid_freq_nominal;
        else if ((fault_grid2.all & 0x8000) | eps_active)
            x = 0;
        else
            x = grid_freq;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
    } else if (page == 2) {
        len = 13;
        arm_tx_frame[len++] = pv_input_mode & 0xFF;
        l = pv2_power * 0.01f;
        arm_tx_frame[len++] = l >> 24;
        arm_tx_frame[len++] = (l >> 16) & 0xFF;
        arm_tx_frame[len++] = (l >> 8) & 0xFF;
        arm_tx_frame[len++] = l & 0xFF;
        l = grid_power_max_pv;
        arm_tx_frame[len++] = l >> 24;
        arm_tx_frame[len++] = (l >> 16) & 0xFF;
        arm_tx_frame[len++] = (l >> 8) & 0xFF;
        arm_tx_frame[len++] = l & 0xFF;
        if (battery_type == 1 && enable_bms_read != 0) {
            x = bms_capacity;
            arm_tx_frame[len++] = x >> 8;
            arm_tx_frame[len++] = x & 0xFF;
        } else {
            arm_tx_frame[len++] = 0;
            arm_tx_frame[len++] = 0;
        }
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = (batt_blocks.all & 0x100) >> 8;
        sl = reactive_power;
        arm_tx_frame[len++] = (unsigned long)sl >> 24;
        arm_tx_frame[len++] = (sl >> 16) & 0xFF;
        arm_tx_frame[len++] = (sl >> 8) & 0xFF;
        arm_tx_frame[len++] = sl & 0xFF;
        x = bms_hr0;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = bms_fw_version;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
    } else if (page == 3) {
        len = 13;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        x = rms_d3d2;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        arm_tx_frame[len++] = 0;
        x = fabs(dc_injection);
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = version;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = bms_serial[0] & 0xFF;
        arm_tx_frame[len++] = bms_serial[1] & 0xFF;
        arm_tx_frame[len++] = bms_serial[2] & 0xFF;
        arm_tx_frame[len++] = bms_serial[3] & 0xFF;
        arm_tx_frame[len++] = bms_serial[4] & 0xFF;
        arm_tx_frame[len++] = bms_serial[5] & 0xFF;
        arm_tx_frame[len++] = bms_serial[6] & 0xFF;
        arm_tx_frame[len++] = bms_serial[7] & 0xFF;
        arm_tx_frame[len++] = bms_serial[8] & 0xFF;
        arm_tx_frame[len++] = bms_serial[9] & 0xFF;
    } else if (page == 4) {
        len = 13;
        buf = arm_tx_frame;
        if (auto_test_state != 0) {
            buf[len++] = auto_test_state;
            buf[len++] = autotest_step;
            x = autotest_ramp_cnt;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
            x = autotest_value;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
            x = autotest_trip_ms;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
            buf[len++] = autotest_result;
            x = autotest_limit;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
            x = autotest_trip_value;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
            x = autotest_expect_time;
            buf[len++] = x >> 8;
            buf[len++] = x & 0xFF;
        }
        if (autotest_result == 5) {
            if (++autotest_done_cnt > 2) {
                autotest_done_cnt = 0;
                auto_test_state = 0;
            }
        } else if (autotest_result == 3 || autotest_result == 4) {
            auto_test_state = 0;
            fault_stop1.all |= 0x200;
        }
        len = 28;
        if (bms_relay_request == 1) {
            arm_tx_frame[len++] = bms_relay_request;
            bms_relay_active = 1;
        } else {
            bms_relay_active = 0;
            arm_tx_frame[len++] = 0;
        }
        x = bms_present;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        x = autotest_trip_value2;
        arm_tx_frame[len++] = x >> 8;
        arm_tx_frame[len++] = x & 0xFF;
        arm_tx_frame[len++] = fault_grid1.byte.lo;
        arm_tx_frame[len++] = fault_grid1.byte.hi;
        arm_tx_frame[len++] = fault_grid2.byte.lo;
        arm_tx_frame[len++] = fault_grid2.all >> 8;
        arm_tx_frame[len++] = fault_grid_eps.byte.lo;
        arm_tx_frame[len++] = fault_grid_eps.all >> 8;
        arm_tx_frame[len++] = fault_batt.byte.lo;
        arm_tx_frame[len++] = fault_batt.all >> 8;
        arm_tx_frame[len++] = fault_pv.byte.lo;
        arm_tx_frame[len++] = fault_pv.all >> 8;
    } else if (page == 5) {
        len = 13;
        buf = arm_tx_frame;
        buf[len++] = bms_fc4_ready;
        if (bms_fc4_ready == 1) {
            buf[len++] = bms_fc4_subpage;
            if (bms_fc4_subpage <= 4) {
                for (bms_fc4_idx = 0; bms_fc4_idx <= 19; bms_fc4_idx++)
                    arm_tx_frame[len++] = bms_fc4_buf[bms_fc4_subpage * 20 + bms_fc4_idx];
                if (++bms_fc4_subpage > 4) {
                    bms_fc4_subpage = 0;
                    bms_fc4_ready = 0;
                }
            }
        }
        arm_tx_frame[35] = bms_lim_hr26 * 0.01f;
        arm_tx_frame[36] = bms_lim_hr27 * 0.01f;
    } else if (page == 6) {
        len = 13;
        arm_tx_frame[len++] = fault_stop1.byte.lo;
        arm_tx_frame[len++] = fault_stop1.all >> 8;
        arm_tx_frame[len++] = fault_stop2.byte.lo;
        arm_tx_frame[len++] = fault_stop2.all >> 8;
        arm_tx_frame[len++] = batt_blocks.byte.lo;
        arm_tx_frame[len++] = batt_blocks.all >> 8;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
        arm_tx_frame[len++] = 0;
    }
}

/* Reads a good 0x43 frame: the header bytes 1-7 (see the file header), then, for every byte
   index i from 1 to 42, calls the handler of the page named by byte 7. Byte 7 is read at i = 7,
   so the handler calls for i < 7 go to the previous frame's page; no handler has a case below 8.
   Run command values above 10 and battery states above 2 are ignored. */
void arm_frame_parse(void) {
    unsigned int i, j, b, x;
    for (i = 1; i < 43; i++) {
        j = i;
        b = arm_rx_frame[j] & 0xFF;
        switch (j) {
        case 4:
            b = arm_rx_frame[j] & 0xFF;
            if (b <= 2)
                batt_state_cmd = b;
            break;
        case 3:
            b = arm_rx_frame[j] & 0xFF;
            if (aging_test_mode == 2)
                b = 70;
            else if (aging_test_mode == 1) {
                if (pv2_voltage > 3000.0f)
                    b = 85;
                else
                    b = 41;
            } else if (b >= 105)
                break;
            active_power_rate = b;
            break;
        case 2:
            b = arm_rx_frame[j] & 0xFF;
            if (b & 4)
                fault_stop2.all |= 0x100;
            else
                fault_stop2.all &= 0xFEFF;
            if (b & 0x10)
                fault_stop2.all |= 0x200;
            else
                fault_stop2.all &= 0xFDFF;
            break;
        case 1:
            if (b <= 10)
                arm_run_command(b);
            break;
        case 7:
            arm_page_no = b < 8 ? b : 7;
            break;
        case 5:
            x = arm_rx_frame[j] << 8;
            x |= arm_rx_frame[++j] &
                 0xFF; /* j becomes 6, so the page handler gets 6 for i = 5 and i = 6 */
            arm_batt_setpoint = (long)(int)x * 10;
            break;
        }
        qu_q_max = 438; /* set again on every pass; nothing in this loop depends on it */
        if (arm_page_no < 8)
            arm_page_handlers[arm_page_no](j);
    }
}

/* The run command of byte 1. Command 4 (update) posts EV0_UPDATE to the state machine on the
   6th frame that carries it (run_cmd4_cnt is never cleared by other commands, so the frames need
   not be in a row). Command 10 posts EV0_STOP when the last command was not 10 and the state is
   not OP_IDLE. Every command is then stored by arm_run_mode_set (app_init.c). */
void arm_run_command(int k) {
    if (k == 4) {
        if (++run_cmd4_cnt > 5) {
            run_cmd4_cnt = 0;
            if (op_state != OP_UPDATE)
                os_post(TASK_STATE, EV0_UPDATE);
        }
    } else if (k == 10 && run_command != 10 && op_state != OP_IDLE) {
        os_post(TASK_STATE, EV0_STOP);
    }
    arm_run_mode_set(k);
}

/* Settings page 0 (bytes 8-42 of the 0x43 frame; A316 field in brackets, the offset there is
   byte - 7):
     8      grid_code 0..28; 20 and 21 become 3 (HR2 high byte)
     9      the model: 50 -> model_power_code 5, 46 -> 4, 36 -> 3, each clearing CF17 bit 10;
            any other value sets CF17 bit 10, FS1_BAD_PRESET (HR2 low byte)
     10-11  inverter_start_time (HR61)
     12-13  pv_start_voltage, 1200..1500 (HR60; the ARM sends 1550 during HR175)
     14     1: grid_func_flags bit 3, 2: bit 12, 3: both, else neither (HR125)
     15-16  each bit inverted: 0 iso_check_enable, 1 rcmu_enable, 2 dci_comp_enable,
            3 hr1_check3, 4 rcmu_step_check_off (HR1)
     17     not used (HR50)               18 reactive_power_rate, below 61 (HR51)
     19-20  power_factor_set (HR52)       21 batt_min_reserve (HR114)
     22     charge_active                 23 discharge_active
     24     battery_power_mode (HR27)     25 inverter_state (HR53)
     26     not used (g_200001B0)         27 q_mode (HR122)
     28     any value clears pv_bus_mode (g_200001D7)
     29-30  inverter_restart_delay (HR62)
     31-33  not used (0, HR33, settings +0x74)
     34     battery_type (HR54)
     35-36  battery_capacity (HR55); bms_capacity instead when battery_type is 1, enable_bms_read
            is set and bms_reply_count is 50 or more
     37-38  charger_temp     39-40 heatsink_temp     41-42 batt_temp (signed) */
void arm_page0(unsigned int i) {
    unsigned int b, x;
    switch (i) {
    case 0x23:
        x = arm_rx_frame[i] << 8;
        x |= arm_rx_frame[i + 1] & 0xFF;
        battery_capacity = x;
        if (battery_type == 1 && enable_bms_read != 0 && bms_reply_count >= 50)
            battery_capacity = bms_capacity;
        break;
    case 0x12:
        b = arm_rx_frame[i] & 0xFF;
        if (b < 61)
            reactive_power_rate = b;
        break;
    case 0xE:
        b = arm_rx_frame[i] & 0xFF;
        if (b == 1) {
            grid_func_flags.bit.b3 = 1;
            grid_func_flags.bit.b12 = 0;
        } else if (b == 2) {
            grid_func_flags.bit.b12 = 1;
            grid_func_flags.bit.b3 = 0;
        } else if (b == 3) {
            grid_func_flags.all |= 0x1008;
        } else {
            grid_func_flags.all &= 0xEFF7;
        }
        break;
    case 9:
        b = arm_rx_frame[i] & 0xFF;
        if (b == 50) {
            fault_stop1.all &= 0xFBFF;
            model_power_code = 5;
        } else if (b == 46) {
            fault_stop1.all &= 0xFBFF;
            model_power_code = 4;
        } else if (b == 36) {
            fault_stop1.all &= 0xFBFF;
            model_power_code = 3;
        } else {
            fault_stop1.all |= 0x400;
        }
        break;
    case 8:
        grid_code = arm_rx_frame[i] & 0xFF;
        limitu(&grid_code, 28, 0);
        if (grid_code == 20 || grid_code == 21)
            grid_code = 3;
        break;
    case 0x29:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        batt_temp = (long)(int)x;
        break;
    case 0x27:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        heatsink_temp = (long)(int)x;
        break;
    case 0x25:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        charger_temp = (long)(int)x;
        break;
    case 0x22:
        battery_type = arm_rx_frame[i] & 0xFF;
        break;
    case 0x1D:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        inverter_restart_delay = x;
        break;
    case 0x1C:
        pv_bus_mode = 0;
        break;
    case 0x1B:
        q_mode = arm_rx_frame[i] & 0xFF;
        break;
    case 0x19:
        inverter_state = arm_rx_frame[i] & 0xFF;
        break;
    case 0x18:
        battery_power_mode = arm_rx_frame[i] & 0xFF;
        break;
    case 0x17:
        discharge_active = arm_rx_frame[i] & 0xFF;
        break;
    case 0x16:
        charge_active = arm_rx_frame[i] & 0xFF;
        break;
    case 0x15:
        batt_min_reserve = arm_rx_frame[i] & 0xFF;
        break;
    case 0x13:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        power_factor_set = x;
        break;
    case 0xF:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        iso_check_enable = (x & 1) ^ 1;
        rcmu_enable = (x >> 1 & 1) ^ 1;
        dci_comp_enable = (x >> 2 & 1) ^ 1;
        hr1_check3 = (x >> 3 & 1) ^ 1;
        rcmu_step_check_off = (x >> 4 & 1) ^ 1;
        break;
    case 0xC:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pv_start_voltage = x;
        limitu(&pv_start_voltage, 1500, 1200);
        break;
    case 0xA:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        inverter_start_time = x;
        break;
    case 0xB:
    case 0xD:
    case 0x10:
    case 0x11:
    case 0x14:
    case 0x1A:
        break;
    }
}

/* Settings page 1, the grid protection limits (A316 field in brackets):
     8-11   p_eps_sample, 32 bits
     12-13  batt_v_low_limit = word x 0.1, x 0.5 while batt_cal_request, x 0.86 while
            batt_cal_stage; 200..480 (HR97)
     14-15  batt_v_high_limit = word x 0.1, x 1.05 while batt_cal_stage; 540..630 (HR98)
     16-17  v_ac_high_out 2300..3150 (HR64)    18-19 v_ac_low_out 0..2300 (HR63)
     20-21  f_ac_high_out (HR66)               22-23 f_ac_low_out (HR65)
     24-25  v_ac_high_in 2300..3150 (HR72)     26-27 v_ac_low_in 0..2300 (HR71)
     28-29  f_ac_high_in (HR74)                30-31 f_ac_low_in (HR73)
     32-33  v_ac_high_c (HR80)                 34-35 v_ac_low_c (HR79)
     36-37  v_ac_high_out_time (HR68)          38-39 v_ac_low_out_time (HR67)
     40-41  f_ac_high_out_time (HR70)
     42     auto_test_state, taken only while it is 0 (the ARM's auto test request) */
void arm_page1(unsigned int i) {
    unsigned int x;
    unsigned long v = 0;
    switch (i) {
    case 0x2A:
        if (auto_test_state == 0)
            auto_test_state = arm_rx_frame[i] & 0xFF;
        break;
    case 0x28:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_high_out_time = x;
        break;
    case 0x26:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_low_out_time = x;
        break;
    case 0x24:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_high_out_time = x;
        break;
    case 0x22:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_low_c = x;
        break;
    case 0x20:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_high_c = x;
        break;
    case 0x1E:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_low_in = x;
        break;
    case 0x1C:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_high_in = x;
        break;
    case 0x1A:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_low_in = x;
        limitu(&v_ac_low_in, 2300, 0);
        break;
    case 0x18:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_high_in = x;
        limitu(&v_ac_high_in, 3150, 2300);
        break;
    case 0x16:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_low_out = x;
        break;
    case 0x14:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_high_out = x;
        break;
    case 0x12:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_low_out = x;
        limitu(&v_ac_low_out, 2300, 0);
        break;
    case 0x10:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_high_out = x;
        limitu(&v_ac_high_out, 3150, 2300);
        break;
    case 0xE:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        if (batt_cal_stage != 0)
            batt_v_high_limit = x * 0.1f * 1.05f;
        else
            batt_v_high_limit = x * 0.1f;
        limitu(&batt_v_high_limit, 630, 540);
        break;
    case 0xC:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        if (batt_cal_request != 0)
            batt_v_low_limit = x * 0.1f * 0.5f;
        else if (batt_cal_stage != 0)
            batt_v_low_limit = x * 0.1f * 0.86f;
        else
            batt_v_low_limit = x * 0.1f;
        limitu(&batt_v_low_limit, 480, 200);
        break;
    case 8:
        v += (unsigned long)arm_rx_frame[i] << 24;
        i++;
        v += (unsigned long)arm_rx_frame[i] << 16 & 0xFF0000UL;
        i++;
        v += (unsigned)(arm_rx_frame[i] << 8);
        i++;
        v += arm_rx_frame[i] & 0xFF;
        p_eps_sample = v;
        break;
    }
}

/* Settings page 2 (A316 field in brackets):
     8-9    f_ac_low_out_time (HR69)           10-11 v_ac_high_in_time (HR76)
     12-13  v_ac_low_in_time (HR75)            14-15 f_ac_high_in_time (HR78)
     16-17  f_ac_low_in_time (HR77)            18-19 f_ac_high_c (HR82)
     20-21  f_ac_low_c (HR81)                  22 cfg_changed_flags
     23     dci_trip_time (grid_country_byte_a)   24 not used (HR128)
     25     ofr_droop 1..100 (HR123)           26-27 v_10_min_protection 2300..2900 (HR83)
     28     freq_stage_sel (HR121)             29 qu_lock_out_p (HR143)
     30     qu_lock_in_p (HR142)               31-32 pfp_lock_in_v (HR144)
     33-34  pfp_lock_out_v (HR145)
     35-36  grid_port_max_power = word x 10, at least 1200 (HR26)
     37     0 = 50 Hz, 1 = 60 Hz: grid_freq_nominal, grid_omega_step and eps_cycle_len, only
            below OP_UPDATE (HR28)
     38     batt_cal_stage (HR29)
     39     aging_test_enable; a change from 0 to 1 posts EV0_STOP (HR332)
     40     not used (ct_cycle_count)
     41-42  ofr_recover_freq 5000..5400, or 6000..6400 at 60 Hz (HR328)
   Byte 39 posts with os_post_isr from task code, without DINT; see docs/findings-comm.md. */
void arm_page2(unsigned int i) {
    unsigned int x = 0;
    switch (i) {
    case 0x29:
        x = arm_rx_frame[i] << 8;
        x |= arm_rx_frame[i + 1] & 0xFF;
        if (grid_is_60hz == 1)
            limitu(&x, 6400, 6000);
        else
            limitu(&x, 5400, 5000);
        ofr_recover_freq = x;
        break;
    case 0x27:
        x = arm_rx_frame[i] & 0xFF;
        if (x == 1 && aging_test_enable == 0)
            os_post_isr(TASK_STATE, EV0_STOP);
        aging_test_enable = x;
        break;
    case 0x25:
        x = arm_rx_frame[i] & 0xFF;
        if (x < 2 && op_state < OP_UPDATE) {
            if (x == 0)
                grid_freq_nominal = 5000.0f;
            else
                grid_freq_nominal = 6000.0f;
            grid_omega_step = 6.2831855f * grid_freq_nominal / 19200.0f * 0.01f;
            DINT;
            if (grid_freq_nominal == 5000.0f)
                eps_cycle_len = 0x180;
            else
                eps_cycle_len = 0x140;
            EINT;
        }
        break;
    case 0x28:
    case 0x18:
        x = arm_rx_frame[i] & 0xFF;
        break;
    case 0x26:
        x = arm_rx_frame[i] & 0xFF;
        batt_cal_stage = x;
        break;
    case 0x23:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        grid_port_max_power = x * 10UL;
        if (grid_port_max_power < 1200.0f)
            grid_port_max_power = 1200.0f;
        break;
    case 0x21:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pfp_lock_out_v = x;
        break;
    case 0x1F:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pfp_lock_in_v = x;
        break;
    case 0x1E:
        x = arm_rx_frame[i] & 0xFF;
        qu_lock_in_p = x;
        break;
    case 0x1D:
        x = arm_rx_frame[i] & 0xFF;
        qu_lock_out_p = x;
        break;
    case 0x1C:
        x = arm_rx_frame[i] & 0xFF;
        freq_stage_sel = x;
        break;
    case 0x1A:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_10_min_protection = x;
        limitu(&v_10_min_protection, 2900, 2300);
        break;
    case 0x19:
        x = arm_rx_frame[i] & 0xFF;
        limitu(&x, 100, 1);
        ofr_droop = x;
        break;
    case 0x17:
        x = arm_rx_frame[i] & 0xFF;
        dci_trip_time = x;
        break;
    case 0x16:
        x = arm_rx_frame[i] & 0xFF;
        cfg_changed_flags = x;
        break;
    case 0x14:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_low_c = x;
        break;
    case 0x12:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_high_c = x;
        break;
    case 0x10:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_low_in_time = x;
        break;
    case 0xE:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_high_in_time = x;
        break;
    case 0xC:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_low_in_time = x;
        break;
    case 0xA:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        v_ac_high_in_time = x;
        break;
    case 8:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        f_ac_low_out_time = x;
        break;
    }
}

/* Settings page 3 (A316 field in brackets):
     8-9    not used (HR130)                   10-11 pf_curve_pf1 (HR131)
     12-13  not used (HR132)                   14-15 pf_curve_pf2 (HR133)
     16-17  not used (HR134)                   18-19 pf_curve_pf3 (HR135)
     20-21  not used (HR136)                   22-23 pf_curve_pf4 (HR137)
     24-25  qu_v_high1 (HR138)                 26-27 qu_v_high2 (HR139)
     28-29  qu_v_low1 (HR140)                  30-31 qu_v_low2 (HR141)
     32     dci_trip_time_it (grid_country_byte_b)
     33-34  ofr_start_freq 5000..5400, or 6000..6400 at 60 Hz (HR321)
     35-36  peek_addr, clamped to 0xC000..0xE000 (HR24): the RAM word sent in bytes 10-11
     37-38  not used (sent as 0)
     39-40  ufr_start_freq 4600..5000, or 5600..6000 at 60 Hz (HR326)
     41     ufr_droop 1..100 (HR327) */
void arm_page3(unsigned int i) {
    unsigned int x = 0;
    switch (i) {
    case 0x27:
        x = arm_rx_frame[i] << 8;
        x |= arm_rx_frame[i + 1] & 0xFF;
        if (grid_is_60hz == 1)
            limitu(&x, 6000, 5600);
        else
            limitu(&x, 5000, 4600);
        ufr_start_freq = x;
        break;
    case 0x23:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        peek_addr = x;
        if (x < 0xC000)
            peek_addr = 0xC000;
        else if (x > 0xE000)
            peek_addr = 0xE000;
        break;
    case 0x21:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        if (grid_is_60hz == 1)
            limitu(&x, 6400, 6000);
        else
            limitu(&x, 5400, 5000);
        ofr_start_freq = x;
        break;
    case 0x29:
        x = arm_rx_frame[i] & 0xFF;
        limitu(&x, 100, 1);
        ufr_droop = x;
        break;
    case 0x25:
    case 0x26:
        x = arm_rx_frame[i] & 0xFF;
        break;
    case 0x20:
        x = arm_rx_frame[i] & 0xFF;
        dci_trip_time_it = x;
        break;
    case 0x1E:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        qu_v_low2 = x;
        break;
    case 0x1C:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        qu_v_low1 = x;
        break;
    case 0x1A:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        qu_v_high2 = x;
        break;
    case 0x18:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        qu_v_high1 = x;
        break;
    case 0x16:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pf_curve_pf4 = x;
        break;
    case 0x14:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x12:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pf_curve_pf3 = x;
        break;
    case 0x10:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0xE:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pf_curve_pf2 = x;
        break;
    case 0xC:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0xA:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        pf_curve_pf1 = x;
        break;
    }
}

/* Settings page 4 (A316 field in brackets):
     8      not used (ct_cycle_count)          9 grid_func_flags bit 4 (HR124)
     10-11  not used (g_200001A4)
     12-13  dci_limit 30..1100, 1000 while rcmu_step_check_off (settings +0x69)
     14-15  dci_limit_b, the same (settings +0x6B)
     16     batt_cal_request (HR175). 0, or 1 while pv_voltage_max is below 1600, clears it and
            low_batt_wake. Otherwise low_batt_wake follows batt_voltage (0.1 V units) with
            200-frame debounces: from 1 to 0 after 200 frames above 460, from 0 to 1 after 200
            frames between 400 and 460; with no BMS reply (CF15 bit 9) or below 400 it takes the
            byte's value.
     17     enable_bms_read (HR109)
     18     bit 0 -> grid_func_flags bit 15, bit 1 -> bit 6, bit 2 -> bit 8 (HR176)
     19     battery_charge_limit 1..100 (HR111)
     20     battery_discharge_limit 1..100 (HR112)
     21     discharge_min_soc 1..100 (discharge_min_soc_now)
     22     grid_func_flags bit 11 (HR115)     23-24 not used (pv_power_override)
     25     eps_cfg_bits bit 10 (HR177)        26 bms_update_pending
     27     hr200_request (HR200)              28 batt_blocks bit 2 (the meter link state) */
void arm_page4(unsigned int i) {
    unsigned int b, c;
    switch (i) {
    case 0x1C:
        if ((arm_rx_frame[i] & 0xFF) == 0)
            batt_blocks.bit.b2 = 0;
        else
            batt_blocks.bit.b2 = 1;
        break;
    case 0x10:
        c = pv_voltage_max < 1600.0f;
        b = arm_rx_frame[i] & 0xFF;
        if ((c && b == 1) || b == 0) {
            batt_cal_request = 0;
            low_batt_wake = 0;
        } else {
            if (!fault_batt.bit.b9 && batt_voltage > 460.0f && low_batt_wake == 1) {
                cal_highv_cnt++;
                cal_lowv_cnt = 0;
                if (cal_highv_cnt > 200) {
                    cal_highv_cnt = 0;
                    low_batt_wake = 0;
                }
            } else if (!fault_batt.bit.b9 && batt_voltage > 400.0f && low_batt_wake == 0) {
                if (batt_voltage > 460.0f) {
                    cal_lowv_cnt = 0;
                } else if (++cal_lowv_cnt > 200) {
                    cal_lowv_cnt = 0;
                    low_batt_wake = 1;
                }
            } else {
                cal_highv_cnt = 0;
                low_batt_wake = b;
            }
            batt_cal_request = b;
        }
        break;
    case 0xE:
        b = arm_rx_frame[i++] << 8;
        b |= arm_rx_frame[i] & 0xFF;
        dci_limit_b = b;
        if (rcmu_step_check_off == 1)
            dci_limit_b = 1000;
        limitu(&dci_limit_b, 1100, 30);
        break;
    case 0xC:
        b = arm_rx_frame[i++] << 8;
        b |= arm_rx_frame[i] & 0xFF;
        dci_limit = b;
        if (rcmu_step_check_off == 1)
            dci_limit = 1000;
        limitu(&dci_limit, 1100, 30);
        break;
    case 0x1B:
        hr200_request = arm_rx_frame[i] & 0xFF;
        break;
    case 0x1A:
        bms_update_pending = arm_rx_frame[i] & 0xFF;
        break;
    case 0x19:
        eps_cfg_bits.bit.b10 = arm_rx_frame[i];
        break;
    case 0x16:
        grid_func_flags.bit.b11 = arm_rx_frame[i];
        break;
    case 0x15:
        discharge_min_soc = arm_rx_frame[i] & 0xFF;
        limitu(&discharge_min_soc, 100, 1);
        break;
    case 0x14:
        battery_discharge_limit = arm_rx_frame[i] & 0xFF;
        limitu(&battery_discharge_limit, 100, 1);
        break;
    case 0x13:
        battery_charge_limit = arm_rx_frame[i] & 0xFF;
        limitu(&battery_charge_limit, 100, 1);
        break;
    case 0x12:
        b = arm_rx_frame[i] & 0xFF;
        grid_func_flags.bit.b15 = b;
        grid_func_flags.bit.b6 = b >> 1;
        grid_func_flags.bit.b8 = b >> 2;
        break;
    case 0x11:
        enable_bms_read = arm_rx_frame[i] & 0xFF;
        break;
    case 9:
        grid_func_flags.bit.b4 = arm_rx_frame[i];
        break;
    }
}

/* Settings page 5, calibration gains. Each is clamped to 870..1178:
     8-9    cal_pv1_v       12-13 cal_pv1_i and cal_pv1_p     16-17 cal_pv2_v
     20-21  cal_pv2_i and cal_pv2_p                           24-25 cal_bus_v
     28-29  cal_grid_v
     42     hr201_request (HR201)
   The other words (10, 14, 18, 22, 26, 30, 32, 36, 39 and the bytes 38 and 41) are read and
   dropped; the ARM fills them with its other calibration gains and offsets. */
void arm_page5(unsigned int i) {
    unsigned int x = 0;
    switch (i) {
    case 0x2A:
        x = arm_rx_frame[i] & 0xFF;
        hr201_request = x;
        break;
    case 0x26:
    case 0x29:
        x = arm_rx_frame[i] & 0xFF;
        break;
    case 0x27:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x24:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x20:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x1E:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x1C:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        cal_grid_v = x;
        limitu(&cal_grid_v, 1178, 870);
        break;
    case 0x1A:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x18:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        cal_bus_v = x;
        limitu(&cal_bus_v, 1178, 870);
        break;
    case 0x16:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x14:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        limitu(&x, 1178, 870);
        cal_pv2_i = x;
        cal_pv2_p = x;
        break;
    case 0x12:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0x10:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        limitu(&x, 1178, 870);
        cal_pv2_v = x;
        break;
    case 0xE:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 0xC:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        limitu(&x, 1178, 870);
        cal_pv1_i = x;
        cal_pv1_p = x;
        break;
    case 0xA:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        break;
    case 8:
        x = arm_rx_frame[i++] << 8;
        x |= arm_rx_frame[i] & 0xFF;
        cal_pv1_v = x;
        limitu(&cal_pv1_v, 1178, 870);
        break;
    }
}

/* the page handlers, by the page number of the ARM's request (task5_arm_link); pages 6 and 7
 * are in arm_page_write.c */

void (*const arm_page_handlers[8])(unsigned int i) = {arm_page0, arm_page1, arm_page2, arm_page3,
                                                      arm_page4, arm_page5, arm_page6, arm_page7};
