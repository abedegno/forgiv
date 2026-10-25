// clang-format off
/* The BMS Modbus master on SCI-B (RTOS task 6, every 40 ms): builds the requests, checks the
   replies' CRC, parses the holding registers.
   at: bms_parse_hr 0x3F0551 .. batt_warning_update 0x3F08ED; bms_relay_idle .. bms_tx_buf

   Job: the DSP is the Modbus RTU master of the battery (slave 1, 8N1 at 9600 baud per the
   research notes, RS485 with the driver enable on GPIO57). Every 6th tick (240 ms) it sends one
   request: an FC3 poll of the holding registers, or an FC6 write (wake HR1, HR2, HR3, HR4), or a
   6-byte request the ARM forwards. Replies are taken from the SCI-B receive ring on every wake-up
   of the task. Entry points: task6_bms_link (created by main); bms_lost and batt_warning_update are
   also called from here only. bms_fc4_reply hands FC4 replies to the ARM link.
   Data it owns: the BMS state words below, transmit buffer bms_tx_buf at 0xD940,
   and receive buffer bms_rx_buf at 0xD9C0. It writes the BMS page 0xD500 (bms.h: HR19, HR20 bits, SoC, pack V/I/T, the HR26/HR27
   limits) and the BMS words of dsp_settings (0xD8C0 .. 0xD8E8).
   Neighbours: sci.c sends bms_tx_buf from its interrupt and sets bms_tx_state; arm_link.c and
   arm_page_write.c set the requests (batt_cal_request, hr200/hr201_request, bms_update_pending,
   arm_bms_request) and read bms_present, bms_alarm, batt_warning; power_limits.c and op_states.c
   use the page. docs/battery.md describes the whole battery path.
   Units: HR22 0.01 V (stored as 0.1 V), HR23 and the limits 0.01 A; HR24 is stored x10.
   name: descriptive, kept from the matching (map/filenames.tsv: src/bms_link.c). */
// clang-format on
#define OWN_fault_words /* match: the fault words 0xCF12.. by name (struct fault_words_t); op_states.c defines them as union bits16_u[8] and the header has that form */
#include "F2806x_Device.h"
#include "util.h"
#include "os.h"
#include "sci.h"
#include "settings.h"
#include "faults.h"
#include "bms.h"
#include "arm_link.h"
#include "control.h"
#include "grid.h"
#define volatile volatile

#include "F2806x_Sci.h"
#include "F2806x_Gpio.h"
#include "power_limits.h"
extern struct fault_words_t fault_words; /* match: own form, see OWN_fault_words */

// clang-format off
/* ebss-begin */
unsigned int bms_rx_len = 0;  /* bytes of the reply received so far */
unsigned int bms_poll_start = 0;  /* first register of the poll (0 or 17) */
unsigned int bms_lost_count = 0;  /* 0xD915: ticks since the last good reply, 750 = lost */
unsigned int bms_alarm_masked = 0;  /* HR20 & 0x00F3; written, never read */
unsigned int bms_tx_len = 0;  /* request length (8) */
unsigned int bms_tx_pos = 0;
unsigned int bms_rx_expect = 0;  /* expected reply length in bytes */
unsigned int bms_fc4_count = 0;  /* register count of the ARM request (at most 26) */
unsigned int bms_reply_count = 0;  /* good replies since the last loss, stops at 50 */
unsigned int bms_hr17 = 0;  /* HR17; written, never read */
unsigned int bms_hr18 = 0;  /* HR18; written, never read */
unsigned int bms_alarm = 0;  /* HR20 as read; the ARM gets its low byte */
unsigned int bms_tx_state = 0;  /* 1: sci.c sent the last byte; 2: GPIO57 cleared */
unsigned int batt_cal_stage = 0;  /* 0xD907: HR29 battery calibration stage (ARM page 2), 0 = none */
unsigned int low_batt_wake = 0;  /* 0xD90A: low-battery wake with batt_cal_request (arm_link.c) */
unsigned int batt_cal_request = 0;  /* 0xD909: HR175 from the ARM; 1 forces charging (power_limits.c) */
unsigned int batt_cal_request_prev = 0;
unsigned int bms_relay_active = 0;  /* 1: the ARM talks to the BMS through the DSP (sci.c), this task is idle */
unsigned int bms_update_pending = 0;  /* value for HR2 (ARM page 4 byte 0x1A) */
unsigned int bms_relay_idle = 0;  /* passes of BMS pass-through (power_limits.c resets the DSP after 15000) */
unsigned int bms_present = 1;  /* 0xD902: a good reply since the last loss (to the ARM) */
unsigned int hr200_request = 0;  /* value for HR3 (ARM page 4 byte 0x1B) */
unsigned int batt_warning = 0;  /* 0xD903: see batt_warning_update */
unsigned int hr201_request = 0;  /* 1: write HR4 = 1 (ARM page 5 byte 0x2A) */
volatile unsigned int bms_hr15;  /* HR15, see bms_parse_hr */
volatile union bits16_u bms_hr1_request; /* D912: desired HR1 bits supplied by the ARM. */
union bits16_u bms_hr15_edge; /* D913: HR15 current/previous state and last sent HR1 bit 2. */
unsigned int bms_poll_count = 0;  /* 0xD900: registers in the current FC3 poll (28 or 9) */
unsigned int bms_crc = 0;  /* last CRC computed */
unsigned int bms_poll_div = 0;  /* ticks 0..5 between requests */
unsigned int bms_wake_count = 0;
unsigned int bms_wake_on = 0;  /* sending HR1 = 1 every 21 polls */
unsigned int bms_wake_off = 0;  /* sending HR1 = 0 five times */
unsigned int bms_wake_off_count = 0;
unsigned int bms_hr2_count = 0;  /* polls between HR2 writes */
unsigned int bms_hr3_count = 0;  /* polls between HR3 writes */
volatile unsigned int bms_tx_buf[100] = {0};   /* the request being sent (volatile: the image stores each 0 as an immediate) */
unsigned int bms_rx_buf[100] = {0};  /* 0xD9C0: the reply, one byte per word */
/* ebss-end */
// clang-format on

/* one holding register of the reply: big-endian, HR n of a poll that starts at HR s is at bytes
   3 + 2 (n - s) */
#define RX16(i) ((bms_rx_buf[i] << 8) + bms_rx_buf[(i) + 1])

/* Parse an FC3 reply whose CRC is good. The full poll (bms_full_poll 1, HR0 .. HR27) also gives
   HR0, the serial HR5 .. HR9, HR11 capacity (capped at 10000), HR13 firmware version, HR14 and
   HR15, and its HR17 .. fields start 0x22 bytes in; the short poll (HR17 .. HR25) starts at byte 3.
   - HR15: 0xFFFF reads as 0; bit 0 is kept only while the last SoC is 100 or more.
   - The first 50 replies after a loss each clear the comms fault CF15 bit 9 (and post a stop to
     task 0 while batt_cal_request is set); bms_reply_count stops at 50.
   - HR20 (alarms): bit 2 sets 0xD500 bit 1 (used as "full"), bit 3 sets bit 2 ("empty").
   - SoC is the low byte of HR21. During a calibration (batt_cal_stage) it is held to
     discharge_min_soc + 1 .. 99.
   - Limits: BMS firmware above 3010 gives HR26 (charge) and HR27 (discharge); 3010 and below gives
     HR25 for both. During a calibration both are raised to at least 800 (8.00 A).
   The short poll does not contain HR26/HR27: with firmware above 3010 they are read from the CRC
   and stale bytes past the 23-byte reply (docs/findings-power.md). */
void bms_parse_hr(void) {
    unsigned int i, base, w;

    base = 0;
    i = 0;
    if (dsp_settings.bms_full_poll == 0) {
        base = 0;
    } else if (dsp_settings.bms_full_poll == 1) {
        base = 0x22;
        dsp_settings.bms_hr0 = RX16(3);
        for (i = 0; i < 10; i++)
            bms_serial[i] = bms_rx_buf[13 + i];
        dsp_settings.bms_capacity = RX16(25);
        if (dsp_settings.bms_capacity > 10000)
            dsp_settings.bms_capacity = 10000;
        dsp_settings.bms_fw_version = RX16(29);
        dsp_settings.bms_hr14 = RX16(31);
        w = RX16(33);
        if (w == 0xFFFF)
            w = 0;
        else if (bms.soc < 100)
            w &= 0xFFFE;
        bms_hr15 = w;
        bms_hr15_edge.bit.b0 = ((bms_hr15 >> 1) | bms_hr15) >> 1;
        bms_hr15_copy = w;
        if (bms_hr15_edge.bit.b1 > bms_hr15_edge.bit.b0)
            bms_request_flags.bit.b2 = 1;
        bms_hr15_edge.bit.b1 = bms_hr15_edge.bit.b0;
    }
    if (bms_reply_count < 50) {
        bms_reply_count++;
        fault_words.fault_batt &= ~FB_BMS_COMMS;
        if (batt_cal_request != 0)
            os_post(TASK_STATE, EV0_STOP);
    }
    bms_hr17 = RX16(base + 3);
    bms_hr18 = RX16(base + 5);
    bms.hr19_status.all = RX16(base + 7);
    bms_alarm_masked = RX16(base + 9);
    bms_alarm = bms_alarm_masked;
    bms_alarm_masked &= 0x00F3;
    if (bms_alarm & 0x0004)
        bms.alarm_flags.all |= 0x0002;
    else
        bms.alarm_flags.all &= ~0x0002;
    if (bms_alarm & 0x0008)
        bms.alarm_flags.all |= 0x0004;
    else
        bms.alarm_flags.all &= ~0x0004;
    dsp_settings.bms_soc_raw = bms_rx_buf[base + 12];
    bms.soc = dsp_settings.bms_soc_raw;
    if (batt_cal_stage != 0) {
        if (dsp_settings.bms_soc_raw < dsp_settings.discharge_min_soc + 1)
            bms.soc = dsp_settings.discharge_min_soc + 1;
        else if (dsp_settings.bms_soc_raw > 99)
            bms.soc = 99;
    }
    bms.pack_voltage = (int)RX16(base + 13) * 0.1f;
    bms.pack_current = RX16(base + 15);
    bms.pack_temp_x10 = RX16(base + 17) * 10;
    if (dsp_settings.bms_fw_version > 3010) {
        bms.bms_charge_limit = RX16(base + 21);
        bms.bms_discharge_limit = RX16(base + 23);
    } else {
        bms.bms_charge_limit = bms.bms_discharge_limit = RX16(base + 19);
    }
    if (batt_cal_stage != 0) {
        if (bms.bms_charge_limit < 800)
            bms.bms_charge_limit = 800;
        if (bms.bms_discharge_limit < 800)
            bms.bms_discharge_limit = 800;
    }
}

/* Copy an FC4 (input register) reply, which only the ARM asks for, to bms_fc4_buf for the ARM
   link, unless the last one has not been taken yet (bms_fc4_ready). */
void bms_fc4_reply(void) {
    unsigned int i = 0;
    if (bms_fc4_ready == 0) {
        bms_fc4_ready = 1;
        for (i = 0; i < 100; i++)
            bms_fc4_buf[i] = bms_rx_buf[i];
    }
}

/* RTOS task 6, every 40 ms. Waits while the ARM link is down (CF18 bit 2), then loops:
   - Receive, on every wake-up unless the ARM has the bus (bms_relay_active): byte 0 must be under
     0x10 (any address 0..15 is accepted), byte 1 FC3 or FC4, then bytes up to the expected length
     (FC3: 2 x bms_poll_count + 5, FC4: 2 x (bms_fc4_count + 3)). At that length the CRC is checked;
     a good reply sets bms_present, clears bms_lost_count and goes to bms_parse_hr or bms_fc4_reply.
     Anything else (FC6 echoes included) restarts the frame.
   - On each tick: 750 ticks (30 s) with no good reply calls bms_lost. Every 6th tick (240 ms) one
     request goes out, in this order of priority:
     wake HR1 = 1 once every 21 polls (5 s) after batt_cal_request rises;
     HR1 = 0 on 5 polls (2, 4 .. 10) after it falls;
     HR4 = 1 while hr201_request is 1 (this branch keeps the last bms_tx_len, see findings);
     HR2 = bms_update_pending every 11th poll, HR3 = hr200_request every 22nd poll;
     the ARM's forwarded request; else the FC3 poll (full or short by bms_full_poll).
     The fall of batt_cal_request also sets CF15 bit 3 and posts a stop to task 0.
   - GPIO57 (RS485 driver enable) is set before sending and cleared on the tick after sci.c
     reports the last byte gone (bms_tx_state 1 -> 2), so up to one tick after the request. */
void task6_bms_link(void) {
    unsigned int ev, i;
    unsigned int c;
    unsigned int *rx;
    unsigned int *src;

    while (fault_words.fault_stop2.bit.b2)
        os_wait(0);
    rx = (unsigned int
              *)0xD9C0; /* the reply buffer bms_rx_buf by address: the image keeps it in XAR2 */
    for (;;) {
        ev = os_wait(0);
        if (bms_relay_active != 0)
            continue;
        while (ring_read(1, &c) == 0) {
            switch (bms_rx_len) {
            case 0:
                if (c < 0x10) {
                    bms_rx_buf[bms_rx_len] = c;
                    if (bms_rx_len < 100)
                        bms_rx_len++;
                    else
                        bms_rx_len = 0;
                } else
                    bms_rx_len = 0;
                break;
            case 1:
                if (c == MB_READ_HOLDING || c == MB_READ_INPUT) {
                    bms_rx_buf[bms_rx_len] = c;
                    if (bms_rx_len < 100)
                        bms_rx_len++;
                    else
                        bms_rx_len = 0;
                } else
                    bms_rx_len = 0;
                break;
            default:
                if (bms_rx_buf[1] == MB_READ_HOLDING || bms_rx_buf[1] == MB_READ_INPUT) {
                    if (bms_rx_buf[1] == MB_READ_HOLDING)
                        bms_rx_expect = (bms_poll_count << 1) + 5;
                    else if (bms_rx_buf[1] == MB_READ_INPUT)
                        bms_rx_expect = (unsigned long)(bms_fc4_count + 3) << 1;
                } else {
                    bms_rx_len = 0;
                    ring_reset(1, 60);
                    break;
                }
                if (bms_rx_len < bms_rx_expect) {
                    bms_rx_buf[bms_rx_len] = c;
                    bms_rx_len++;
                } else
                    bms_rx_len = 0;
                if (bms_rx_expect >= 100)
                    bms_rx_expect = 0;
                if (bms_rx_len == bms_rx_expect) {
                    bms_crc = CRC16(rx, bms_rx_expect - 2);
                    if ((bms_crc >> 8) == rx[bms_rx_expect - 1] &&
                        (bms_crc & 0xFF) == rx[bms_rx_expect - 2]) {
                        bms_tx_state = 0;
                        bms_present = 1;
                        if (rx[1] == MB_READ_INPUT) {
                            bms_fc4_reply();
                            bms_rx_len = 0;
                        } else {
                            bms_parse_hr();
                            bms_rx_len = 0;
                        }
                        bms_lost_count = 0;
                    } else
                        bms_rx_len = 0;
                }
                break;
            }
        }
        if (!(ev & EV_TICK_M))
            continue;
        if (bms_relay_active == 0) {
            if (++bms_lost_count >= 750) {
                bms_lost_count = 0;
                ring_reset(1, 60);
                bms_lost();
            }
            if (bms_poll_div < 5)
                bms_poll_div++;
            else {
                bms_poll_div = 0;
                bms_rx_len = 0;
                ring_reset(1, 60);
                if (dsp_settings.bms_full_poll == 0) {
                    bms_poll_start = BMS_POLL_SHORT_START;
                    bms_poll_count = BMS_POLL_SHORT_COUNT;
                } else if (dsp_settings.bms_full_poll == 1) {
                    bms_poll_start = 0;
                    bms_poll_count = BMS_POLL_FULL_COUNT;
                }
                for (i = 0; i < 100; i++)
                    bms_tx_buf[i] = 0;
                if (ScibRegs.SCIHBAUD !=
                    2) { /* always true: only device_init.c writes SCIHBAUD, with 1 */
                    if (bms_wake_on == 1) {
                        if (++bms_wake_count >= 21)
                            bms_wake_count = 0;
                    } else
                        bms_wake_count = 0;
                    if (bms_wake_off == 1) {
                        if (++bms_wake_off_count >= 11) {
                            bms_wake_off = 0;
                            bms_wake_off_count = 0;
                        }
                    } else
                        bms_wake_off_count = 0;
                    GpioDataRegs.GPBSET.bit.GPIO57 = 1;
                    if (bms_wake_count == 20) {
                        bms_tx_len = 8;
                        bms_tx_buf[0] = BMS_SLAVE;
                        bms_tx_buf[1] = MB_WRITE_REGISTER;
                        bms_tx_buf[2] = 0;
                        bms_tx_buf[3] = BMS_HR_WAKE;
                        bms_tx_buf[4] = 0;
                        bms_tx_buf[5] = 1;
                    } else if (bms_wake_off_count == 2 || bms_wake_off_count == 4 ||
                               bms_wake_off_count == 6 || bms_wake_off_count == 8 ||
                               bms_wake_off_count == 10) {
                        bms_tx_len = 8;
                        bms_tx_buf[0] = BMS_SLAVE;
                        bms_tx_buf[1] = MB_WRITE_REGISTER;
                        bms_tx_buf[2] = 0;
                        bms_tx_buf[3] = BMS_HR_WAKE;
                        bms_tx_buf[4] = 0;
                        bms_tx_buf[5] = 0;
                    } else if (hr201_request == 1) {
                        bms_tx_buf[0] = BMS_SLAVE;
                        bms_tx_buf[1] = MB_WRITE_REGISTER;
                        bms_tx_buf[2] = 0;
                        bms_tx_buf[3] = BMS_HR_201;
                        bms_tx_buf[4] = 0;
                        bms_tx_buf[5] = 1;
                    } else if (bms_hr15_edge.bit.b2 != bms_hr1_request.bit.b2) {
                        bms_tx_len = 8;
                        bms_tx_buf[0] = BMS_SLAVE;
                        bms_tx_buf[1] = MB_WRITE_REGISTER;
                        bms_tx_buf[2] = 0;
                        bms_tx_buf[3] = BMS_HR_WAKE;
                        bms_tx_buf[4] = 0;
                        bms_tx_buf[5] = bms_hr1_request.byte.lo;
                        bms_hr15_edge.bit.b2 = bms_hr1_request.bit.b2;
                    } else {
                        if (batt_cal_request == 1 && batt_cal_request_prev == 0) {
                            bms_wake_on = 1;
                            bms_wake_off = 0;
                        }
                        if (batt_cal_request == 0 && batt_cal_request_prev == 1) {
                            bms_wake_off = 1;
                            bms_wake_on = 0;
                            fault_words.fault_batt |= FB_CAL_END;
                            os_post(TASK_STATE, EV0_STOP);
                        }
                        batt_cal_request_prev = batt_cal_request;
                        if (bms_update_pending != 0 && bms_hr2_count++ >= 10) {
                            bms_hr2_count = 0;
                            bms_tx_len = 8;
                            bms_tx_buf[0] = BMS_SLAVE;
                            bms_tx_buf[1] = MB_WRITE_REGISTER;
                            bms_tx_buf[2] = 0;
                            bms_tx_buf[3] = BMS_HR_UPDATE;
                            bms_tx_buf[4] = 0;
                            bms_tx_buf[5] = bms_update_pending;
                        } else if (hr200_request != 0 && bms_hr3_count++ > 20) {
                            bms_hr3_count = 0;
                            bms_tx_len = 8;
                            bms_tx_buf[0] = BMS_SLAVE;
                            bms_tx_buf[1] = MB_WRITE_REGISTER;
                            bms_tx_buf[2] = 0;
                            bms_tx_buf[3] = BMS_HR_200;
                            bms_tx_buf[4] = 0;
                            bms_tx_buf[5] = hr200_request;
                        } else if (arm_bms_request_pending == 1) {
                            bms_tx_len = 8;
                            src = (unsigned int
                                       *)0xD60A; /* arm_bms_request, a request from the ARM */
                            bms_tx_buf[0] = src[0];
                            bms_tx_buf[1] = src[1];
                            bms_tx_buf[2] = src[2];
                            bms_tx_buf[3] = src[3];
                            bms_tx_buf[4] = src[4];
                            bms_tx_buf[5] = src[5];
                            arm_bms_request_pending = 0;
                            bms_fc4_count = (src[4] << 8) + src[5];
                            if (bms_fc4_count >= 26)
                                bms_fc4_count = 26;
                        } else {
                            bms_tx_len = 8;
                            bms_tx_buf[0] = BMS_SLAVE;
                            bms_tx_buf[1] = MB_READ_HOLDING;
                            bms_tx_buf[2] = 0;
                            bms_tx_buf[3] = bms_poll_start;
                            bms_tx_buf[4] = 0;
                            bms_tx_buf[5] = bms_poll_count;
                        }
                    }
                    bms_crc = CRC16((unsigned int *)bms_tx_buf, bms_tx_len - 2);
                    bms_tx_buf[bms_tx_len - 2] = bms_crc & 0xFF;
                    bms_tx_buf[bms_tx_len - 1] = bms_crc >> 8;
                    bms_tx_pos = 0;
                    sci_tx(1);
                }
            }
        }
        batt_warning_update();
        if (bms_tx_state == 1) {
            bms_tx_state = 2;
            GpioDataRegs.GPBCLEAR.bit.GPIO57 = 1;
        } else if (bms_tx_state == 2)
            bms_tx_state = 0;
    }
}

/* No good reply for 750 ticks (30 s): zero what the battery reported (pack V, I, T, SoC, the
   HR26/HR27 limits, HR17, HR18, HR20), clear bms_present and bms_reply_count, and set the comms
   fault CF15 bit 9 in BMS mode (battery_type != 0; cleared otherwise). HR19 and the 0xD500 alarm
   bits keep their last values. */
void bms_lost(void) {
    unsigned int i;
    bms.pack_voltage = 0;
    bms.pack_current = 0;
    dsp_settings.bms_soc_raw = 0;
    bms.pack_temp_x10 = 0;
    bms.bms_discharge_limit = 0;
    bms.bms_charge_limit = 0;
    bms_hr17 = 0;
    bms_hr18 = 0;
    bms_alarm_masked = 0;
    bms_alarm = 0;
    bms_present = 0;
    bms_reply_count = 0;
    dsp_settings.bms_hr0 = 0;
    bms.soc = 0;
    for (i = 0; i < 10; i++)
        bms_serial[i] = 0;
    dsp_settings.bms_fw_version = 0;
    dsp_settings.bms_capacity = 0;
    bms_hr15 = 0;
    if (dsp_settings.battery_type != 0)
        fault_words.fault_batt |= FB_BMS_COMMS;
    else
        fault_words.fault_batt &= ~FB_BMS_COMMS;
}

/* The warning byte the ARM gets (arm_link.c, byte 8 of its frame): bit 1 over-voltage (CF15
   bit 6), bit 2 under-voltage (CF15 bit 2), bit 3 soft-start timeout (CF15 bit 5), bit 5 BMS comms
   (CF15 bit 9), bit 7 charger over-temperature (CF17 bit 6). Runs every tick. */
void batt_warning_update(void) {
    if (fault_words.fault_batt & FB_OVER_V)
        batt_warning |= 0x0002;
    else
        batt_warning &= ~0x0002;
    if (fault_words.fault_batt & FB_UNDER_V)
        batt_warning |= 0x0004;
    else
        batt_warning &= ~0x0004;
    if (fault_words.fault_batt & FB_SOFT_START_TIMEOUT)
        batt_warning |= 0x0008;
    else
        batt_warning &= ~0x0008;
    if (fault_words.fault_batt & FB_BMS_COMMS)
        batt_warning |= 0x0020;
    else
        batt_warning &= ~0x0020;
    if (fault_words.fault_stop1 & FS1_CHARGER_OT)
        batt_warning |= 0x0080;
    else
        batt_warning &= ~0x0080;
}
