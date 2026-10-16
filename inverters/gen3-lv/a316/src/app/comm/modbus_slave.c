// clang-format off
/* at: crc16_modbus 0x08017FCC, systick_int_disable 0x08017FFE, systick_disable 0x08018010, modbus_fill_meter_regs 0x08018022, modbus_fc22_read_meter 0x08018386, modbus_fc16_write_regs 0x08018816, modbus_fc06_write_reg 0x0801C160, modbus_fill_battery_regs 0x0801FA04, modbus_fill_bcu_regs 0x0802007C, modbus_fill_slave30_regs 0x0802039C, modbus_fill_meter_ir 0x080206A4, modbus_fc04_read_input 0x08020A32, modbus_fc03_read_holding 0x08021464, modbus_slave_process 0x08022C50, task_modbus_rs485 0x08022F7A */
/* at: g_20000060 0x20000060, modbus_no_reply 0x20000068, modbus_rx_byte 0x2000007A, modbus_rx_idle 0x2000007B, modbus_crc 0x2000007C, modbus_ir_slave 0x20000082, modbus_batt_index 0x20000084, modbus_meter_index 0x20000086, adc_rms_ch0 0x20000150, g_20000152 0x20000152, save_req_pending 0x200001A8, inv_run_mode 0x200001AE, i_rms_ch5 0x200001BC, bus_p_voltage 0x200001DC, dsp_p4_b02 0x20000248, dsp_p4_w03 0x2000024A, dsp_p4_w07 0x2000024C, dsp_p4_w14 0x20000250, dsp_p4_w12 0x20000252, dsp_p4_w05 0x20000254, dsp_p4_w10 0x20000256, dsp_p4_b09 0x2000025A, g_2000025C 0x2000025C, sys_mode 0x200002D2, charge_status 0x200002D5, dsp_warning_word 0x200002D6, batt_voltage 0x200002DC, batt_soc 0x200002E2, batt_capacity_bms 0x200002E4, bus_hv_voltage 0x200002E8, modbus_src 0x2000030B, usb_cdc_timer 0x20000310, p_inverter_out 0x20000320, p_battery 0x20000324, p_load_demand 0x2000035C, modbus_usb_wait 0x20000394, board_init_done 0x20000396, discharge_min_soc_now 0x2000039E, charge_target_soc_now 0x2000039F, bms_packs_online 0x200003AC, p_pv_total_calc 0x200003C0, eeprom_save_delay 0x200003D0, ir_regs 0x2000060A, hr_regs 0x20000682, modbus_rx_buf 0x200010B2, modbus_tx_buf 0x200011DF, g_200019E4 0x200019E4, sched_table_b 0x20001FC6, dev11_data 0x20002028, dsp_fault_words 0x2000236A, modbus_err_reply 0x2000006C, meter_swap_a 0x20000070, meter_swap_b 0x20000074, meter_dir_changed 0x20000078, modbus_fc22_slave 0x2000007E, modbus_fc22_index 0x20000080, cfg_changed_flags 0x2000019C, cfg_reset_mode 0x2000019D, grid_country 0x200001AB, hr31_value 0x200001AC, pv1_power 0x200001F4, pv2_power 0x200001F8, hr4000_value 0x20000206, modbus_rx_idx 0x20000226, modbus_req_len 0x20000228, modbus_reply_len 0x2000022A, auto_test_state 0x20000247, eeprom_dirty 0x20000260, rtc_time 0x2000028F, peek_chip 0x200002EF, peek_addr 0x200002F2, rtc_set_pending 0x2000030C, batt_cal_request 0x20000397, hr200_request 0x200003AB, force_state_hr329 0x200003C8, batt_power_remote 0x200003CA, bms_charge_limit_a 0x200003CC, bms_discharge_limit_a 0x200003CD, hr201_request 0x200003CE, os_task_dly_idx 0x200004CC, modbus_reg_buf 0x20000588, bms_modules 0x200007A4, event_log 0x20000FB8, modbus_reply 0x20001276, fault_log 0x200019B7, cfg_settings 0x20001AAE, cfg_serial_and_settings 0x20001AC1, cal_gains 0x20001B81, energy_totals 0x20001BC5, meter_in_day_start 0x20001C42, cfg_block6 0x20001C82, cfg_block8 0x20001E76, cfg_hr360_table 0x20001F66, meter_data 0x20003C7A, task_tbl 0x20007C6C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* modbus_slave.c: the Modbus RTU slave that the app, the cloud and GivTCP talk to, on two
   transports: RS485 on USART3 (task_modbus_rs485) and USB CDC to the WiFi/LAN dongle (received
   in usbh_cdc_core.c, which calls modbus_slave_process through UserCb).

   Entry points: task_modbus_rs485 (task 3); modbus_slave_process (both transports); crc16_modbus
   (used by the whole application: EEPROM blocks, DSP frames, meters).
   Data: the CRC tables crc16_tbl_hi/lo (flash); modbus_rx_buf, modbus_reply, modbus_tx_buf,
   modbus_reg_buf (one 60-register window being built), and the HR and IR images hr_regs and
   ir_regs that meter.c fills.
   Neighbours: everything whose settings it writes: board.c (defaults, grid limits),
   control/monitor.c (calib_table_defaults), drivers/rtc.c (RTC_Set), the settings blocks in
   settings.h; comm/meter.c (modbus_hr_update, modbus_ir_update build HR0-59 and IR0-59);
   control/measure.c (bms_modules, the pack records it serves).

   Frames, as the code reads and writes them:

     request FC3/4/6/22:  [slave][fc][start or register, 2][count or value, 2][CRC, 2]
     request FC16:        [slave][fc][start, 2][count, 2][count x 2 data][CRC, 2]  (no byte count)
     reply:               [slave][fc][10-byte inverter serial][start, 2][count, 2][data][CRC]
     error reply:         the same 16-byte header with fc | 0x80, then the CRC

   Slaves answered: the inverter's own address (HR30), 0, 1-8, 0x11, 0x30-0x37; on RS485 the task
   only takes first bytes HR30, 1-8 and 0x30-0x36. A read may not cross a multiple of 60
   registers. Register map (reads):

     HR0-59      hr_regs (modbus_hr_update)
     HR60-539    settings, slots, the 0x11 device, tables, a "remote control" view at 480-539
     HR4080-4199, HR4340-4399   calibration gains and offsets
     IR0-59      ir_regs (modbus_ir_update)
     IR60-119    per slave: 1-8 meters, 0x30 system, 0x31 battery unit, 0x32-0x36 packs 1-5
     IR120-179   13 records from g_200019E4 (never written: always zero)
     IR180-239   DSP fault words (IR217-224) and DSP page 4 values
     IR240-299   RMS values, total PV power, calibration offsets

   name: likely (map/filenames.tsv). The 45 KB file is above MDK-Lite's 32 KB link limit, so the
   gate proves it by placing armcc's own object (tools/objplace.py). */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/a316_types.h"
#include "../../include/battery.h"
#include "../../include/board.h"
#include "../../include/clib.h"
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
#include "../../include/uart.h"
#include "../../include/usb.h"

/* auchCRCHi */
const uchar crc16_tbl_hi[256] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40};
/* auchCRCLo */
const uchar crc16_tbl_lo[256] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
    0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
    0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
    0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
    0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
    0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
    0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
    0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
    0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40};

/* CRC16/MODBUS (polynomial 0xA001 reflected, start 0xFFFF) by the two classic tables. The result
   has the first byte to send in its low byte: callers that store "crc, crc >> 8" send the
   standard order, callers that store "crc >> 8, crc" (the DSP link, the 0x11 device) send it
   high byte first. */
ushort crc16_modbus(byte *data, ushort len) {
    uint table_idx;
    uint crc_hi;
    uchar crc_lo;

    crc_hi = 0xff;
    crc_lo = 0xff;
    while (len--) {
        table_idx = *data++ ^ crc_lo;
        crc_lo = crc16_tbl_hi[table_idx] ^ crc_hi;
        crc_hi = (uint)crc16_tbl_lo[table_idx];
    }
    return crc_lo | crc_hi << 8;
}

/* Clears TICKINT in SysTick CTRL (before the jump to the bootloader, HR164). */
void systick_int_disable() {

    *(volatile uint *)SYSTICK_CTRL_ADDR = *(volatile uint *)SYSTICK_CTRL_ADDR & 0xfffffffd;
    return;
}

/* Clears ENABLE in SysTick CTRL (before the jump to the bootloader, HR164). */
void systick_disable() {

    *(volatile uint *)SYSTICK_CTRL_ADDR = *(volatile uint *)SYSTICK_CTRL_ADDR & 0xfffffffe;
    return;
}

typedef __packed struct {
    uchar pad[0x1c];
    uint a;
    uint b;
    ushort w[5];
} rec_t;

/* Fills modbus_reg_buf with the FC22 view of meter n (registers 60-68): the two 32-bit
   function-3 values at +0x1C and +0x20, then the five words +0x24..+0x2C; the rest is 0. */
void modbus_fill_meter_regs(uchar n) {
    uint v;

    v = (uint)meter_data[n].f1C >> 16;
    modbus_reg_buf[0] = v >> 8;
    modbus_reg_buf[1] = v;
    v = (ushort)meter_data[n].f1C;
    modbus_reg_buf[2] = v >> 8;
    modbus_reg_buf[3] = v;
    v = (uint)meter_data[n].f20 >> 16;
    modbus_reg_buf[4] = v >> 8;
    modbus_reg_buf[5] = v;
    v = (ushort)meter_data[n].f20;
    modbus_reg_buf[6] = v >> 8;
    modbus_reg_buf[7] = v;
    v = meter_data[n].f24;
    modbus_reg_buf[8] = v >> 8;
    modbus_reg_buf[9] = v;
    v = meter_data[n].f26;
    modbus_reg_buf[10] = v >> 8;
    modbus_reg_buf[11] = v;
    v = meter_data[n].f28;
    modbus_reg_buf[12] = v >> 8;
    modbus_reg_buf[13] = v;
    v = meter_data[n].f2A;
    modbus_reg_buf[14] = v >> 8;
    modbus_reg_buf[15] = v;
    v = meter_data[n].f2C;
    modbus_reg_buf[16] = v >> 8;
    modbus_reg_buf[17] = v;
    v = 0;
    modbus_reg_buf[18] = v >> 8;
    modbus_reg_buf[19] = v;
    v = 0;
    modbus_reg_buf[20] = v >> 8;
    modbus_reg_buf[21] = v;
    v = 0;
    modbus_reg_buf[22] = v >> 8;
    modbus_reg_buf[23] = v;
    v = 0;
    modbus_reg_buf[24] = v >> 8;
    modbus_reg_buf[25] = v;
    v = 0;
    modbus_reg_buf[26] = v >> 8;
    modbus_reg_buf[27] = v;
    v = 0;
    modbus_reg_buf[28] = v >> 8;
    modbus_reg_buf[29] = v;
    v = 0;
    modbus_reg_buf[30] = v >> 8;
    modbus_reg_buf[31] = v;
    v = 0;
    modbus_reg_buf[32] = v >> 8;
    modbus_reg_buf[33] = v;
    v = 0;
    modbus_reg_buf[34] = v >> 8;
    modbus_reg_buf[35] = v;
    v = 0;
    modbus_reg_buf[36] = v >> 8;
    modbus_reg_buf[37] = v;
    v = 0;
    modbus_reg_buf[38] = v >> 8;
    modbus_reg_buf[39] = v;
    v = 0;
    modbus_reg_buf[40] = v >> 8;
    modbus_reg_buf[41] = v;
    v = 0;
    modbus_reg_buf[42] = v >> 8;
    modbus_reg_buf[43] = v;
    v = 0;
    modbus_reg_buf[44] = v >> 8;
    modbus_reg_buf[45] = v;
    v = 0;
    modbus_reg_buf[46] = v >> 8;
    modbus_reg_buf[47] = v;
    v = 0;
    modbus_reg_buf[48] = v >> 8;
    modbus_reg_buf[49] = v;
    v = 0;
    modbus_reg_buf[50] = v >> 8;
    modbus_reg_buf[51] = v;
    v = 0;
    modbus_reg_buf[52] = v >> 8;
    modbus_reg_buf[53] = v;
    v = 0;
    modbus_reg_buf[54] = v >> 8;
    modbus_reg_buf[55] = v;
    v = 0;
    modbus_reg_buf[56] = v >> 8;
    modbus_reg_buf[57] = v;
    v = 0;
    modbus_reg_buf[58] = v >> 8;
    modbus_reg_buf[59] = v;
    v = 0;
    modbus_reg_buf[60] = v >> 8;
    modbus_reg_buf[61] = v;
    v = 0;
    modbus_reg_buf[62] = v >> 8;
    modbus_reg_buf[63] = v;
    v = 0;
    modbus_reg_buf[64] = v >> 8;
    modbus_reg_buf[65] = v;
    v = 0;
    modbus_reg_buf[66] = v >> 8;
    modbus_reg_buf[67] = v;
    v = 0;
    modbus_reg_buf[68] = v >> 8;
    modbus_reg_buf[69] = v;
    v = 0;
    modbus_reg_buf[70] = v >> 8;
    modbus_reg_buf[71] = v;
    v = 0;
    modbus_reg_buf[72] = v >> 8;
    modbus_reg_buf[73] = v;
    v = 0;
    modbus_reg_buf[74] = v >> 8;
    modbus_reg_buf[75] = v;
    v = 0;
    modbus_reg_buf[76] = v >> 8;
    modbus_reg_buf[77] = v;
    v = 0;
    modbus_reg_buf[78] = v >> 8;
    modbus_reg_buf[79] = v;
    v = 0;
    modbus_reg_buf[80] = v >> 8;
    modbus_reg_buf[81] = v;
    v = 0;
    modbus_reg_buf[82] = v >> 8;
    modbus_reg_buf[83] = v;
    v = 0;
    modbus_reg_buf[84] = v >> 8;
    modbus_reg_buf[85] = v;
    v = 0;
    modbus_reg_buf[86] = v >> 8;
    modbus_reg_buf[87] = v;
    v = 0;
    modbus_reg_buf[88] = v >> 8;
    modbus_reg_buf[89] = v;
    v = 0;
    modbus_reg_buf[90] = v >> 8;
    modbus_reg_buf[91] = v;
    v = 0;
    modbus_reg_buf[92] = v >> 8;
    modbus_reg_buf[93] = v;
    v = 0;
    modbus_reg_buf[94] = v >> 8;
    modbus_reg_buf[95] = v;
    v = 0;
    modbus_reg_buf[96] = v >> 8;
    modbus_reg_buf[97] = v;
    v = 0;
    modbus_reg_buf[98] = v >> 8;
    modbus_reg_buf[99] = v;
    v = 0;
    modbus_reg_buf[100] = v >> 8;
    modbus_reg_buf[101] = v;
    v = 0;
    modbus_reg_buf[102] = v >> 8;
    modbus_reg_buf[103] = v;
    v = 0;
    modbus_reg_buf[104] = v >> 8;
    modbus_reg_buf[105] = v;
    v = 0;
    modbus_reg_buf[106] = v >> 8;
    modbus_reg_buf[107] = v;
    v = 0;
    modbus_reg_buf[108] = v >> 8;
    modbus_reg_buf[109] = v;
    v = 0;
    modbus_reg_buf[110] = v >> 8;
    modbus_reg_buf[111] = v;
    v = 0;
    modbus_reg_buf[112] = v >> 8;
    modbus_reg_buf[113] = v;
    v = 0;
    modbus_reg_buf[114] = v >> 8;
    modbus_reg_buf[115] = v;
    v = 0;
    modbus_reg_buf[116] = v >> 8;
    modbus_reg_buf[117] = v;
    v = 0;
    modbus_reg_buf[118] = v >> 8;
    modbus_reg_buf[119] = v;
}

/* Function 0x16 (A316's own): reads meter records. Registers 0-59 read as zeros; 60-119 give
   meter (slave - 1) for slaves 1-8 (modbus_fill_meter_regs). Count 1..60, no crossing of a
   60-register boundary. For a start of 120 or above no branch sets j, and the uninitialised j is
   then used as the CRC length and the CRC position (docs/findings.md). */
void modbus_fc22_read_meter(uchar *req) {
    ushort i;
    ushort j;
    uint v;
    int addr;
    int cnt;
    ushort crc;
    uchar err = 0;

    addr = req[2];
    addr = addr << 8 | req[3];
    cnt = req[4];
    cnt = cnt << 8 | req[5];
    if (cnt > 0x3c || cnt == 0)
        err = 1;
    if (addr + cnt > (addr / 0x3c + 1) * 0x3c)
        err = 1;
    for (i = 0; i < 10; i++)
        modbus_reply[2 + i] = cfg_serial_and_settings[i];
    for (i = 0; i < 4; i++)
        modbus_reply[0xc + i] = (req + 2)[i];
    if (err == 1) {
        modbus_reply[1] = 0x96;
        crc = crc16_modbus(modbus_reply, 0x10);
        modbus_reply[0x10] = crc;
        modbus_reply[0x11] = crc >> 8;
        modbus_err_reply = 1;
        return;
    }
    for (i = 0; i < 0x78; i++)
        modbus_reg_buf[i] = 0;
    if (addr < 0x3c) {
        v = 0;
        modbus_reg_buf[0] = v >> 8;
        modbus_reg_buf[1] = v;
        v = 0;
        modbus_reg_buf[2] = v >> 8;
        modbus_reg_buf[3] = v;
        v = 0;
        modbus_reg_buf[4] = v >> 8;
        modbus_reg_buf[5] = v;
        v = 0;
        modbus_reg_buf[6] = v >> 8;
        modbus_reg_buf[7] = v;
        v = 0;
        modbus_reg_buf[8] = v >> 8;
        modbus_reg_buf[9] = v;
        v = 0;
        modbus_reg_buf[10] = v >> 8;
        modbus_reg_buf[11] = v;
        v = 0;
        modbus_reg_buf[12] = v >> 8;
        modbus_reg_buf[13] = v;
        v = 0;
        modbus_reg_buf[14] = v >> 8;
        modbus_reg_buf[15] = v;
        v = 0;
        modbus_reg_buf[16] = v >> 8;
        modbus_reg_buf[17] = v;
        v = 0;
        modbus_reg_buf[18] = v >> 8;
        modbus_reg_buf[19] = v;
        v = 0;
        modbus_reg_buf[20] = v >> 8;
        modbus_reg_buf[21] = v;
        v = 0;
        modbus_reg_buf[22] = v >> 8;
        modbus_reg_buf[23] = v;
        v = 0;
        modbus_reg_buf[24] = v >> 8;
        modbus_reg_buf[25] = v;
        v = 0;
        modbus_reg_buf[26] = v >> 8;
        modbus_reg_buf[27] = v;
        v = 0;
        modbus_reg_buf[28] = v >> 8;
        modbus_reg_buf[29] = v;
        v = 0;
        modbus_reg_buf[30] = v >> 8;
        modbus_reg_buf[31] = v;
        v = 0;
        modbus_reg_buf[32] = v >> 8;
        modbus_reg_buf[33] = v;
        v = 0;
        modbus_reg_buf[34] = v >> 8;
        modbus_reg_buf[35] = v;
        v = 0;
        modbus_reg_buf[36] = v >> 8;
        modbus_reg_buf[37] = v;
        v = 0;
        modbus_reg_buf[38] = v >> 8;
        modbus_reg_buf[39] = v;
        v = 0;
        modbus_reg_buf[40] = v >> 8;
        modbus_reg_buf[41] = v;
        v = 0;
        modbus_reg_buf[42] = v >> 8;
        modbus_reg_buf[43] = v;
        v = 0;
        modbus_reg_buf[44] = v >> 8;
        modbus_reg_buf[45] = v;
        v = 0;
        modbus_reg_buf[46] = v >> 8;
        modbus_reg_buf[47] = v;
        v = 0;
        modbus_reg_buf[48] = v >> 8;
        modbus_reg_buf[49] = v;
        v = 0;
        modbus_reg_buf[50] = v >> 8;
        modbus_reg_buf[51] = v;
        v = 0;
        modbus_reg_buf[52] = v >> 8;
        modbus_reg_buf[53] = v;
        v = 0;
        modbus_reg_buf[54] = v >> 8;
        modbus_reg_buf[55] = v;
        v = 0;
        modbus_reg_buf[56] = v >> 8;
        modbus_reg_buf[57] = v;
        v = 0;
        modbus_reg_buf[58] = v >> 8;
        modbus_reg_buf[59] = v;
        v = 0;
        modbus_reg_buf[60] = v >> 8;
        modbus_reg_buf[61] = v;
        v = 0;
        modbus_reg_buf[62] = v >> 8;
        modbus_reg_buf[63] = v;
        v = 0;
        modbus_reg_buf[64] = v >> 8;
        modbus_reg_buf[65] = v;
        v = 0;
        modbus_reg_buf[66] = v >> 8;
        modbus_reg_buf[67] = v;
        v = 0;
        modbus_reg_buf[68] = v >> 8;
        modbus_reg_buf[69] = v;
        v = 0;
        modbus_reg_buf[70] = v >> 8;
        modbus_reg_buf[71] = v;
        v = 0;
        modbus_reg_buf[72] = v >> 8;
        modbus_reg_buf[73] = v;
        v = 0;
        modbus_reg_buf[74] = v >> 8;
        modbus_reg_buf[75] = v;
        v = 0;
        modbus_reg_buf[76] = v >> 8;
        modbus_reg_buf[77] = v;
        v = 0;
        modbus_reg_buf[78] = v >> 8;
        modbus_reg_buf[79] = v;
        v = 0;
        modbus_reg_buf[80] = v >> 8;
        modbus_reg_buf[81] = v;
        v = 0;
        modbus_reg_buf[82] = v >> 8;
        modbus_reg_buf[83] = v;
        v = 0;
        modbus_reg_buf[84] = v >> 8;
        modbus_reg_buf[85] = v;
        v = 0;
        modbus_reg_buf[86] = v >> 8;
        modbus_reg_buf[87] = v;
        v = 0;
        modbus_reg_buf[88] = v >> 8;
        modbus_reg_buf[89] = v;
        v = 0;
        modbus_reg_buf[90] = v >> 8;
        modbus_reg_buf[91] = v;
        v = 0;
        modbus_reg_buf[92] = v >> 8;
        modbus_reg_buf[93] = v;
        v = 0;
        modbus_reg_buf[94] = v >> 8;
        modbus_reg_buf[95] = v;
        v = 0;
        modbus_reg_buf[96] = v >> 8;
        modbus_reg_buf[97] = v;
        v = 0;
        modbus_reg_buf[98] = v >> 8;
        modbus_reg_buf[99] = v;
        v = 0;
        modbus_reg_buf[100] = v >> 8;
        modbus_reg_buf[101] = v;
        v = 0;
        modbus_reg_buf[102] = v >> 8;
        modbus_reg_buf[103] = v;
        v = 0;
        modbus_reg_buf[104] = v >> 8;
        modbus_reg_buf[105] = v;
        v = 0;
        modbus_reg_buf[106] = v >> 8;
        modbus_reg_buf[107] = v;
        v = 0;
        modbus_reg_buf[108] = v >> 8;
        modbus_reg_buf[109] = v;
        v = 0;
        modbus_reg_buf[110] = v >> 8;
        modbus_reg_buf[111] = v;
        v = 0;
        modbus_reg_buf[112] = v >> 8;
        modbus_reg_buf[113] = v;
        v = 0;
        modbus_reg_buf[114] = v >> 8;
        modbus_reg_buf[115] = v;
        v = 0;
        modbus_reg_buf[116] = v >> 8;
        modbus_reg_buf[117] = v;
        v = 0;
        modbus_reg_buf[118] = v >> 8;
        modbus_reg_buf[119] = v;
        for (i = addr * 2, j = 0x10; addr * 2 + cnt * 2 > i; i++, j++) {
            modbus_reply[j] = modbus_reg_buf[i];
        }
    } else if (addr < 0x78) {
        modbus_fc22_slave = req[0];
        if (modbus_fc22_slave >= 1 && modbus_fc22_slave <= 8) {
            modbus_fc22_index = modbus_fc22_slave - 1;
            modbus_fill_meter_regs(modbus_fc22_index);
        }
        for (i = addr * 2 - 0x78, j = 0x10; addr * 2 + cnt * 2 - 0x78 > i; i++, j++) {
            modbus_reply[j] = modbus_reg_buf[i];
        }
    }
    crc = crc16_modbus(modbus_reply, j);
    modbus_reply[j] = crc;
    modbus_reply[j + 1] = crc >> 8;
}

/* match: AAE.t[i][j] is cfg_settings.inverter_serial[i][j] and PX the meters' day-start counters
   (energy_totals.meter_in_day_start, reached from cfg_settings' address); written as those
   fields the bytes differ */
#define AAE (*(struct aae *)&cfg_settings)

#define PX ((*(aaeb_t *)&cfg_settings).a)
#define NVIC ((nvic_t *)NVIC_ISER_ADDR)
/* Function 16: writes count registers from start, each through the same checks as
   modbus_fc06_write_reg (see there for the register rules). Differences from FC6: HR51 accepts
   up to 50 here and 60 there; HR167 (+0x73, 0..4) can only be written here. The values are taken
   from bytes 6 + 2i (there is no byte count); the index stops at byte 150, so a write of more
   than 73 registers repeats byte 150 for the rest. The first refused register stops the write
   (the ones before it stay written) and the reply becomes the 0x90 error. The count is the
   request's own, not the one modbus_slave_process capped at 100. */
void modbus_fc16_write_regs(int unused_start, int unused_count, uchar *reply, uchar *req) {
    int reg_addr = 0;
    int reg_count = 0;
    ushort reg_value = 0;
    ushort reg_idx = 0;
    int value_hi = 0;
    int value_lo = 0;
    ushort crc = 0;
    int write_error = 0;
    uchar i = 0;
    int byte_hi_idx = 0;
    int byte_lo_idx = 0;
    int clear_len;
    uchar *clear_data;

    for (i = 0; i < 0xa; i++) {
        reply[2 + i] = cfg_serial_and_settings[i];
    }
    for (i = 0; i < 4; i++) {
        reply[0xc + i] = (req + 2)[i];
    }
    reg_count = req[4];
    reg_count = req[5] | reg_count << 8;
    reg_addr = req[2];
    reg_addr = req[3] | reg_addr << 8;
    value_hi = req[6];
    value_lo = req[7];
    reg_value = req[6];
    reg_value = req[7] | reg_value << 8;
    for (reg_idx = 1; reg_idx <= reg_count && write_error == 0; reg_idx++) {
        switch (reg_addr) {
        case 0:
            break;
        case HR_INVERTER_MODULE_H:
            cfg_settings.inverter_module_h = reg_value;
            eeprom_dirty = 0x11;
            break;
        case HR_INVERTER_MODULE_L:
            if (0x1d > (int)((uint)reg_value >> 8) && (uchar)reg_value <= 0x32) {
                cfg_settings.inverter_module_l = reg_value;
                cfg_load_defaults(1);
                cfg_load_defaults(6);
                eeprom_dirty = eeprom_dirty | 0x31;
                cfg_changed_flags = cfg_changed_flags | 1;
                cfg_reset_mode = 0;
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_AMMETER:
            if (reg_value <= 0xff) {
                cfg_settings.enable_ammeter = reg_value;
                if ((cfg_settings.enable_ammeter & 1) != 0) {
                    cfg_settings.hr176 = (cfg_settings.hr176 & ~8) + 8;
                } else {
                    cfg_settings.hr176 = cfg_settings.hr176 & ~8;
                }
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 8:
            write_error = 1;
            break;
        case 9:
            write_error = 1;
            break;
        case 10:
            write_error = 1;
            break;
        case 13:
            AAE.t[0][0] = (uint)reg_value >> 8;
            AAE.t[0][1] = reg_value;
            break;
        case 14:
            AAE.t[1][0] = (uint)reg_value >> 8;
            AAE.t[1][1] = reg_value;
            break;
        case 15:
            AAE.t[2][0] = (uint)reg_value >> 8;
            AAE.t[2][1] = reg_value;
            break;
        case 16:
            AAE.t[3][0] = (uint)reg_value >> 8;
            AAE.t[3][1] = reg_value;
            break;
        case 17:
            AAE.t[4][0] = (uint)reg_value >> 8;
            AAE.t[4][1] = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
            break;
        case HR_ENABLE_CHARGE_TARGET:
            if (reg_value <= 1) {
                cfg_settings.enable_charge_target = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 23:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 1) {
                peek_chip[0] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 24:
            if (reg_value >= 0xc000 && reg_value <= 0xe000 && peek_chip[0] == 0) {
                peek_addr = reg_value;
            } else {
                if ((reg_value > 0 || reg_value == 0) && reg_value < 0x10000 && peek_chip[0] == 1) {
                    peek_addr = reg_value;
                } else {
                    write_error = 1;
                }
            }
            break;
        case HR_GRID_PORT_MAX_POWER:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0xea60) {
                cfg_settings.grid_port_max_power = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_POWER_MODE:
            if (reg_value <= 1) {
                cfg_settings.battery_power_mode = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_60HZ_FREQ_MODE:
            if (reg_value <= 1) {
                cfg_settings.enable_60hz_freq_mode = reg_value;
                grid_load_country_limits();
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_SOC_FORCE_ADJUST:
            if (reg_value <= 7 && cfg_settings.battery_type == 1) {
                if (reg_value > 0) {
                    cfg_settings.soc_force_adjust =
                        cfg_settings.soc_force_adjust | (uchar)reg_value;
                } else {
                    cfg_settings.soc_force_adjust = reg_value;
                }
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_MODBUS_ADDRESS:
            if (reg_value > 0 && reg_value <= 0xff) {
                cfg_settings.modbus_address = reg_value;
                eeprom_dirty = eeprom_dirty | 0x11;
            } else {
                write_error = 1;
            }
            break;
        case 31:
            if (reg_value < 5) {
                hr31_value = reg_value;
                cfg_settings.hr31 = cfg_settings.hr31 & 0xf;
                cfg_settings.hr31 = cfg_settings.hr31 | 0xf0 & hr31_value << 4;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_USER_CODE:
            if (reg_value <= 0x80) {
                cfg_settings.user_code = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 35:
            rtc_set_pending = 1;
            if (reg_value > 0 && reg_value <= 0x2710) {
                if (reg_value <= 0x63) {
                    rtc_time.year = reg_value;
                } else {
                    rtc_time.year = reg_value - 0x7d0;
                }
            } else {
                write_error = 1;
            }
            break;
        case 36:
            if (reg_value > 0 && reg_value <= 0xc) {
                rtc_time.month = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 37:
            if (rtc_time.month == 1 || rtc_time.month == 3 || rtc_time.month == 5 ||
                rtc_time.month == 7 || rtc_time.month == 8 || rtc_time.month == 0xa ||
                rtc_time.month == 0xc) {
                if (reg_value > 0 && reg_value <= 0x1f) {
                    rtc_time.date = reg_value;
                } else {
                    write_error = 1;
                }
            } else {
                if (rtc_time.month == 4 || rtc_time.month == 6 || rtc_time.month == 9 ||
                    rtc_time.month == 0xb) {
                    if (reg_value > 0 && reg_value <= 0x1e) {
                        rtc_time.date = reg_value;
                    } else {
                        write_error = 1;
                    }
                } else {
                    if ((rtc_time.year + 0x7d0) % 0x190 == 0 ||
                        ((rtc_time.year + 0x7d0) % 4 == 0 && (rtc_time.year + 0x7d0) % 0x64 != 0)) {
                        if (reg_value > 0 && reg_value <= 0x1d) {
                            rtc_time.date = reg_value;
                        } else {
                            write_error = 1;
                        }
                    } else {
                        if (reg_value > 0 && reg_value <= 0x1c) {
                            rtc_time.date = reg_value;
                        } else {
                            write_error = 1;
                        }
                    }
                }
            }
            break;
        case 38:
            if (reg_value <= 0x17) {
                rtc_time.hour = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 39:
            if (reg_value <= 0x3b) {
                rtc_time.min = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 40:
            if (reg_value <= 0x3b) {
                rtc_time.sec = reg_value;
            } else {
                write_error = 1;
            }
            RTC_EnterConfigMode();
            RTC_WaitForLastTask();
            RTC_Set(rtc_time.year + 0x7d0, rtc_time.month, rtc_time.date, rtc_time.hour,
                    rtc_time.min, rtc_time.sec);
            RTC_ExitConfigMode();
            BKP_WriteBackupRegister(4, 0xa5a5);
            rtc_set_pending = 0;
            break;
        case HR_ENABLE_DRM_RJ45:
            if (reg_value <= 1) {
                cfg_settings.enable_drm_rj45 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_REVERSE_CT_CLAMP:
            if (reg_value <= 3) {
                cfg_settings.enable_reverse_ct_clamp = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_DISCHARGE_SLOT_2_START:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.discharge_slot_2_start = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_DISCHARGE_SLOT_2_END:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.discharge_slot_2_end = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 46:
            break;
        case HR_METER_TYPE:
            if (reg_value < 2) {
                cfg_settings.meter_type = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_REVERSE_115_METER_DIR:
            if (reg_value <= 0xff) {
                meter_dir_changed = cfg_settings.reverse_115_meter_dir ^ reg_value;
                for (i = 0; i < 8; i++) {
                    if ((1 << i & meter_dir_changed) == 1 << i) {
                        meter_swap_a = PX[0][i];
                        meter_swap_b = PX[1][i];
                        PX[0][i] = meter_swap_b;
                        PX[1][i] = meter_swap_a;
                    }
                }
                cfg_settings.reverse_115_meter_dir = reg_value;
                cfg_settings.fA2 = 3;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                if (reg_value == 0xaa) {
                    cfg_settings.fA2 = 0;
                    eeprom_dirty = eeprom_dirty | 1;
                } else {
                    write_error = 1;
                }
            }
            break;
        case HR_REVERSE_418_METER_DIR:
            if (reg_value <= 0xff) {
                meter_dir_changed = cfg_settings.reverse_418_meter_dir ^ reg_value;
                for (i = 0; i < 8; i++) {
                    if ((1 << i == 1 << i & meter_dir_changed) != 0) {
                        meter_swap_a = PX[0][i];
                        meter_swap_b = PX[1][i];
                        PX[0][i] = meter_swap_b;
                        PX[1][i] = meter_swap_a;
                    }
                }
                cfg_settings.reverse_418_meter_dir = reg_value;
                cfg_settings.fA2 = 3;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                if (reg_value == 0xaa) {
                    cfg_settings.reverse_418_meter_dir = 0;
                    eeprom_dirty = eeprom_dirty | 1;
                } else {
                    write_error = 1;
                }
            }
            break;
        case HR_ACTIVE_POWER_RATE:
            if (reg_value <= 0x64) {
                cfg_settings.active_power_rate = reg_value;
                cfg_settings.hr121 = 0;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_REACTIVE_POWER_RATE:
            if (reg_value <= 0x32) {
                cfg_settings.reactive_power_rate = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_POWER_FACTOR:
            if (reg_value <= 0x4e20) {
                cfg_settings.power_factor = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_INVERTER_STATE:
            if ((uchar)reg_value == 0) {
                cfg_settings.inverter_state = 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                if ((uchar)reg_value == 1) {
                    cfg_settings.inverter_state = 1;
                    eeprom_dirty = eeprom_dirty | 1;
                } else {
                    write_error = 1;
                }
            }
            if ((reg_value & 0xff00) == 0x100) {
                cfg_settings.f09 = 1;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                if ((reg_value & 0xff00) == 0) {
                    cfg_settings.f09 = 0;
                    eeprom_dirty = eeprom_dirty | 1;
                } else {
                    write_error = 1;
                }
            }
            break;
        case HR_BATTERY_TYPE:
            if (reg_value <= 1) {
                cfg_settings.battery_type = reg_value;
                if (cfg_settings.battery_type == 0) {
                    cfg_settings.soc_force_adjust = 0;
                    cfg_settings.battery_charge_limit = 0x14;
                    cfg_settings.enable_auto_judge_battery_type = 0;
                } else {
                    if (cfg_settings.battery_type == 1) {
                        cfg_settings.battery_charge_limit = 0x32;
                    }
                }
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_CAPACITY:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x2710) {
                cfg_settings.battery_capacity = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_DISCHARGE_SLOT_1_START:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.discharge_slot_1_start = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_DISCHARGE_SLOT_1_END:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.discharge_slot_1_end = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_AUTO_JUDGE_BATTERY_TYPE:
            if (reg_value <= 1) {
                cfg_settings.enable_auto_judge_battery_type = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_DISCHARGE:
            if (reg_value <= 1) {
                cfg_settings.enable_discharge = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_V_PV_INPUT_START:
            if (reg_value >= 0x320 && reg_value <= 0x2710) {
                cfg_settings.v_pv_input_start = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_INVERTER_START_TIME:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x258) {
                cfg_settings.inverter_start_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_INVERTER_RESTART_DELAY:
            if (reg_value <= 0x384) {
                cfg_settings.inverter_restart_delay = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_LOW_OUT:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_low_out = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_HIGH_OUT:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_high_out = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_LOW_OUT:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_low_out = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_HIGH_OUT:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_high_out = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_LOW_OUT_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.v_ac_low_out_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_HIGH_OUT_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.v_ac_high_out_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_LOW_OUT_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.f_ac_low_out_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_HIGH_OUT_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.f_ac_high_out_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_LOW_IN:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_low_in = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_HIGH_IN:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_high_in = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_LOW_IN:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_low_in = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_HIGH_IN:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_high_in = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_LOW_IN_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.v_ac_low_in_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_HIGH_IN_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.v_ac_high_in_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_LOW_IN_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.f_ac_low_in_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_HIGH_IN_TIME:
            if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
                cfg_settings.f_ac_high_in_time = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_LOW_C:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_low_c = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_AC_HIGH_C:
            if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
                cfg_settings.v_ac_high_c = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_LOW_C:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_low_c = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_F_AC_HIGH_C:
            if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
                cfg_settings.f_ac_high_c = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_V_10_MIN_PROTECTION:
            if (reg_value <= 0xbb8) {
                cfg_settings.v_10_min_protection = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_CHARGE_SLOT_1_START:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.charge_slot_1_start = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_CHARGE_SLOT_1_END:
            if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
                cfg_settings.charge_slot_1_end = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_ENABLE_CHARGE:
            if (reg_value <= 1) {
                cfg_settings.enable_charge = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_LOW_VOLTAGE_LIMIT:
            if (reg_value <= 0xffdc) {
                cfg_settings.battery_low_voltage_limit = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_HIGH_VOLTAGE_LIMIT:
            if (reg_value <= 0xffdc) {
                cfg_settings.battery_high_voltage_limit = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 101:
            if (reg_value <= 0xff && reg_value >= 1) {
                cfg_settings.hr101 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 102:
            if (reg_value <= 1) {
                cfg_settings.hr102 = cfg_settings.hr102 & 0xffffffbf | (reg_value & 1) << 6;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 108:
            if (reg_value <= 0x64) {
                cfg_settings.hr108 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 109:
            if (reg_value <= 0x64) {
                cfg_settings.hr109 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_SOC_RESERVE:
            if (reg_value < 0x64) {
                cfg_settings.battery_soc_reserve = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_CHARGE_LIMIT:
            if (reg_value <= 0xfa) {
                cfg_settings.battery_charge_limit = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_DISCHARGE_LIMIT:
            if (reg_value <= 0xfa) {
                cfg_settings.battery_discharge_limit = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 113:
            if (reg_value <= 1) {
                cfg_settings.hr113 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_BATTERY_DISCHARGE_MIN_POWER_RESERVE:
            if (reg_value <= 0x64) {
                cfg_settings.battery_discharge_min_power_reserve = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 115:
            if (reg_value <= 1) {
                cfg_settings.hr115 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case HR_CHARGE_TARGET_SOC:
            if (reg_value <= 0x64) {
                cfg_settings.charge_target_soc = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 121:
            if (reg_value <= 0xff) {
                cfg_settings.hr121 = reg_value;
                if (reg_value == 1) {
                    cfg_settings.hr121 = 3;
                }
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 2;
            }
            break;
        case 122:
            if (reg_value <= 0xa) {
                cfg_settings.hr122 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 123:
            if (reg_value >= 5 && reg_value <= 0x64) {
                cfg_settings.hr123 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 124:
            if (reg_value <= 1) {
                cfg_block6.hr124 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
                write_error = 0;
            } else {
                write_error = 2;
            }
            break;
        case 125:
            if (reg_value <= 3) {
                cfg_settings.hr125 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 126:
            if (reg_value > 1) {
                write_error = 1;
            }
            break;
        case 127:
            if (reg_value == 1 && grid_country == 4) {
                auto_test_state = 1;
                write_error = 0;
            } else {
                write_error = 2;
            }
            break;
        case 128:
            if (reg_value <= 1) {
                cfg_settings.hr128 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 129:
            if (reg_value == 1) {
                cfg_settings.hr129 = 1;
            } else {
                if (reg_value == 0) {
                    cfg_settings.hr129 = 0;
                } else {
                    write_error = 1;
                }
            }
            break;
        case 130:
            if (reg_value > 0 && reg_value <= 0x12c) {
                cfg_settings.hr130 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 131:
            if (reg_value > 0 && reg_value <= 0xc350) {
                cfg_settings.hr131 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 132:
            if (reg_value > 0 && reg_value <= 0x12c) {
                cfg_settings.hr132 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 133:
            if (reg_value > 0 && reg_value <= 0xc350) {
                cfg_settings.hr133 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 134:
            if (reg_value > 0 && reg_value <= 0x12c) {
                cfg_settings.hr134 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 135:
            if (reg_value > 0 && reg_value <= 0xc350) {
                cfg_settings.hr135 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 136:
            if (reg_value > 0 && reg_value <= 0x12c) {
                cfg_settings.hr136 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 137:
            if (reg_value > 0 && reg_value <= 0xc350) {
                cfg_settings.hr137 = reg_value;
                if (cfg_settings.hr129 == 1) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 138:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr138 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 139:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr139 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 140:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr140 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 141:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr141 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 142:
            if (reg_value <= 0x64) {
                cfg_settings.hr142 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 143:
            if (reg_value <= 0x64) {
                cfg_settings.hr143 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 144:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr144 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 145:
            if (reg_value <= 0xbb8) {
                cfg_settings.hr145 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 162:
            if (reg_value == 1) {
                clear_len = 0xbd;
                clear_data = (uchar *)&energy_totals;
                for (i = 0; i < clear_len; i++) {
                    clear_data[i] = 0;
                }
                clear_len = 0x2d;
                clear_data = &fault_log[0];
                for (i = 0; i < clear_len; i++) {
                    clear_data[i] = 0;
                }
                for (i = 0; i < 0xfa; i++) {
                    event_log[i] = 0;
                }
                eeprom_dirty = eeprom_dirty | 0xe;
            } else {
                write_error = 1;
            }
            break;
        case 163:
            if (reg_value == 1 || reg_value == 2) {
                clear_len = 0xbd;
                clear_data = (uchar *)&energy_totals;
                for (i = 0; i < clear_len; i++) {
                    clear_data[i] = 0;
                }
                clear_len = 0x2d;
                clear_data = &fault_log[0];
                for (i = 0; i < clear_len; i++) {
                    clear_data[i] = 0;
                }
                for (i = 0; i < 0xfa; i++) {
                    event_log[i] = 0;
                }
                cfg_load_defaults(1);
                cfg_load_defaults(6);
                if (reg_value == 2) {
                    calib_table_defaults();
                }
                eeprom_dirty = eeprom_dirty | 0x1f;
                cfg_changed_flags = cfg_changed_flags | 1;
                cfg_reset_mode = 0;
            } else {
                if (reg_value == 3) {
                    calib_table_defaults();
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    if (reg_value == 0x64) {
                        cfg_reset_mode = 0;
                        cfg_changed_flags = cfg_changed_flags | 1;
                    } else {
                        if (reg_value == 0x65) {
                            cfg_reset_mode = 0;
                        } else {
                            if (reg_value == 0xc8) {
                                cfg_reset_mode = 1;
                                cfg_changed_flags = cfg_changed_flags | 1;
                            } else {
                                write_error = 1;
                            }
                        }
                    }
                }
            }
            break;
        case 164:
            if (*req != 0) {
                if (reg_value == 1) {
                    led_set(1, 0);
                    led_set(2, 0);
                    systick_int_disable();
                    systick_disable();
                    NVIC->ICER[0] = 0xffffffff;
                    NVIC->ICER[1] = 0xffffffff;
                    NVIC_SetVectorTable(0x8000000, 0);
                    ((code *)*(volatile uint *)0x2c)();
                } else {
                    if (reg_value == 0x100) {
                        os_task_delay((uchar)os_task_dly_idx, 0x12c, 5);
                        task_tbl[TASK_DSP_RX].period = 0;
                        task_tbl[TASK_DSP_TX].period = 0;
                        while (1) {
                        }
                    }
                    write_error = 1;
                }
            }
            break;
        case 166:
            if (reg_value <= 1 && cfg_settings.hr166 != reg_value) {
                cfg_settings.hr166 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 167:
            if (reg_value <= 4) {
                cfg_settings.f73 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 168:
            break;
        case 169:
            break;
        case 170:
            break;
        case 171:
            break;
        case 175:
            if (reg_value <= 1) {
                batt_cal_request = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 176:
            if (reg_value <= 0xff) {
                cfg_settings.hr176 = cfg_settings.hr176 & 0xffffff00 | (reg_value & 0xff) << 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 177:
            if (reg_value <= 1) {
                cfg_settings.hr177 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 178:
            if (reg_value <= 1) {
                cfg_settings.hr178 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 179:
            if (reg_value <= 1) {
                cfg_settings.hr179 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 180:
            cfg_block8[0] = (uint)reg_value >> 8;
            cfg_block8[1] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 181:
            cfg_block8[2] = (uint)reg_value >> 8;
            cfg_block8[3] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 182:
            cfg_block8[4] = (uint)reg_value >> 8;
            cfg_block8[5] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 183:
            cfg_block8[6] = (uint)reg_value >> 8;
            cfg_block8[7] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 184:
            cfg_block8[8] = (uint)reg_value >> 8;
            cfg_block8[9] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 185:
            cfg_block8[10] = (uint)reg_value >> 8;
            cfg_block8[11] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 186:
            cfg_block8[12] = (uint)reg_value >> 8;
            cfg_block8[13] = reg_value;
            eeprom_dirty = eeprom_dirty | 0x80;
            break;
        case 200:
            if (reg_value <= 1) {
                hr200_request = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 201:
            if (reg_value <= 1) {
                hr201_request = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 230:
            if (reg_value <= 1) {
                cfg_block6.hr230 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 240:
            break;
        case 241:
            break;
        case 242:
            if (reg_value <= 0x64) {
                cfg_block6.hr242 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 243:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b1_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 244:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b1_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 245:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b1_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 246:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b2_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 247:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b2_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 248:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b2_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 249:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b3_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 250:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b3_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 251:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b3_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 252:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b4_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 253:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b4_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 254:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b4_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 255:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b5_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 256:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b5_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 257:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b5_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 258:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b6_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 259:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b6_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 260:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b6_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 261:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b7_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 262:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b7_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 263:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b7_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 264:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b8_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 265:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b8_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 266:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b8_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 267:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b9_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 268:
            if (reg_value <= 0x960) {
                cfg_block6.slot_b9_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 269:
            if (reg_value <= 0x64) {
                cfg_block6.slot_b9_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 270:
            break;
        case 271:
            break;
        case 272:
            if (reg_value <= 0x64) {
                cfg_block6.hr272 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 273:
            break;
        case 274:
            break;
        case 275:
            if (reg_value <= 0x64) {
                cfg_block6.hr275 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 276:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a1_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 277:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a1_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 278:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a1_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 279:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a2_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 280:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a2_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 281:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a2_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 282:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a3_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 283:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a3_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 284:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a3_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 285:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a4_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 286:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a4_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 287:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a4_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 288:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a5_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 289:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a5_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 290:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a5_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 291:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a6_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 292:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a6_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 293:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a6_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 294:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a7_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 295:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a7_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 296:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a7_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 297:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a8_start = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 298:
            if (reg_value <= 0x960) {
                cfg_block6.slot_a8_end = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 299:
            if (reg_value <= 0x64) {
                cfg_block6.slot_a8_soc = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 300:
            if (reg_value <= 0x7530) {
                cfg_settings.hr300 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 301:
            if (reg_value <= 0x7530) {
                cfg_settings.hr301 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 302:
            if (reg_value >= 0xa && reg_value <= 0x7530) {
                cfg_settings.hr302 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 303:
            if (reg_value <= 0x7530) {
                cfg_settings.hr303 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 304:
            break;
        case 305:
            if (reg_value <= 4) {
                cfg_settings.hr305 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 306:
            if (reg_value >= 0xa && reg_value <= 0x7530) {
                cfg_settings.hr306 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 307:
            if (reg_value > 0x7d0 && reg_value <= 0x960) {
                cfg_settings.hr307 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 308:
            if (reg_value > 0x1f4 && reg_value <= 0x1770) {
                cfg_settings.hr308 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 309:
            if (reg_value >= 0xa && reg_value <= 0x50) {
                cfg_settings.hr309 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 310:
            if (reg_value >= 0x14 && reg_value <= 0x64) {
                cfg_settings.hr310 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 311:
            if (reg_value <= 2) {
                cfg_settings.hr311 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 312:
            if (reg_value <= 0x7530) {
                cfg_settings.hr312 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 313:
            if (reg_value >= 1 && reg_value <= 0x64) {
                cfg_settings.hr313 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 314:
            if (reg_value >= 1 && reg_value <= 0x64) {
                cfg_settings.hr314 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 315:
            if (reg_value <= 0xc80) {
                cfg_settings.hr315 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 316:
            if (reg_value <= 0xc80) {
                cfg_settings.hr316 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 317:
            if (reg_value <= 1) {
                cfg_settings.hr102 = cfg_settings.hr102 & 0xfffffffe | (reg_value & 1) << 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 318:
            if (reg_value <= 3) {
                cfg_settings.hr318 = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 319:
            if (reg_value <= 0x960) {
                cfg_settings.hr319 = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 320:
            if (reg_value <= 0x960) {
                cfg_settings.hr320 = reg_value;
                if (cfg_settings.hr166 == 0) {
                    eeprom_dirty = eeprom_dirty | 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 321:
            if (reg_value >= 0x139c && reg_value <= 0x1450) {
                cfg_settings.hr321 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 322:
            if (reg_value <= 1) {
                cfg_block6.hr322 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 323:
            if (reg_value < 0x4e20) {
                cfg_block6.hr323 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 324:
            if (reg_value < 0x4e20) {
                cfg_block6.hr324 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 325:
            if (reg_value < 0x4e20) {
                cfg_block6.hr325 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 326:
            if (reg_value >= 0x11f8 && reg_value <= 0x1388) {
                cfg_settings.hr326 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 327:
            if (reg_value >= 0x11 && reg_value <= 0x64) {
                cfg_settings.hr327 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 328:
            if (reg_value >= 0x1388 && reg_value <= 0x1450) {
                cfg_settings.hr328 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 329:
            if (reg_value <= 1) {
                force_state_hr329 = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 330:
            if (reg_value <= 0xff) {
                cfg_settings.hr330 = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 331:
            if (reg_value <= 0xff) {
                cfg_settings.hr331 = cfg_settings.hr331 & 0xffffff00 | (reg_value & 0xff) << 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 332:
            if (reg_value < 2) {
                if (reg_value == 0) {
                    cfg_settings.hr305 = 0;
                    cfg_settings.fD2 = 0;
                    cfg_settings.battery_type = 1;
                } else {
                    cfg_settings.hr305 = 1;
                    cfg_settings.fD2 = 1;
                    cfg_settings.battery_type = 0;
                    cfg_settings.battery_capacity = 0x7d;
                }
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 351:
            if (reg_value <= 1) {
                cfg_block6.hr351 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 352:
            if (reg_value <= 1) {
                cfg_block6.hr352 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 353:
            if (reg_value < 0xffff) {
                cfg_block6.hr353 = reg_value;
                eeprom_dirty = eeprom_dirty | 0x20;
            } else {
                write_error = 1;
            }
            break;
        case 360:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][0] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 361:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][1] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 362:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][2] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 363:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][3] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 364:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][4] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 365:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][5] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 366:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][6] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 367:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][7] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 368:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][8] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 369:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][9] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 370:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][10] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 371:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][11] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 372:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][12] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 373:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][13] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 374:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][14] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 375:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][15] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 376:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][16] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 377:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][17] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 378:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][18] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 379:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][19] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 380:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][20] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 381:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][21] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 382:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][22] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 383:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][23] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 384:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][24] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 385:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][25] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 386:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][26] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 387:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][27] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 388:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][28] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 389:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][29] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 390:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][30] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 391:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][31] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 392:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][32] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 393:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][33] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 394:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][34] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 395:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][35] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 396:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][36] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 397:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][37] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 398:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][38] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 399:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][39] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 400:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][40] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 401:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][41] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 402:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][42] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 403:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][43] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 404:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][44] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 405:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][45] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 406:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][46] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 407:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[1][47] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 408:
            break;
        case 409:
            break;
        case 410:
            break;
        case 411:
            break;
        case 412:
            break;
        case 413:
            break;
        case 414:
            break;
        case 415:
            break;
        case 416:
            break;
        case 417:
            break;
        case 420:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][0] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 421:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][1] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 422:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][2] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 423:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][3] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 424:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][4] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 425:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][5] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 426:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][6] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 427:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][7] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 428:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][8] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 429:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][9] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 430:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][10] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 431:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][11] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 432:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][12] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 433:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][13] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 434:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][14] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 435:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][15] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 436:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][16] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 437:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][17] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 438:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][18] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 439:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][19] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 440:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][20] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 441:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][21] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 442:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][22] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 443:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][23] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 444:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][24] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 445:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][25] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 446:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][26] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 447:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][27] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 448:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][28] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 449:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][29] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 450:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][30] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 451:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][31] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 452:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][32] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 453:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][33] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 454:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][34] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 455:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][35] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 456:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][36] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 457:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][37] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 458:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][38] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 459:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][39] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 460:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][40] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 461:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][41] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 462:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][42] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 463:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][43] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 464:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][44] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 465:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][45] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 466:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][46] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 467:
            if (reg_value < 0x4e20) {
                cfg_hr360_table[0][47] = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 481:
            if (reg_value <= 1) {
                cfg_settings.battery_power_mode = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 482:
            if (reg_value <= 7 && cfg_settings.battery_type == 1) {
                if (reg_value > 0) {
                    cfg_settings.soc_force_adjust =
                        cfg_settings.soc_force_adjust | (uchar)reg_value;
                } else {
                    cfg_settings.soc_force_adjust = reg_value;
                }
            } else {
                write_error = 1;
            }
            break;
        case 483:
            if (reg_value <= 0x960) {
                cfg_settings.discharge_slot_1_start = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 484:
            if (reg_value <= 0x960) {
                cfg_settings.discharge_slot_1_end = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 485:
            if (reg_value <= 1) {
                cfg_settings.enable_discharge = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 486:
            if (reg_value <= 0x960) {
                cfg_settings.charge_slot_1_start = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 487:
            if (reg_value <= 0x960) {
                cfg_settings.charge_slot_1_end = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 488:
            if (reg_value <= 1) {
                cfg_settings.enable_charge = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 489:
            if (reg_value <= 0x64) {
                cfg_settings.active_power_rate = reg_value;
                cfg_settings.hr121 = 0;
            } else {
                write_error = 1;
            }
            break;
        case 490:
            if ((uchar)reg_value == 0) {
                cfg_settings.inverter_state = 0;
            } else {
                if ((uchar)reg_value == 1) {
                    cfg_settings.inverter_state = 1;
                } else {
                    write_error = 1;
                }
            }
            if ((reg_value & 0xff00) == 0x100) {
                cfg_settings.f09 = 1;
            } else {
                if ((reg_value & 0xff00) == 0) {
                    cfg_settings.f09 = 0;
                } else {
                    write_error = 1;
                }
            }
            break;
        case 494:
            if (reg_value >= 1) {
                cfg_block6.hr494 = reg_value;
            } else {
                cfg_block6.hr494 = 0;
            }
            break;
        case 495:
            if (reg_value < 0x10000) {
                batt_power_remote = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 512:
            if (reg_value <= 0x2710) {
                cfg_settings.battery_capacity = reg_value;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
            break;
        case 515:
            if (reg_value >= 1 && reg_value <= 0x64) {
                cfg_settings.hr314 = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 4000:
            if (reg_value < 0x10000) {
                hr4000_value = reg_value;
            } else {
                write_error = 1;
            }
            break;
        case 4097:
            if (reg_value == 0x400 || cal_gains.gain0 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain0 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4098:
            if (reg_value == 0x400 || cal_gains.gain2 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain2 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4099:
            if (reg_value == 0x400 || cal_gains.gain4 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain4 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4101:
            if (reg_value == 0x400 || cal_gains.gain5 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain5 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4102:
            energy_totals.batt_discharge_total = reg_value << 0x10;
            energy_totals.batt_charge_total = reg_value << 0x10;
            break;
        case 4103:
            energy_totals.batt_discharge_total =
                energy_totals.batt_discharge_total & 0xffff0000 | (reg_value & 0xffff) << 0;
            energy_totals.batt_charge_total =
                energy_totals.batt_charge_total & 0xffff0000 | (reg_value & 0xffff) << 0;
            eeprom_dirty = eeprom_dirty | 8;
            break;
        case 4104:
            if (reg_value == 0x400 || cal_gains.gain1 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain1 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4105:
            if (reg_value == 0x400 || cal_gains.gain3 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain3 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4106:
            if (reg_value == 0x400 || cal_gains.gain6 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain6 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4107:
            energy_totals.pv_total = reg_value << 0x10;
            break;
        case 4108:
            energy_totals.pv_total = energy_totals.pv_total & 0xffff0000 | (reg_value & 0xffff)
                                                                               << 0;
            eeprom_dirty = eeprom_dirty | 8;
            break;
        case 4120:
            if (reg_value == 0x400 || cal_gains.gain7 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain7 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4140:
            energy_totals.inv_out_day = reg_value;
            eeprom_dirty = eeprom_dirty | 8;
            break;
        case 4141:
            energy_totals.inv_out_total = reg_value << 0x10;
            break;
        case 4142:
            energy_totals.inv_out_total =
                energy_totals.inv_out_total & 0xffff0000 | (reg_value & 0xffff) << 0;
            eeprom_dirty = eeprom_dirty | 8;
            break;
        case 4143:
            energy_totals.work_time = reg_value << 0x10;
            break;
        case 4144:
            energy_totals.work_time = energy_totals.work_time & 0xffff0000 | (reg_value & 0xffff)
                                                                                 << 0;
            eeprom_dirty = eeprom_dirty | 8;
            break;
        case 4146:
            if (reg_value == 0x400 || cal_gains.gain8 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain8 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4354:
            if (reg_value == 0 || cal_gains.offset0 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset0 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4355:
            if (reg_value == 0 || cal_gains.offset2 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset2 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4356:
            if (reg_value == 0 || cal_gains.offset4 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset4 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4358:
            if (reg_value == 0 || cal_gains.offset5 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset5 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4361:
            if (reg_value == 0 || cal_gains.offset1 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset1 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4362:
            if (reg_value == 0 || cal_gains.offset3 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset3 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4363:
            if (reg_value == 0 || cal_gains.offset6 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset6 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4366:
            if (reg_value == 0 || cal_gains.offset7 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset7 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4378:
            if (reg_value == 0 || cal_gains.offset8 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset8 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4380:
            if (reg_value == 0x400 || cal_gains.gain11 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain11 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4381:
            if (reg_value == 0 || cal_gains.offset11 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset11 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4382:
            if (reg_value == 0x400 || cal_gains.gain9 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain9 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4383:
            if (reg_value == 0 || cal_gains.offset9 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset9 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4384:
            if (reg_value == 0x400 || cal_gains.gain10 == 0x400) {
                if (reg_value >= 0x366 && reg_value <= 0x499) {
                    cal_gains.gain10 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        case 4385:
            if (reg_value == 0 || cal_gains.offset10 == 0) {
                if (reg_value <= 0x64) {
                    cal_gains.offset10 = reg_value;
                    eeprom_dirty = eeprom_dirty | 2;
                } else {
                    write_error = 1;
                }
            } else {
                write_error = 1;
            }
            break;
        default:
            write_error = 1;
        }
        reg_addr = (ushort)(reg_addr + 1);
        byte_hi_idx = (ushort)((reg_idx + reg_idx) + 6);
        byte_lo_idx = (ushort)((reg_idx + reg_idx) + 7);
        if (byte_hi_idx > 0x96) {
            byte_hi_idx = 0x96;
        }
        if (byte_lo_idx > 0x96) {
            byte_lo_idx = 0x96;
        }
        value_hi = req[byte_hi_idx];
        value_lo = req[byte_lo_idx];
        reg_value = value_lo | value_hi << 8;
    }
    if (write_error == 0) {
        crc = crc16_modbus(reply, 0x10);
        reply[0x10] = crc;
        reply[0x11] = crc >> 8;
    } else {
        reply[1] = 0x90;
        crc = crc16_modbus(reply, 0x10);
        reply[0x10] = crc;
        reply[0x11] = crc >> 8;
        modbus_err_reply = 1;
    }
}
#undef AAE
#undef PX
#undef NVIC

/* match: AAE.t[i][j] is cfg_settings.inverter_serial[i][j] and PX the meters' day-start counters
   (energy_totals.meter_in_day_start, reached from cfg_settings' address); written as those
   fields the bytes differ */
#define AAE (*(struct aae *)&cfg_settings)

#define PX ((*(aaeb_t *)&cfg_settings).a)
#define NVIC ((nvic_t *)NVIC_ISER_ADDR)
/* Function 6: writes one holding register. A value outside the register's range is refused with
   the 0x86 error reply (a few return code 2 instead, which also answers with the error). Writes
   set bits in eeprom_dirty; modbus_slave_process then asks task_monitor to save after 100
   passes. Groups, by register:

     1, 2       model and grid code; a valid HR2 reloads all defaults of blocks 1 and 6
     7          meters present (bit mask); bit 0 also sets HR176 bit 3
     13-17      the serial number, two characters each; only HR17 marks it for saving
     20, 26-33, 41-62, 94-116, 121-145   block 1 settings, each with its range
     23, 24     the peek: chip (0 DSP, 1 ARM) and address; HR25 then reads it
     35-40      the clock: HR35 starts a set (year 0-99 or 2000+), HR40 writes the RTC
     63-83      grid limits: the voltage and frequency ones go through cfg_limit_check, which
                accepts everything; the times must be 1..50000
     162        1: clear the energy counters, fault log and event log
     163        1/2: factory reset of blocks 1 and 6 (2 also the gains), 3: gains only,
                100/101/200: cfg_reset_mode
     164        1: jump to the bootloader (not for slave 0); 0x100: stop the DSP link and hang
     166        1 keeps most battery and schedule writes in RAM only (no EEPROM save)
     175, 200, 201, 329, 495, 4000   RAM only, not saved
     180-186    block 8, raw words
     230-299    block 6: charge slots b1-b9 (243-269), discharge slots a1-a8 (276-299), their
                SOCs and HR242/272/275 (the slot 1 SOCs), times <= 2400, SOCs <= 100
     300-332, 351-353   more block 1 and block 6 settings; 332 switches battery type
     360-407    sched_table_b, 420-467 cfg_hr360_table: the half-hour tables, RAM only
     481-490    RAM-only copies of HR27, 29, 56, 57, 59, 94, 95, 96, 50 and 53: the same
                checks, no EEPROM write (inference: for frequent remote control)
     494, 495   remote battery power mode and setpoint (494 is stored but not marked for saving)
     512, 515   aliases of HR55 and HR314
     4097-4385  calibration gains (0x366..0x499) and offsets (0..100), counters 4102-4144

   The gain and offset rule: a gain may only change when it is 1.0 (0x400) now or is being set to
   1.0; an offset only when it is 0 now or is being set to 0. So a calibration must be reset
   before it can be changed. */
void modbus_fc06_write_reg(uchar *req) {
    ushort reg_value;
    uchar i;
    ushort crc;
    int clear_len;
    uchar *clear_data;
    int reg_addr;
    int write_error;

    write_error = 0;
    reg_addr = req[2];
    reg_addr = req[3] | reg_addr << 8;
    reg_value = req[4];
    reg_value = req[5] | reg_value << 8;
    for (i = 0; i < 0xa; i++) {
        modbus_reply[2 + i] = cfg_serial_and_settings[i];
    }
    for (i = 0; i < 4; i++) {
        modbus_reply[0xc + i] = *(req + 2 + i);
    }
    switch (reg_addr) {
    case 0:
        break;
    case HR_INVERTER_MODULE_H:
        cfg_settings.inverter_module_h = reg_value;
        eeprom_dirty = 0x11;
        break;
    case HR_INVERTER_MODULE_L:
        if (0x1d > (int)((uint)reg_value >> 8) && (uchar)reg_value <= 0x32) {
            cfg_settings.inverter_module_l = reg_value;
            cfg_load_defaults(1);
            cfg_load_defaults(6);
            eeprom_dirty = eeprom_dirty | 0x31;
            cfg_changed_flags = cfg_changed_flags | 1;
            cfg_reset_mode = 0;
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_AMMETER:
        if (reg_value <= 0xff) {
            cfg_settings.enable_ammeter = reg_value;
            if ((cfg_settings.enable_ammeter & 1) != 0) {
                cfg_settings.hr176 = (cfg_settings.hr176 & ~8) + 8;
            } else {
                cfg_settings.hr176 = cfg_settings.hr176 & ~8;
            }
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 8:
        write_error = 1;
        break;
    case 9:
        write_error = 1;
        break;
    case 10:
        write_error = 1;
        break;
    /* HR13-17: the 10-character serial, two characters a register, high one first. */
    case 13:
        AAE.t[0][0] = (uint)reg_value >> 8;
        AAE.t[0][1] = reg_value;
        break;
    case 14:
        AAE.t[1][0] = (uint)reg_value >> 8;
        AAE.t[1][1] = reg_value;
        break;
    case 15:
        AAE.t[2][0] = (uint)reg_value >> 8;
        AAE.t[2][1] = reg_value;
        break;
    case 16:
        AAE.t[3][0] = (uint)reg_value >> 8;
        AAE.t[3][1] = reg_value;
        break;
    case 17:
        AAE.t[4][0] = (uint)reg_value >> 8;
        AAE.t[4][1] = reg_value;
        eeprom_dirty = eeprom_dirty | 1;
        break;
    case HR_ENABLE_CHARGE_TARGET:
        if (reg_value <= 1) {
            cfg_settings.enable_charge_target = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    /* HR23/HR24: the peek. HR23 chooses the chip (0 DSP, 1 ARM); HR24 is the address, 0xC000-0xE000
     for the DSP; for the ARM any value, read by modbus_hr_update at 0x20000000 + HR24. */
    case 23:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 1) {
            peek_chip[0] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 24:
        if (reg_value >= 0xc000 && reg_value <= 0xe000 && peek_chip[0] == 0) {
            peek_addr = reg_value;
        } else {
            if ((reg_value > 0 || reg_value == 0) && reg_value < 0x10000 && peek_chip[0] == 1) {
                peek_addr = reg_value;
            } else {
                write_error = 1;
            }
        }
        break;
    case HR_GRID_PORT_MAX_POWER:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0xea60) {
            cfg_settings.grid_port_max_power = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_POWER_MODE:
        if (reg_value <= 1) {
            cfg_settings.battery_power_mode = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_60HZ_FREQ_MODE:
        if (reg_value <= 1) {
            cfg_settings.enable_60hz_freq_mode = reg_value;
            grid_load_country_limits();
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR29 calibration stage: only with lithium (HR54 = 1); a non-zero value is ORed into the stored
     bits, 0 clears them. task_dsp_rx advances it from the DSP's warning bits. */
    case HR_SOC_FORCE_ADJUST:
        if (reg_value <= 7 && cfg_settings.battery_type == 1) {
            if (reg_value > 0) {
                cfg_settings.soc_force_adjust = cfg_settings.soc_force_adjust | (uchar)reg_value;
            } else {
                cfg_settings.soc_force_adjust = reg_value;
            }
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_MODBUS_ADDRESS:
        if (reg_value > 0 && reg_value <= 0xff) {
            cfg_settings.modbus_address = reg_value;
            eeprom_dirty = eeprom_dirty | 0x11;
        } else {
            write_error = 1;
        }
        break;
    case 31:
        if (reg_value < 5) {
            hr31_value = reg_value;
            cfg_settings.hr31 = cfg_settings.hr31 & 0xf;
            cfg_settings.hr31 = cfg_settings.hr31 | 0xf0 & hr31_value << 4;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_USER_CODE:
        if (reg_value <= 0x80) {
            cfg_settings.user_code = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR35-40: setting the clock. HR35 holds RTC_Get off (rtc_set_pending), HR40 writes the RTC
     and the "set" mark. */
    case 35:
        rtc_set_pending = 1;
        if (reg_value > 0 && reg_value <= 0x2710) {
            if (reg_value <= 0x63) {
                rtc_time.year = reg_value;
            } else {
                rtc_time.year = reg_value - 0x7d0;
            }
        } else {
            write_error = 1;
        }
        break;
    case 36:
        if (reg_value > 0 && reg_value <= 0xc) {
            rtc_time.month = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 37:
        if (rtc_time.month == 1 || rtc_time.month == 3 || rtc_time.month == 5 ||
            rtc_time.month == 7 || rtc_time.month == 8 || rtc_time.month == 0xa ||
            rtc_time.month == 0xc) {
            if (reg_value > 0 && reg_value <= 0x1f) {
                rtc_time.date = reg_value;
            } else {
                write_error = 1;
            }
        } else {
            if (rtc_time.month == 4 || rtc_time.month == 6 || rtc_time.month == 9 ||
                rtc_time.month == 0xb) {
                if (reg_value > 0 && reg_value <= 0x1e) {
                    rtc_time.date = reg_value;
                } else {
                    write_error = 1;
                }
            } else {
                if ((rtc_time.year + 0x7d0) % 0x190 == 0 ||
                    ((rtc_time.year + 0x7d0) % 4 == 0 && (rtc_time.year + 0x7d0) % 0x64 != 0)) {
                    if (reg_value > 0 && reg_value <= 0x1d) {
                        rtc_time.date = reg_value;
                    } else {
                        write_error = 1;
                    }
                } else {
                    if (reg_value > 0 && reg_value <= 0x1c) {
                        rtc_time.date = reg_value;
                    } else {
                        write_error = 1;
                    }
                }
            }
        }
        break;
    case 38:
        if (reg_value <= 0x17) {
            rtc_time.hour = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 39:
        if (reg_value <= 0x3b) {
            rtc_time.min = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 40:
        if (reg_value <= 0x3b) {
            rtc_time.sec = reg_value;
        } else {
            write_error = 1;
        }
        RTC_EnterConfigMode();
        RTC_WaitForLastTask();
        RTC_Set(rtc_time.year + 0x7d0, rtc_time.month, rtc_time.date, rtc_time.hour, rtc_time.min,
                rtc_time.sec);
        RTC_ExitConfigMode();
        BKP_WriteBackupRegister(4, 0xa5a5);
        rtc_set_pending = 0;
        break;
    case HR_ENABLE_DRM_RJ45:
        if (reg_value <= 1) {
            cfg_settings.enable_drm_rj45 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_REVERSE_CT_CLAMP:
        if (reg_value <= 3) {
            cfg_settings.enable_reverse_ct_clamp = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_DISCHARGE_SLOT_2_START:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.discharge_slot_2_start = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_DISCHARGE_SLOT_2_END:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.discharge_slot_2_end = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 46:
        break;
    case HR_METER_TYPE:
        if (reg_value < 2) {
            cfg_settings.meter_type = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR48/HR49: reverse the direction of meters (bit per meter). For each bit that changes, the
     meter's import and export day-start counters are swapped. The "== 0xAA" branch is never
     reached: 0xAA already passes the "<= 0xFF" test. */
    case HR_REVERSE_115_METER_DIR:
        if (reg_value <= 0xff) {
            meter_dir_changed = cfg_settings.reverse_115_meter_dir ^ reg_value;
            for (i = 0; i < 8; i++) {
                if ((meter_dir_changed & 1 << i) == 1 << i) {
                    meter_swap_a = PX[0][i];
                    meter_swap_b = PX[1][i];
                    PX[0][i] = meter_swap_b;
                    PX[1][i] = meter_swap_a;
                }
            }
            cfg_settings.reverse_115_meter_dir = reg_value;
            cfg_settings.fA2 = 3;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            if (reg_value == 0xaa) {
                cfg_settings.fA2 = 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
        }
        break;
    case HR_REVERSE_418_METER_DIR:
        if (reg_value <= 0xff) {
            meter_dir_changed = cfg_settings.reverse_418_meter_dir ^ reg_value;
            for (i = 0; i < 8; i++) {
                if ((1 << i == 1 << i & meter_dir_changed) != 0) {
                    meter_swap_a = PX[0][i];
                    meter_swap_b = PX[1][i];
                    PX[0][i] = meter_swap_b;
                    PX[1][i] = meter_swap_a;
                }
            }
            cfg_settings.reverse_418_meter_dir = reg_value;
            cfg_settings.fA2 = 3;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            if (reg_value == 0xaa) {
                cfg_settings.reverse_418_meter_dir = 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
        }
        break;
    case HR_ACTIVE_POWER_RATE:
        if (reg_value <= 0x64) {
            cfg_settings.active_power_rate = reg_value;
            cfg_settings.hr121 = 0;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_REACTIVE_POWER_RATE:
        if (reg_value <= 0x3c) {
            cfg_settings.reactive_power_rate = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_POWER_FACTOR:
        if (reg_value <= 0x4e20) {
            cfg_settings.power_factor = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_INVERTER_STATE:
        if ((uchar)reg_value == 0) {
            cfg_settings.inverter_state = 0;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            if ((uchar)reg_value == 1) {
                cfg_settings.inverter_state = 1;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
        }
        if ((reg_value & 0xff00) == 0x100) {
            cfg_settings.f09 = 1;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            if ((reg_value & 0xff00) == 0) {
                cfg_settings.f09 = 0;
                eeprom_dirty = eeprom_dirty | 1;
            } else {
                write_error = 1;
            }
        }
        break;
    /* HR54 battery type: 0 (lead acid) also clears HR29 and HR58 and sets HR111 to 20; 1 (lithium)
     sets HR111 to 50. */
    case HR_BATTERY_TYPE:
        if (reg_value <= 1) {
            cfg_settings.battery_type = reg_value;
            if (cfg_settings.battery_type == 0) {
                cfg_settings.soc_force_adjust = 0;
                cfg_settings.battery_charge_limit = 0x14;
                cfg_settings.enable_auto_judge_battery_type = 0;
            } else {
                if (cfg_settings.battery_type == 1) {
                    cfg_settings.battery_charge_limit = 0x32;
                }
            }
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_CAPACITY:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x2710) {
            cfg_settings.battery_capacity = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_DISCHARGE_SLOT_1_START:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.discharge_slot_1_start = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_DISCHARGE_SLOT_1_END:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.discharge_slot_1_end = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_AUTO_JUDGE_BATTERY_TYPE:
        if (reg_value <= 1) {
            cfg_settings.enable_auto_judge_battery_type = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_DISCHARGE:
        if (reg_value <= 1) {
            cfg_settings.enable_discharge = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_V_PV_INPUT_START:
        if (reg_value >= 0x320 && reg_value <= 0x2710) {
            cfg_settings.v_pv_input_start = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_INVERTER_START_TIME:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x258) {
            cfg_settings.inverter_start_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_INVERTER_RESTART_DELAY:
        if (reg_value <= 0x384) {
            cfg_settings.inverter_restart_delay = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* cfg_limit_check returns 1 for every value, so the grid voltage and frequency limits accept
     anything. */
    case HR_V_AC_LOW_OUT:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_low_out = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_HIGH_OUT:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_high_out = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_LOW_OUT:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_low_out = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_HIGH_OUT:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_high_out = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_LOW_OUT_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.v_ac_low_out_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_HIGH_OUT_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.v_ac_high_out_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_LOW_OUT_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.f_ac_low_out_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_HIGH_OUT_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.f_ac_high_out_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_LOW_IN:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_low_in = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_HIGH_IN:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_high_in = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_LOW_IN:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_low_in = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_HIGH_IN:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_high_in = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_LOW_IN_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.v_ac_low_in_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_HIGH_IN_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.v_ac_high_in_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_LOW_IN_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.f_ac_low_in_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_HIGH_IN_TIME:
        if (reg_value >= 1 && reg_value <= 0xc350 && 1) {
            cfg_settings.f_ac_high_in_time = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_LOW_C:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_low_c = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_AC_HIGH_C:
        if (cfg_limit_check(grid_country, 0, reg_value) == 1) {
            cfg_settings.v_ac_high_c = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_LOW_C:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_low_c = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_F_AC_HIGH_C:
        if (cfg_limit_check(grid_country, 1, reg_value) == 1) {
            cfg_settings.f_ac_high_c = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_V_10_MIN_PROTECTION:
        if (reg_value <= 0xbb8) {
            cfg_settings.v_10_min_protection = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_CHARGE_SLOT_1_START:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.charge_slot_1_start = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_CHARGE_SLOT_1_END:
        if ((reg_value > 0 || reg_value == 0) && reg_value <= 0x960) {
            cfg_settings.charge_slot_1_end = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_ENABLE_CHARGE:
        if (reg_value <= 1) {
            cfg_settings.enable_charge = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    /* HR97/HR98 battery voltage limits: any value up to 0xFFDC, no check against each other.
     cfg_load_all overwrites both at every boot (43.20 V and 58.50 V; docs/findings.md). */
    case HR_BATTERY_LOW_VOLTAGE_LIMIT:
        if (reg_value <= 0xffdc) {
            cfg_settings.battery_low_voltage_limit = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_HIGH_VOLTAGE_LIMIT:
        if (reg_value <= 0xffdc) {
            cfg_settings.battery_high_voltage_limit = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 101:
        if (reg_value <= 0xff && reg_value >= 1) {
            cfg_settings.hr101 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 102:
        if (reg_value <= 1) {
            cfg_settings.hr102 = cfg_settings.hr102 & 0xffffffbf | (reg_value & 1) << 6;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 108:
        if (reg_value <= 0x64) {
            cfg_settings.hr108 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 109:
        if (reg_value <= 0x64) {
            cfg_settings.hr109 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_SOC_RESERVE:
        if (reg_value < 0x64) {
            cfg_settings.battery_soc_reserve = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_CHARGE_LIMIT:
        if (reg_value <= 0xfa) {
            cfg_settings.battery_charge_limit = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_DISCHARGE_LIMIT:
        if (reg_value <= 0xfa) {
            cfg_settings.battery_discharge_limit = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 113:
        if (reg_value <= 1) {
            cfg_settings.hr113 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_BATTERY_DISCHARGE_MIN_POWER_RESERVE:
        if (reg_value <= 0x64) {
            cfg_settings.battery_discharge_min_power_reserve = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 115:
        if (reg_value <= 1) {
            cfg_settings.hr115 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case HR_CHARGE_TARGET_SOC:
        if (reg_value <= 0x64) {
            cfg_settings.charge_target_soc = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 121:
        if (reg_value <= 0xff) {
            cfg_settings.hr121 = reg_value;
            if (reg_value == 1) {
                cfg_settings.hr121 = 3;
            }
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 2;
        }
        break;
    case 122:
        if (reg_value <= 0xa) {
            cfg_settings.hr122 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 123:
        if (reg_value >= 5 && reg_value <= 0x64) {
            cfg_settings.hr123 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 124:
        if (reg_value <= 1) {
            cfg_block6.hr124 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
            write_error = 0;
        } else {
            write_error = 2;
        }
        break;
    case 125:
        if (reg_value <= 3) {
            cfg_settings.hr125 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 126:
        if (reg_value > 1) {
            write_error = 1;
        }
        break;
    case 127:
        if (reg_value == 1 && grid_country == 4) {
            auto_test_state = 1;
            write_error = 0;
        } else {
            write_error = 2;
        }
        break;
    case 128:
        if (reg_value <= 1) {
            cfg_settings.hr128 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 129:
        if (reg_value == 1) {
            cfg_settings.hr129 = 1;
        } else {
            if (reg_value == 0) {
                cfg_settings.hr129 = 0;
            } else {
                write_error = 1;
            }
        }
        break;
    case 130:
        if (reg_value > 0 && reg_value <= 0x12c) {
            cfg_settings.hr130 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 131:
        if (reg_value > 0 && reg_value <= 0xc350) {
            cfg_settings.hr131 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 132:
        if (reg_value > 0 && reg_value <= 0x12c) {
            cfg_settings.hr132 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 133:
        if (reg_value > 0 && reg_value <= 0xc350) {
            cfg_settings.hr133 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 134:
        if (reg_value > 0 && reg_value <= 0x12c) {
            cfg_settings.hr134 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 135:
        if (reg_value > 0 && reg_value <= 0xc350) {
            cfg_settings.hr135 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 136:
        if (reg_value > 0 && reg_value <= 0x12c) {
            cfg_settings.hr136 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 137:
        if (reg_value > 0 && reg_value <= 0xc350) {
            cfg_settings.hr137 = reg_value;
            if (cfg_settings.hr129 == 1) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 138:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr138 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 139:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr139 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 140:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr140 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 141:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr141 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 142:
        if (reg_value <= 0x64) {
            cfg_settings.hr142 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 143:
        if (reg_value <= 0x64) {
            cfg_settings.hr143 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 144:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr144 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 145:
        if (reg_value <= 0xbb8) {
            cfg_settings.hr145 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR162 = 1: clear the energy counters, the fault log and the event log. */
    case 162:
        if (reg_value == 1) {
            clear_len = 0xbd;
            clear_data = (uchar *)&energy_totals;
            for (i = 0; i < clear_len; i++) {
                clear_data[i] = 0;
            }
            clear_len = 0x2d;
            clear_data = &fault_log[0];
            for (i = 0; i < clear_len; i++) {
                clear_data[i] = 0;
            }
            for (i = 0; i < 0xfa; i++) {
                event_log[i] = 0;
            }
            eeprom_dirty = eeprom_dirty | 0xe;
        } else {
            write_error = 1;
        }
        break;
    /* HR163: 1 or 2 clears the counters and logs and reloads the defaults of blocks 1 and 6 (2 also
     resets the calibration); 3 resets the calibration only; 100, 101 and 200 set cfg_reset_mode. */
    case 163:
        if (reg_value == 1 || reg_value == 2) {
            clear_len = 0xbd;
            clear_data = (uchar *)&energy_totals;
            for (i = 0; i < clear_len; i++) {
                clear_data[i] = 0;
            }
            clear_len = 0x2d;
            clear_data = &fault_log[0];
            for (i = 0; i < clear_len; i++) {
                clear_data[i] = 0;
            }
            for (i = 0; i < 0xfa; i++) {
                event_log[i] = 0;
            }
            cfg_load_defaults(1);
            cfg_load_defaults(6);
            if (reg_value == 2) {
                calib_table_defaults();
            }
            eeprom_dirty = eeprom_dirty | 0x1f;
            cfg_changed_flags = cfg_changed_flags | 1;
            cfg_reset_mode = 0;
        } else {
            if (reg_value == 3) {
                calib_table_defaults();
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                if (reg_value == 0x64) {
                    cfg_reset_mode = 0;
                    cfg_changed_flags = cfg_changed_flags | 1;
                } else {
                    if (reg_value == 0x65) {
                        cfg_reset_mode = 0;
                    } else {
                        if (reg_value == 0xc8) {
                            cfg_reset_mode = 1;
                            cfg_changed_flags = cfg_changed_flags | 1;
                        } else {
                            write_error = 1;
                        }
                    }
                }
            }
        }
        break;
    /* HR164 (not as a broadcast): 1 turns the LEDs and SysTick off, masks every IRQ, points VTOR at
     0x08000000 and calls the address in the word at 0x0000002C (the boot area's vector 11), that
     is, enters the bootloader (inference). 0x100 stops the DSP tasks and hangs this task. */
    case 164:
        if (*req != 0) {
            if (reg_value == 1) {
                led_set(1, 0);
                led_set(2, 0);
                systick_int_disable();
                systick_disable();
                NVIC->ICER[0] = 0xffffffff;
                NVIC->ICER[1] = 0xffffffff;
                NVIC_SetVectorTable(0x8000000, 0);
                ((code *)*(volatile uint *)0x2c)();
            } else {
                if (reg_value == 0x100) {
                    os_task_delay((uchar)os_task_dly_idx, 0x12c, 5);
                    task_tbl[TASK_DSP_RX].period = 0;
                    task_tbl[TASK_DSP_TX].period = 0;
                    while (1) {
                    }
                }
                write_error = 1;
            }
        }
        break;
    /* HR166: while 1, writes of HR20, 27, 50, 56, 57, 59, 94, 95, 96, 110-112, 116 and 318-320
     change RAM but are not saved. */
    case 166:
        if (reg_value <= 1 && cfg_settings.hr166 != reg_value) {
            cfg_settings.hr166 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 167:
        break;
    case 168:
        break;
    case 169:
        break;
    case 170:
        break;
    case 171:
        break;
    case 175:
        if (reg_value <= 1) {
            batt_cal_request = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 176:
        if (reg_value <= 0xff) {
            cfg_settings.hr176 = cfg_settings.hr176 & 0xffffff00 | (reg_value & 0xff) << 0;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 177:
        if (reg_value <= 1) {
            cfg_settings.hr177 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 178:
        if (reg_value <= 1) {
            cfg_settings.hr178 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 179:
        if (reg_value <= 1) {
            cfg_settings.hr179 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR180-186: 14 raw bytes at the start of block 8. */
    case 180:
        cfg_block8[0] = (uint)reg_value >> 8;
        cfg_block8[1] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 181:
        cfg_block8[2] = (uint)reg_value >> 8;
        cfg_block8[3] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 182:
        cfg_block8[4] = (uint)reg_value >> 8;
        cfg_block8[5] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 183:
        cfg_block8[6] = (uint)reg_value >> 8;
        cfg_block8[7] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 184:
        cfg_block8[8] = (uint)reg_value >> 8;
        cfg_block8[9] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 185:
        cfg_block8[10] = (uint)reg_value >> 8;
        cfg_block8[11] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 186:
        cfg_block8[12] = (uint)reg_value >> 8;
        cfg_block8[13] = reg_value;
        eeprom_dirty = eeprom_dirty | 0x80;
        break;
    case 200:
        if (reg_value <= 1) {
            hr200_request = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 201:
        if (reg_value <= 1) {
            hr201_request = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 230:
        if (reg_value <= 1) {
            cfg_block6.hr230 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 240:
        break;
    case 241:
        break;
    case 242:
        if (reg_value <= 0x64) {
            cfg_block6.hr242 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    /* HR243-269: charge slots b1-b9 (start, end HHMM <= 2400, SOC <= 100). */
    case 243:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b1_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 244:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b1_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 245:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b1_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 246:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b2_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 247:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b2_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 248:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b2_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 249:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b3_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 250:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b3_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 251:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b3_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 252:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b4_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 253:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b4_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 254:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b4_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 255:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b5_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 256:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b5_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 257:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b5_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 258:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b6_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 259:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b6_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 260:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b6_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 261:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b7_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 262:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b7_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 263:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b7_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 264:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b8_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 265:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b8_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 266:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b8_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 267:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b9_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 268:
        if (reg_value <= 0x960) {
            cfg_block6.slot_b9_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 269:
        if (reg_value <= 0x64) {
            cfg_block6.slot_b9_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 270:
        break;
    case 271:
        break;
    case 272:
        if (reg_value <= 0x64) {
            cfg_block6.hr272 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 273:
        break;
    case 274:
        break;
    case 275:
        if (reg_value <= 0x64) {
            cfg_block6.hr275 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    /* HR276-299: discharge slots a1-a8, the same layout. */
    case 276:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a1_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 277:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a1_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 278:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a1_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 279:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a2_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 280:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a2_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 281:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a2_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 282:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a3_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 283:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a3_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 284:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a3_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 285:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a4_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 286:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a4_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 287:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a4_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 288:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a5_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 289:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a5_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 290:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a5_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 291:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a6_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 292:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a6_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 293:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a6_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 294:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a7_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 295:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a7_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 296:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a7_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 297:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a8_start = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 298:
        if (reg_value <= 0x960) {
            cfg_block6.slot_a8_end = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 299:
        if (reg_value <= 0x64) {
            cfg_block6.slot_a8_soc = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 300:
        if (reg_value <= 0x7530) {
            cfg_settings.hr300 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 301:
        if (reg_value <= 0x7530) {
            cfg_settings.hr301 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 302:
        if (reg_value >= 0xa && reg_value <= 0x7530) {
            cfg_settings.hr302 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 303:
        if (reg_value <= 0x7530) {
            cfg_settings.hr303 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 304:
        break;
    case 305:
        if (reg_value <= 4) {
            cfg_settings.hr305 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 306:
        if (reg_value >= 0xa && reg_value <= 0x7530) {
            cfg_settings.hr306 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 307:
        if (reg_value > 0x7d0 && reg_value <= 0x960) {
            cfg_settings.hr307 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 308:
        if (reg_value > 0x1f4 && reg_value <= 0x1770) {
            cfg_settings.hr308 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 309:
        if (reg_value >= 0xa && reg_value <= 0x50) {
            cfg_settings.hr309 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 310:
        if (reg_value >= 0x14 && reg_value <= 0x64) {
            cfg_settings.hr310 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 311:
        if (reg_value <= 2) {
            cfg_settings.hr311 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 312:
        if (reg_value <= 0x7530) {
            cfg_settings.hr312 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 313:
        if (reg_value >= 1 && reg_value <= 0x64) {
            cfg_settings.hr313 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 314:
        if (reg_value >= 1 && reg_value <= 0x64) {
            cfg_settings.hr314 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 315:
        if (reg_value <= 0xc80) {
            cfg_settings.hr315 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 316:
        if (reg_value <= 0xc80) {
            cfg_settings.hr316 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 317:
        if (reg_value <= 1) {
            cfg_settings.hr102 = cfg_settings.hr102 & 0xfffffffe | (reg_value & 1) << 0;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 318:
        if (reg_value <= 3) {
            cfg_settings.hr318 = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 319:
        if (reg_value <= 0x960) {
            cfg_settings.hr319 = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 320:
        if (reg_value <= 0x960) {
            cfg_settings.hr320 = reg_value;
            if (cfg_settings.hr166 == 0) {
                eeprom_dirty = eeprom_dirty | 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 321:
        if (reg_value >= 0x139c && reg_value <= 0x1450) {
            cfg_settings.hr321 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 322:
        if (reg_value <= 1) {
            cfg_block6.hr322 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 323:
        if (reg_value < 0x4e20) {
            cfg_block6.hr323 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 324:
        if (reg_value < 0x4e20) {
            cfg_block6.hr324 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 325:
        if (reg_value < 0x4e20) {
            cfg_block6.hr325 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 326:
        if (reg_value >= 0x11f8 && reg_value <= 0x1388) {
            cfg_settings.hr326 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 327:
        if (reg_value >= 0x11 && reg_value <= 0x64) {
            cfg_settings.hr327 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 328:
        if (reg_value >= 0x1388 && reg_value <= 0x1450) {
            cfg_settings.hr328 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 329:
        if (reg_value <= 1) {
            force_state_hr329 = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 330:
        if (reg_value <= 0xff) {
            cfg_settings.hr330 = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 331:
        if (reg_value <= 0xff) {
            cfg_settings.hr331 = cfg_settings.hr331 & 0xffffff00 | (reg_value & 0xff) << 0;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    /* HR332: 1 switches to lead acid (HR54 = 0, 125 Ah, +0xD2 = 1), 0 back to lithium. */
    case 332:
        if (reg_value < 2) {
            if (reg_value == 0) {
                cfg_settings.hr305 = 0;
                cfg_settings.fD2 = 0;
                cfg_settings.battery_type = 1;
            } else {
                cfg_settings.hr305 = 1;
                cfg_settings.fD2 = 1;
                cfg_settings.battery_type = 0;
                cfg_settings.battery_capacity = 0x7d;
            }
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 351:
        if (reg_value <= 1) {
            cfg_block6.hr351 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 352:
        if (reg_value <= 1) {
            cfg_block6.hr352 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    case 353:
        if (reg_value < 0xffff) {
            cfg_block6.hr353 = reg_value;
            eeprom_dirty = eeprom_dirty | 0x20;
        } else {
            write_error = 1;
        }
        break;
    /* HR360-407: the second half-hour table (sched_table_b, written as cfg_hr360_table[1]), values
     below 20000. Neither table is in an EEPROM block, so both are zero after a restart. */
    case 360:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][0] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 361:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][1] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 362:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][2] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 363:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][3] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 364:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][4] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 365:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][5] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 366:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][6] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 367:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][7] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 368:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][8] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 369:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][9] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 370:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][10] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 371:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][11] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 372:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][12] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 373:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][13] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 374:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][14] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 375:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][15] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 376:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][16] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 377:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][17] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 378:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][18] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 379:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][19] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 380:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][20] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 381:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][21] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 382:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][22] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 383:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][23] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 384:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][24] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 385:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][25] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 386:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][26] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 387:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][27] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 388:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][28] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 389:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][29] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 390:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][30] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 391:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][31] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 392:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][32] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 393:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][33] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 394:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][34] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 395:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][35] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 396:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][36] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 397:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][37] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 398:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][38] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 399:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][39] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 400:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][40] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 401:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][41] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 402:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][42] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 403:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][43] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 404:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][44] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 405:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][45] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 406:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][46] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 407:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[1][47] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 408:
        break;
    case 409:
        break;
    case 410:
        break;
    case 411:
        break;
    case 412:
        break;
    case 413:
        break;
    case 414:
        break;
    case 415:
        break;
    case 416:
        break;
    case 417:
        break;
    /* HR420-467: the first half-hour table (cfg_hr360_table), values below 20000; RAM only. */
    case 420:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][0] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 421:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][1] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 422:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][2] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 423:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][3] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 424:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][4] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 425:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][5] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 426:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][6] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 427:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][7] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 428:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][8] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 429:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][9] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 430:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][10] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 431:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][11] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 432:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][12] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 433:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][13] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 434:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][14] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 435:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][15] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 436:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][16] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 437:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][17] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 438:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][18] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 439:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][19] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 440:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][20] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 441:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][21] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 442:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][22] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 443:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][23] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 444:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][24] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 445:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][25] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 446:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][26] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 447:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][27] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 448:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][28] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 449:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][29] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 450:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][30] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 451:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][31] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 452:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][32] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 453:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][33] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 454:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][34] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 455:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][35] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 456:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][36] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 457:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][37] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 458:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][38] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 459:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][39] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 460:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][40] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 461:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][41] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 462:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][42] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 463:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][43] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 464:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][44] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 465:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][45] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 466:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][46] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 467:
        if (reg_value < 0x4e20) {
            cfg_hr360_table[0][47] = reg_value;
        } else {
            write_error = 1;
        }
        break;
    /* HR481-490: the same as HR27, 29, 56, 57, 59, 94, 95, 96, 50, 53 but RAM only (no dirty bit). */
    case 481:
        if (reg_value <= 1) {
            cfg_settings.battery_power_mode = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 482:
        if (reg_value <= 7 && cfg_settings.battery_type == 1) {
            if (reg_value > 0) {
                cfg_settings.soc_force_adjust = cfg_settings.soc_force_adjust | (uchar)reg_value;
            } else {
                cfg_settings.soc_force_adjust = reg_value;
            }
        } else {
            write_error = 1;
        }
        break;
    case 483:
        if (reg_value <= 0x960) {
            cfg_settings.discharge_slot_1_start = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 484:
        if (reg_value <= 0x960) {
            cfg_settings.discharge_slot_1_end = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 485:
        if (reg_value <= 1) {
            cfg_settings.enable_discharge = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 486:
        if (reg_value <= 0x960) {
            cfg_settings.charge_slot_1_start = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 487:
        if (reg_value <= 0x960) {
            cfg_settings.charge_slot_1_end = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 488:
        if (reg_value <= 1) {
            cfg_settings.enable_charge = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 489:
        if (reg_value <= 0x64) {
            cfg_settings.active_power_rate = reg_value;
            cfg_settings.hr121 = 0;
        } else {
            write_error = 1;
        }
        break;
    case 490:
        if ((uchar)reg_value == 0) {
            cfg_settings.inverter_state = 0;
        } else {
            if ((uchar)reg_value == 1) {
                cfg_settings.inverter_state = 1;
            } else {
                write_error = 1;
            }
        }
        if ((reg_value & 0xff00) == 0x100) {
            cfg_settings.f09 = 1;
        } else {
            if ((reg_value & 0xff00) == 0) {
                cfg_settings.f09 = 0;
            } else {
                write_error = 1;
            }
        }
        break;
    /* HR494: remote battery power mode, used when exactly 1 (stored in a byte, so 256 reads as 0).
     Not marked for saving. */
    case 494:
        if (reg_value >= 1) {
            cfg_block6.hr494 = reg_value;
        } else {
            cfg_block6.hr494 = 0;
        }
        break;
    case 495:
        if (reg_value < 0x10000) {
            batt_power_remote = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 512:
        if (reg_value <= 0x2710) {
            cfg_settings.battery_capacity = reg_value;
            eeprom_dirty = eeprom_dirty | 1;
        } else {
            write_error = 1;
        }
        break;
    case 515:
        if (reg_value >= 1 && reg_value <= 0x64) {
            cfg_settings.hr314 = reg_value;
        } else {
            write_error = 1;
        }
        break;
    case 4000:
        if (reg_value < 0x10000) {
            hr4000_value = reg_value;
        } else {
            write_error = 1;
        }
        break;
    /* HR4097-4146 gains (0x366..0x499 around 0x400 = 1.0): a gain can only be changed when it is 1.0
     now or is being set to 1.0. */
    case 4097:
        if (reg_value == 0x400 || cal_gains.gain0 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain0 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4098:
        if (reg_value == 0x400 || cal_gains.gain2 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain2 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4099:
        if (reg_value == 0x400 || cal_gains.gain4 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain4 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4101:
        if (reg_value == 0x400 || cal_gains.gain5 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain5 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    /* HR4102/4103, 4107/4108, 4140-4144: set energy counters (high word first, the low word
     saves). */
    case 4102:
        energy_totals.batt_discharge_total = reg_value << 0x10;
        energy_totals.batt_charge_total = reg_value << 0x10;
        break;
    case 4103:
        energy_totals.batt_discharge_total =
            energy_totals.batt_discharge_total & 0xffff0000 | (reg_value & 0xffff) << 0;
        energy_totals.batt_charge_total =
            energy_totals.batt_charge_total & 0xffff0000 | (reg_value & 0xffff) << 0;
        eeprom_dirty = eeprom_dirty | 8;
        break;
    case 4104:
        if (reg_value == 0x400 || cal_gains.gain1 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain1 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4105:
        if (reg_value == 0x400 || cal_gains.gain3 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain3 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4106:
        if (reg_value == 0x400 || cal_gains.gain6 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain6 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4107:
        energy_totals.pv_total = reg_value << 0x10;
        break;
    case 4108:
        energy_totals.pv_total = energy_totals.pv_total & 0xffff0000 | (reg_value & 0xffff) << 0;
        eeprom_dirty = eeprom_dirty | 8;
        break;
    case 4120:
        if (reg_value == 0x400 || cal_gains.gain7 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain7 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4140:
        energy_totals.inv_out_day = reg_value;
        eeprom_dirty = eeprom_dirty | 8;
        break;
    case 4141:
        energy_totals.inv_out_total = reg_value << 0x10;
        break;
    case 4142:
        energy_totals.inv_out_total =
            energy_totals.inv_out_total & 0xffff0000 | (reg_value & 0xffff) << 0;
        eeprom_dirty = eeprom_dirty | 8;
        break;
    case 4143:
        energy_totals.work_time = reg_value << 0x10;
        break;
    case 4144:
        energy_totals.work_time = energy_totals.work_time & 0xffff0000 | (reg_value & 0xffff) << 0;
        eeprom_dirty = eeprom_dirty | 8;
        break;
    case 4146:
        if (reg_value == 0x400 || cal_gains.gain8 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain8 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    /* HR4354-4385 offsets (0..100): an offset can only be changed when it is 0 now or is being set
     to 0. */
    case 4354:
        if (reg_value == 0 || cal_gains.offset0 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset0 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4355:
        if (reg_value == 0 || cal_gains.offset2 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset2 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4356:
        if (reg_value == 0 || cal_gains.offset4 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset4 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4358:
        if (reg_value == 0 || cal_gains.offset5 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset5 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4361:
        if (reg_value == 0 || cal_gains.offset1 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset1 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4362:
        if (reg_value == 0 || cal_gains.offset3 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset3 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4363:
        if (reg_value == 0 || cal_gains.offset6 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset6 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4366:
        if (reg_value == 0 || cal_gains.offset7 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset7 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4378:
        if (reg_value == 0 || cal_gains.offset8 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset8 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4380:
        if (reg_value == 0x400 || cal_gains.gain11 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain11 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4381:
        if (reg_value == 0 || cal_gains.offset11 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset11 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4382:
        if (reg_value == 0x400 || cal_gains.gain9 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain9 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4383:
        if (reg_value == 0 || cal_gains.offset9 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset9 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4384:
        if (reg_value == 0x400 || cal_gains.gain10 == 0x400) {
            if (reg_value >= 0x366 && reg_value <= 0x499) {
                cal_gains.gain10 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    case 4385:
        if (reg_value == 0 || cal_gains.offset10 == 0) {
            if (reg_value <= 0x64) {
                cal_gains.offset10 = reg_value;
                eeprom_dirty = eeprom_dirty | 2;
            } else {
                write_error = 1;
            }
        } else {
            write_error = 1;
        }
        break;
    default:
        write_error = 1;
    }
    if (write_error == 0) {
        crc = crc16_modbus(&modbus_reply[0], 0x10);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
    } else {
        modbus_reply[1] = 0x86;
        crc = crc16_modbus(&modbus_reply[0], 0x10);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
        modbus_err_reply = 1;
        return;
    }
}
#undef AAE
#undef PX
#undef NVIC

typedef __packed struct {
    uchar b0[2];
    uchar id[10];
    uchar pad0[0x20];
    uchar f2c;
    ushort f2d;
    int f2f;
    ushort f33;
    int f35;
    uint f39;
    uint f3d;
    uint f41;
    uchar f45;
    uchar b46[9];
    uchar pad1[4];
    ushort w53[16];
    ushort f73;
    ushort f75;
    ushort f77;
    ushort w79[5];
} dev_t;

/* Fills IR60-119 of slaves 0x32-0x36 from pack record n (bms_modules[n], filled by bms_poll):

     60-75 cells (mV)  76-79 temperatures 1-4  80 pack voltage  81 temperature 5
     82/83 block 2 u32 voltage  84/85 calibrated capacity  86/87 design capacity
     88/89 remaining capacity  90-94 status bytes  95 current / 10 (0.01 A)  96 cycles
     97 cell count  98 BMS firmware  100 SOC  101/102 design capacity again
     103/104 highest/lowest temperature  105/106 an energy_totals word pair
     110-114 the first 10 characters of the serial

   docs/battery.md, section 2, has the source of each field. */
/* match: in this function three kinds of cast stay: the low halves of bms_modules[n].v_out and of
   energy_totals.batt_discharge_total and batt_charge_total are halfword loads at the field's
   address, which (ushort)field would load as a word; and the low bytes of the status pairs go
   through the module seen as a dev_t; as bms_modules[n].status[k] the bytes differ */
void modbus_fill_battery_regs(uchar n) {
    ushort batt_value;
    uint v;

    v = bms_modules[n].v_cell[0];
    modbus_reg_buf[0] = v >> 8;
    modbus_reg_buf[1] = v;
    v = bms_modules[n].v_cell[1];
    modbus_reg_buf[2] = v >> 8;
    modbus_reg_buf[3] = v;
    v = bms_modules[n].v_cell[2];
    modbus_reg_buf[4] = v >> 8;
    modbus_reg_buf[5] = v;
    v = bms_modules[n].v_cell[3];
    modbus_reg_buf[6] = v >> 8;
    modbus_reg_buf[7] = v;
    v = bms_modules[n].v_cell[4];
    modbus_reg_buf[8] = v >> 8;
    modbus_reg_buf[9] = v;
    v = bms_modules[n].v_cell[5];
    modbus_reg_buf[10] = v >> 8;
    modbus_reg_buf[11] = v;
    v = bms_modules[n].v_cell[6];
    modbus_reg_buf[12] = v >> 8;
    modbus_reg_buf[13] = v;
    v = bms_modules[n].v_cell[7];
    modbus_reg_buf[14] = v >> 8;
    modbus_reg_buf[15] = v;
    v = bms_modules[n].v_cell[8];
    modbus_reg_buf[16] = v >> 8;
    modbus_reg_buf[17] = v;
    v = bms_modules[n].v_cell[9];
    modbus_reg_buf[18] = v >> 8;
    modbus_reg_buf[19] = v;
    v = bms_modules[n].v_cell[10];
    modbus_reg_buf[20] = v >> 8;
    modbus_reg_buf[21] = v;
    v = bms_modules[n].v_cell[11];
    modbus_reg_buf[22] = v >> 8;
    modbus_reg_buf[23] = v;
    v = bms_modules[n].v_cell[12];
    modbus_reg_buf[24] = v >> 8;
    modbus_reg_buf[25] = v;
    v = bms_modules[n].v_cell[13];
    modbus_reg_buf[26] = v >> 8;
    modbus_reg_buf[27] = v;
    v = bms_modules[n].v_cell[14];
    modbus_reg_buf[28] = v >> 8;
    modbus_reg_buf[29] = v;
    v = bms_modules[n].v_cell[15];
    modbus_reg_buf[30] = v >> 8;
    modbus_reg_buf[31] = v;
    v = bms_modules[n].temp[0];
    modbus_reg_buf[32] = v >> 8;
    modbus_reg_buf[33] = v;
    v = bms_modules[n].temp[1];
    modbus_reg_buf[34] = v >> 8;
    modbus_reg_buf[35] = v;
    v = bms_modules[n].temp[2];
    modbus_reg_buf[36] = v >> 8;
    modbus_reg_buf[37] = v;
    v = bms_modules[n].temp[3];
    modbus_reg_buf[38] = v >> 8;
    modbus_reg_buf[39] = v;
    v = bms_modules[n].v_cells_sum;
    modbus_reg_buf[40] = v >> 8;
    modbus_reg_buf[41] = v;
    v = bms_modules[n].temp[4];
    modbus_reg_buf[42] = v >> 8;
    modbus_reg_buf[43] = v;
    v = bms_modules[n].v_out >> 16;
    modbus_reg_buf[44] = v >> 8;
    modbus_reg_buf[45] = v;
    v = *(__packed ushort *)&bms_modules[n].v_out;
    modbus_reg_buf[46] = v >> 8;
    modbus_reg_buf[47] = v;
    v = bms_modules[n].cap_calibrated >> 16;
    modbus_reg_buf[48] = v >> 8;
    modbus_reg_buf[49] = v;
    v = (ushort)bms_modules[n].cap_calibrated;
    modbus_reg_buf[50] = v >> 8;
    modbus_reg_buf[51] = v;
    v = bms_modules[n].cap_design >> 16;
    modbus_reg_buf[52] = v >> 8;
    modbus_reg_buf[53] = v;
    v = (ushort)bms_modules[n].cap_design;
    modbus_reg_buf[54] = v >> 8;
    modbus_reg_buf[55] = v;
    v = bms_modules[n].cap_remaining >> 16;
    modbus_reg_buf[56] = v >> 8;
    modbus_reg_buf[57] = v;
    v = (ushort)bms_modules[n].cap_remaining;
    modbus_reg_buf[58] = v >> 8;
    modbus_reg_buf[59] = v;
    v = (0xffff & (bms_modules[n].status[0] << 8)) +
        ((uchar *)&((dev_t *)((uchar *)bms_modules))[n])[0x47];
    modbus_reg_buf[60] = v >> 8;
    modbus_reg_buf[61] = v;
    v = (0xffff & (bms_modules[n].status[2] << 8)) +
        ((uchar *)&((dev_t *)((uchar *)bms_modules))[n])[0x49];
    modbus_reg_buf[62] = v >> 8;
    modbus_reg_buf[63] = v;
    v = (0xffff & (bms_modules[n].status[4] << 8)) +
        ((uchar *)&((dev_t *)((uchar *)bms_modules))[n])[0x4b];
    modbus_reg_buf[64] = v >> 8;
    modbus_reg_buf[65] = v;
    v = bms_modules[n].status[6];
    modbus_reg_buf[66] = v >> 8;
    modbus_reg_buf[67] = v;
    v = (0xffff & (bms_modules[n].status[7] << 8)) +
        ((uchar *)&((dev_t *)((uchar *)bms_modules))[n])[0x4e];
    modbus_reg_buf[68] = v >> 8;
    modbus_reg_buf[69] = v;
    batt_value = bms_modules[n].f35 / 10;
    modbus_reg_buf[70] = batt_value >> 8;
    modbus_reg_buf[71] = batt_value;
    v = bms_modules[n].num_cycles;
    modbus_reg_buf[72] = v >> 8;
    modbus_reg_buf[73] = v;
    v = bms_modules[n].num_cells;
    modbus_reg_buf[74] = v >> 8;
    modbus_reg_buf[75] = v;
    v = bms_modules[n].fw_version;
    modbus_reg_buf[76] = v >> 8;
    modbus_reg_buf[77] = v;
    v = 0;
    modbus_reg_buf[78] = v >> 8;
    modbus_reg_buf[79] = v;
    v = bms_modules[n].soc;
    modbus_reg_buf[80] = v >> 8;
    modbus_reg_buf[81] = v;
    v = bms_modules[n].cap_design >> 16;
    modbus_reg_buf[82] = v >> 8;
    modbus_reg_buf[83] = v;
    v = (ushort)bms_modules[n].cap_design;
    modbus_reg_buf[84] = v >> 8;
    modbus_reg_buf[85] = v;
    v = bms_modules[n].temp_max;
    modbus_reg_buf[86] = v >> 8;
    modbus_reg_buf[87] = v;
    v = bms_modules[n].temp_min;
    modbus_reg_buf[88] = v >> 8;
    modbus_reg_buf[89] = v;
    v = *(ushort *)&energy_totals.batt_discharge_total;
    modbus_reg_buf[90] = v >> 8;
    modbus_reg_buf[91] = v;
    v = *(ushort *)&energy_totals.batt_charge_total;
    modbus_reg_buf[92] = v >> 8;
    modbus_reg_buf[93] = v;
    v = 0;
    modbus_reg_buf[94] = v >> 8;
    modbus_reg_buf[95] = v;
    v = 0;
    modbus_reg_buf[96] = v >> 8;
    modbus_reg_buf[97] = v;
    v = 0;
    modbus_reg_buf[98] = v >> 8;
    modbus_reg_buf[99] = v;
    modbus_reg_buf[100] = bms_modules[n].serial[0];
    modbus_reg_buf[101] = bms_modules[n].serial[1];
    modbus_reg_buf[102] = bms_modules[n].serial[2];
    modbus_reg_buf[103] = bms_modules[n].serial[3];
    modbus_reg_buf[104] = bms_modules[n].serial[4];
    modbus_reg_buf[105] = bms_modules[n].serial[5];
    modbus_reg_buf[106] = bms_modules[n].serial[6];
    modbus_reg_buf[107] = bms_modules[n].serial[7];
    modbus_reg_buf[108] = bms_modules[n].serial[8];
    modbus_reg_buf[109] = bms_modules[n].serial[9];
    v = 0;
    modbus_reg_buf[110] = v >> 8;
    modbus_reg_buf[111] = v;
    v = 0;
    modbus_reg_buf[112] = v >> 8;
    modbus_reg_buf[113] = v;
    v = 0;
    modbus_reg_buf[114] = v >> 8;
    modbus_reg_buf[115] = v;
    v = 0;
    modbus_reg_buf[116] = v >> 8;
    modbus_reg_buf[117] = v;
    v = 0;
    modbus_reg_buf[118] = v >> 8;
    modbus_reg_buf[119] = v;
}

/* Fills IR60-119 of slave 0x31: IR62 the BMS charge limit and IR63 the BMS discharge limit, in A,
   from DSP page 5; the rest 0. */
void modbus_fill_bcu_regs() {
    uint v;

    v = 0;
    modbus_reg_buf[0] = v >> 8;
    modbus_reg_buf[1] = v;
    v = 0;
    modbus_reg_buf[2] = v >> 8;
    modbus_reg_buf[3] = v;
    v = bms_charge_limit_a;
    modbus_reg_buf[4] = v >> 8;
    modbus_reg_buf[5] = v;
    v = bms_discharge_limit_a;
    modbus_reg_buf[6] = v >> 8;
    modbus_reg_buf[7] = v;
    v = 0;
    modbus_reg_buf[8] = v >> 8;
    modbus_reg_buf[9] = v;
    v = 0;
    modbus_reg_buf[10] = v >> 8;
    modbus_reg_buf[11] = v;
    v = 0;
    modbus_reg_buf[12] = v >> 8;
    modbus_reg_buf[13] = v;
    v = 0;
    modbus_reg_buf[14] = v >> 8;
    modbus_reg_buf[15] = v;
    v = 0;
    modbus_reg_buf[16] = v >> 8;
    modbus_reg_buf[17] = v;
    v = 0;
    modbus_reg_buf[18] = v >> 8;
    modbus_reg_buf[19] = v;
    v = 0;
    modbus_reg_buf[20] = v >> 8;
    modbus_reg_buf[21] = v;
    v = 0;
    modbus_reg_buf[22] = v >> 8;
    modbus_reg_buf[23] = v;
    v = 0;
    modbus_reg_buf[24] = v >> 8;
    modbus_reg_buf[25] = v;
    v = 0;
    modbus_reg_buf[26] = v >> 8;
    modbus_reg_buf[27] = v;
    v = 0;
    modbus_reg_buf[28] = v >> 8;
    modbus_reg_buf[29] = v;
    v = 0;
    modbus_reg_buf[30] = v >> 8;
    modbus_reg_buf[31] = v;
    v = 0;
    modbus_reg_buf[32] = v >> 8;
    modbus_reg_buf[33] = v;
    v = 0;
    modbus_reg_buf[34] = v >> 8;
    modbus_reg_buf[35] = v;
    v = 0;
    modbus_reg_buf[36] = v >> 8;
    modbus_reg_buf[37] = v;
    v = 0;
    modbus_reg_buf[38] = v >> 8;
    modbus_reg_buf[39] = v;
    v = 0;
    modbus_reg_buf[40] = v >> 8;
    modbus_reg_buf[41] = v;
    v = 0;
    modbus_reg_buf[42] = v >> 8;
    modbus_reg_buf[43] = v;
    v = 0;
    modbus_reg_buf[44] = v >> 8;
    modbus_reg_buf[45] = v;
    v = 0;
    modbus_reg_buf[46] = v >> 8;
    modbus_reg_buf[47] = v;
    v = 0;
    modbus_reg_buf[48] = v >> 8;
    modbus_reg_buf[49] = v;
    v = 0;
    modbus_reg_buf[50] = v >> 8;
    modbus_reg_buf[51] = v;
    v = 0;
    modbus_reg_buf[52] = v >> 8;
    modbus_reg_buf[53] = v;
    v = 0;
    modbus_reg_buf[54] = v >> 8;
    modbus_reg_buf[55] = v;
    v = 0;
    modbus_reg_buf[56] = v >> 8;
    modbus_reg_buf[57] = v;
    v = 0;
    modbus_reg_buf[58] = v >> 8;
    modbus_reg_buf[59] = v;
    v = 0;
    modbus_reg_buf[60] = v >> 8;
    modbus_reg_buf[61] = v;
    v = 0;
    modbus_reg_buf[62] = v >> 8;
    modbus_reg_buf[63] = v;
    v = 0;
    modbus_reg_buf[64] = v >> 8;
    modbus_reg_buf[65] = v;
    v = 0;
    modbus_reg_buf[66] = v >> 8;
    modbus_reg_buf[67] = v;
    v = 0;
    modbus_reg_buf[68] = v >> 8;
    modbus_reg_buf[69] = v;
    v = 0;
    modbus_reg_buf[70] = v >> 8;
    modbus_reg_buf[71] = v;
    v = 0;
    modbus_reg_buf[72] = v >> 8;
    modbus_reg_buf[73] = v;
    v = 0;
    modbus_reg_buf[74] = v >> 8;
    modbus_reg_buf[75] = v;
    v = 0;
    modbus_reg_buf[76] = v >> 8;
    modbus_reg_buf[77] = v;
    v = 0;
    modbus_reg_buf[78] = 0;
    modbus_reg_buf[79] = 0;
    v = 0;
    modbus_reg_buf[80] = v >> 8;
    modbus_reg_buf[81] = v;
    v = 0;
    modbus_reg_buf[82] = v >> 8;
    modbus_reg_buf[83] = v;
    v = 0;
    modbus_reg_buf[84] = v >> 8;
    modbus_reg_buf[85] = v;
    v = 0;
    modbus_reg_buf[86] = v >> 8;
    modbus_reg_buf[87] = v;
    v = 0;
    modbus_reg_buf[88] = v >> 8;
    modbus_reg_buf[89] = v;
    v = 0;
    modbus_reg_buf[90] = v >> 8;
    modbus_reg_buf[91] = v;
    v = 0;
    modbus_reg_buf[92] = v >> 8;
    modbus_reg_buf[93] = v;
    v = 0;
    modbus_reg_buf[94] = v >> 8;
    modbus_reg_buf[95] = v;
    v = 0;
    modbus_reg_buf[96] = v >> 8;
    modbus_reg_buf[97] = v;
    v = 0;
    modbus_reg_buf[98] = v >> 8;
    modbus_reg_buf[99] = v;
    v = 0;
    modbus_reg_buf[100] = v >> 8;
    modbus_reg_buf[101] = v;
    v = 0;
    modbus_reg_buf[102] = v >> 8;
    modbus_reg_buf[103] = v;
    v = 0;
    modbus_reg_buf[104] = v >> 8;
    modbus_reg_buf[105] = v;
    v = 0;
    modbus_reg_buf[106] = v >> 8;
    modbus_reg_buf[107] = v;
    v = 0;
    modbus_reg_buf[108] = v >> 8;
    modbus_reg_buf[109] = v;
    v = 0;
    modbus_reg_buf[110] = v >> 8;
    modbus_reg_buf[111] = v;
    v = 0;
    modbus_reg_buf[112] = v >> 8;
    modbus_reg_buf[113] = v;
    v = 0;
    modbus_reg_buf[114] = v >> 8;
    modbus_reg_buf[115] = v;
    v = 0;
    modbus_reg_buf[116] = v >> 8;
    modbus_reg_buf[117] = v;
    v = 0;
    modbus_reg_buf[118] = v >> 8;
    modbus_reg_buf[119] = v;
}

/* Fills IR60-119 of slave 0x30: IR99 = packs online (high byte) and 16 (low byte); the rest 0. */
void modbus_fill_slave30_regs() {
    int v;

    v = 0;
    modbus_reg_buf[0] = v >> 8;
    modbus_reg_buf[1] = v;
    v = 0;
    modbus_reg_buf[2] = v >> 8;
    modbus_reg_buf[3] = v;
    v = 0;
    modbus_reg_buf[4] = v >> 8;
    modbus_reg_buf[5] = v;
    modbus_reg_buf[6] = 0;
    modbus_reg_buf[7] = 0;
    v = 0;
    modbus_reg_buf[8] = v >> 8;
    modbus_reg_buf[9] = v;
    v = 0;
    modbus_reg_buf[10] = v >> 8;
    modbus_reg_buf[11] = v;
    v = 0;
    modbus_reg_buf[12] = v >> 8;
    modbus_reg_buf[13] = v;
    v = 0;
    modbus_reg_buf[14] = v >> 8;
    modbus_reg_buf[15] = v;
    v = 0;
    modbus_reg_buf[16] = v >> 8;
    modbus_reg_buf[17] = v;
    v = 0;
    modbus_reg_buf[18] = v >> 8;
    modbus_reg_buf[19] = v;
    v = 0;
    modbus_reg_buf[20] = v >> 8;
    modbus_reg_buf[21] = v;
    v = 0;
    modbus_reg_buf[22] = v >> 8;
    modbus_reg_buf[23] = v;
    v = 0;
    modbus_reg_buf[24] = v >> 8;
    modbus_reg_buf[25] = v;
    v = 0;
    modbus_reg_buf[26] = v >> 8;
    modbus_reg_buf[27] = v;
    v = 0;
    modbus_reg_buf[28] = v >> 8;
    modbus_reg_buf[29] = v;
    v = 0;
    modbus_reg_buf[30] = v >> 8;
    modbus_reg_buf[31] = v;
    v = 0;
    modbus_reg_buf[32] = v >> 8;
    modbus_reg_buf[33] = v;
    v = 0;
    modbus_reg_buf[34] = v >> 8;
    modbus_reg_buf[35] = v;
    v = 0;
    modbus_reg_buf[36] = v >> 8;
    modbus_reg_buf[37] = v;
    v = 0;
    modbus_reg_buf[38] = v >> 8;
    modbus_reg_buf[39] = v;
    v = 0;
    modbus_reg_buf[40] = v >> 8;
    modbus_reg_buf[41] = v;
    v = 0;
    modbus_reg_buf[42] = v >> 8;
    modbus_reg_buf[43] = v;
    v = 0;
    modbus_reg_buf[44] = v >> 8;
    modbus_reg_buf[45] = v;
    v = 0;
    modbus_reg_buf[46] = v >> 8;
    modbus_reg_buf[47] = v;
    v = 0;
    modbus_reg_buf[48] = v >> 8;
    modbus_reg_buf[49] = v;
    v = 0;
    modbus_reg_buf[50] = v >> 8;
    modbus_reg_buf[51] = v;
    v = 0;
    modbus_reg_buf[52] = v >> 8;
    modbus_reg_buf[53] = v;
    v = 0;
    modbus_reg_buf[54] = v >> 8;
    modbus_reg_buf[55] = v;
    v = 0;
    modbus_reg_buf[56] = v >> 8;
    modbus_reg_buf[57] = v;
    v = 0;
    modbus_reg_buf[58] = v >> 8;
    modbus_reg_buf[59] = v;
    v = 0;
    modbus_reg_buf[60] = v >> 8;
    modbus_reg_buf[61] = v;
    v = 0;
    modbus_reg_buf[62] = v >> 8;
    modbus_reg_buf[63] = v;
    v = 0;
    modbus_reg_buf[64] = v >> 8;
    modbus_reg_buf[65] = v;
    v = 0;
    modbus_reg_buf[66] = v >> 8;
    modbus_reg_buf[67] = v;
    v = 0;
    modbus_reg_buf[68] = v >> 8;
    modbus_reg_buf[69] = v;
    v = 0;
    modbus_reg_buf[70] = v >> 8;
    modbus_reg_buf[71] = v;
    v = 0;
    modbus_reg_buf[72] = v >> 8;
    modbus_reg_buf[73] = v;
    v = 0;
    modbus_reg_buf[74] = v >> 8;
    modbus_reg_buf[75] = v;
    v = 0;
    modbus_reg_buf[76] = v >> 8;
    modbus_reg_buf[77] = v;
    v = 0;
    modbus_reg_buf[78] = bms_packs_online[0];
    modbus_reg_buf[79] = 0x10;
    v = 0;
    modbus_reg_buf[80] = v >> 8;
    modbus_reg_buf[81] = v;
    v = 0;
    modbus_reg_buf[82] = v >> 8;
    modbus_reg_buf[83] = v;
    v = 0;
    modbus_reg_buf[84] = v >> 8;
    modbus_reg_buf[85] = v;
    v = 0;
    modbus_reg_buf[86] = v >> 8;
    modbus_reg_buf[87] = v;
    v = 0;
    modbus_reg_buf[88] = v >> 8;
    modbus_reg_buf[89] = v;
    v = 0;
    modbus_reg_buf[90] = v >> 8;
    modbus_reg_buf[91] = v;
    v = 0;
    modbus_reg_buf[92] = v >> 8;
    modbus_reg_buf[93] = v;
    v = 0;
    modbus_reg_buf[94] = v >> 8;
    modbus_reg_buf[95] = v;
    v = 0;
    modbus_reg_buf[96] = v >> 8;
    modbus_reg_buf[97] = v;
    v = 0;
    modbus_reg_buf[98] = v >> 8;
    modbus_reg_buf[99] = v;
    v = 0;
    modbus_reg_buf[100] = v >> 8;
    modbus_reg_buf[101] = v;
    v = 0;
    modbus_reg_buf[102] = v >> 8;
    modbus_reg_buf[103] = v;
    v = 0;
    modbus_reg_buf[104] = v >> 8;
    modbus_reg_buf[105] = v;
    v = 0;
    modbus_reg_buf[106] = v >> 8;
    modbus_reg_buf[107] = v;
    v = 0;
    modbus_reg_buf[108] = v >> 8;
    modbus_reg_buf[109] = v;
    v = 0;
    modbus_reg_buf[110] = v >> 8;
    modbus_reg_buf[111] = v;
    v = 0;
    modbus_reg_buf[112] = v >> 8;
    modbus_reg_buf[113] = v;
    v = 0;
    modbus_reg_buf[114] = v >> 8;
    modbus_reg_buf[115] = v;
    v = 0;
    modbus_reg_buf[116] = v >> 8;
    modbus_reg_buf[117] = v;
    v = 0;
    modbus_reg_buf[118] = v >> 8;
    modbus_reg_buf[119] = v;
}

typedef __packed struct {
    ushort h00;
    ushort h02;
    ushort h04;
    int w06;
    uint u0a;
    uint u0e;
    ushort h12;
    int w14;
    int w18;
    int w1c;
    int w20;
    ushort h24;
    ushort h26;
    ushort h28;
    ushort h2a;
    ushort h2c;
} PACK_080206A4;

/* match: PK is meter_data with q_reactive unsigned (u0a) and pf unsigned (h12); as meter_data[idx]
   the division and the load are signed and the bytes differ */
#define PK ((PACK_080206A4 *)((uchar *)&meter_data))

/* Fills IR60-119 of meter slaves 1-8 from meter_data[idx]: IR60 voltage, IR63 frequency, IR68
   active power / 10, IR72 apparent power / 10, IR76 +0x0A / 10, IR80 PF, IR84 current, IR85-88
   import and export energy; the rest 0. */
void modbus_fill_meter_ir(uchar idx) {
    int v;

    v = meter_data[idx].voltage;
    modbus_reg_buf[0] = v >> 8;
    modbus_reg_buf[1] = v;
    v = 0;
    modbus_reg_buf[2] = v >> 8;
    modbus_reg_buf[3] = v;
    v = 0;
    modbus_reg_buf[4] = v >> 8;
    modbus_reg_buf[5] = v;
    v = meter_data[idx].frequency;
    modbus_reg_buf[6] = v >> 8;
    modbus_reg_buf[7] = v;
    v = 0;
    modbus_reg_buf[8] = v >> 8;
    modbus_reg_buf[9] = v;
    v = 0;
    modbus_reg_buf[10] = v >> 8;
    modbus_reg_buf[11] = v;
    v = 0;
    modbus_reg_buf[12] = v >> 8;
    modbus_reg_buf[13] = v;
    v = 0;
    modbus_reg_buf[14] = v >> 8;
    modbus_reg_buf[15] = v;
    v = meter_data[idx].p_active / 10;
    modbus_reg_buf[16] = v >> 8;
    modbus_reg_buf[17] = v;
    v = 0;
    modbus_reg_buf[18] = v >> 8;
    modbus_reg_buf[19] = v;
    v = 0;
    modbus_reg_buf[20] = v >> 8;
    modbus_reg_buf[21] = v;
    v = 0;
    modbus_reg_buf[22] = v >> 8;
    modbus_reg_buf[23] = v;
    v = (ushort)(meter_data[idx].s_apparent / 10);
    modbus_reg_buf[24] = v >> 8;
    modbus_reg_buf[25] = v;
    v = 0;
    modbus_reg_buf[26] = v >> 8;
    modbus_reg_buf[27] = v;
    v = 0;
    modbus_reg_buf[28] = v >> 8;
    modbus_reg_buf[29] = v;
    v = 0;
    modbus_reg_buf[30] = v >> 8;
    modbus_reg_buf[31] = v;
    v = (ushort)(PK[idx].u0a / 10);
    modbus_reg_buf[32] = v >> 8;
    modbus_reg_buf[33] = v;
    v = 0;
    modbus_reg_buf[34] = v >> 8;
    modbus_reg_buf[35] = v;
    v = 0;
    modbus_reg_buf[36] = v >> 8;
    modbus_reg_buf[37] = v;
    v = 0;
    modbus_reg_buf[38] = v >> 8;
    modbus_reg_buf[39] = v;
    v = PK[idx].h12;
    modbus_reg_buf[40] = v >> 8;
    modbus_reg_buf[41] = v;
    v = 0;
    modbus_reg_buf[42] = v >> 8;
    modbus_reg_buf[43] = v;
    v = 0;
    modbus_reg_buf[44] = v >> 8;
    modbus_reg_buf[45] = v;
    v = 0;
    modbus_reg_buf[46] = v >> 8;
    modbus_reg_buf[47] = v;
    v = meter_data[idx].current;
    modbus_reg_buf[48] = v >> 8;
    modbus_reg_buf[49] = v;
    v = (ushort)meter_data[idx].e_import;
    modbus_reg_buf[50] = v >> 8;
    modbus_reg_buf[51] = v;
    v = 0;
    modbus_reg_buf[52] = v >> 8;
    modbus_reg_buf[53] = v;
    v = (ushort)meter_data[idx].e_export;
    modbus_reg_buf[54] = v >> 8;
    modbus_reg_buf[55] = v;
    v = 0;
    modbus_reg_buf[56] = v >> 8;
    modbus_reg_buf[57] = v;
    v = 0;
    modbus_reg_buf[58] = v >> 8;
    modbus_reg_buf[59] = v;
    v = 0;
    modbus_reg_buf[60] = v >> 8;
    modbus_reg_buf[61] = v;
    v = 0;
    modbus_reg_buf[62] = v >> 8;
    modbus_reg_buf[63] = v;
    v = 0;
    modbus_reg_buf[64] = v >> 8;
    modbus_reg_buf[65] = v;
    v = 0;
    modbus_reg_buf[66] = v >> 8;
    modbus_reg_buf[67] = v;
    v = 0;
    modbus_reg_buf[68] = v >> 8;
    modbus_reg_buf[69] = v;
    v = 0;
    modbus_reg_buf[70] = v >> 8;
    modbus_reg_buf[71] = v;
    v = 0;
    modbus_reg_buf[72] = v >> 8;
    modbus_reg_buf[73] = v;
    v = 0;
    modbus_reg_buf[74] = v >> 8;
    modbus_reg_buf[75] = v;
    v = 0;
    modbus_reg_buf[76] = v >> 8;
    modbus_reg_buf[77] = v;
    v = 0;
    modbus_reg_buf[78] = v >> 8;
    modbus_reg_buf[79] = v;
    v = 0;
    modbus_reg_buf[80] = v >> 8;
    modbus_reg_buf[81] = v;
    v = 0;
    modbus_reg_buf[82] = v >> 8;
    modbus_reg_buf[83] = v;
    v = 0;
    modbus_reg_buf[84] = v >> 8;
    modbus_reg_buf[85] = v;
    v = 0;
    modbus_reg_buf[86] = v >> 8;
    modbus_reg_buf[87] = v;
    v = 0;
    modbus_reg_buf[88] = v >> 8;
    modbus_reg_buf[89] = v;
    v = 0;
    modbus_reg_buf[90] = v >> 8;
    modbus_reg_buf[91] = v;
    v = 0;
    modbus_reg_buf[92] = v >> 8;
    modbus_reg_buf[93] = v;
    v = 0;
    modbus_reg_buf[94] = v >> 8;
    modbus_reg_buf[95] = v;
    v = 0;
    modbus_reg_buf[96] = v >> 8;
    modbus_reg_buf[97] = v;
    v = 0;
    modbus_reg_buf[98] = v >> 8;
    modbus_reg_buf[99] = v;
    v = 0;
    modbus_reg_buf[100] = v >> 8;
    modbus_reg_buf[101] = v;
    v = 0;
    modbus_reg_buf[102] = v >> 8;
    modbus_reg_buf[103] = v;
    v = 0;
    modbus_reg_buf[104] = v >> 8;
    modbus_reg_buf[105] = v;
    v = 0;
    modbus_reg_buf[106] = v >> 8;
    modbus_reg_buf[107] = v;
    v = 0;
    modbus_reg_buf[108] = v >> 8;
    modbus_reg_buf[109] = v;
    v = 0;
    modbus_reg_buf[110] = v >> 8;
    modbus_reg_buf[111] = v;
    v = 0;
    modbus_reg_buf[112] = v >> 8;
    modbus_reg_buf[113] = v;
    v = 0;
    modbus_reg_buf[114] = v >> 8;
    modbus_reg_buf[115] = v;
    v = 0;
    modbus_reg_buf[116] = v >> 8;
    modbus_reg_buf[117] = v;
    v = 0;
    modbus_reg_buf[118] = v >> 8;
    modbus_reg_buf[119] = v;
}
#undef PK

// clang-format off
  /* at: __aeabi_fmul 0x080369A8, __aeabi_f2uiz 0x08036968, __aeabi_f2iz 0x08036930 */
// clang-format on
typedef __packed struct {
    ushort f0;
    ushort f2;
    ushort f4;
    ushort f6;
    ushort f8;
} REC_08020A32;

/* g_200019E4 is a byte array nothing writes (docs/findings.md); REC reads it as 5-word records */
#define REC ((REC_08020A32 *)g_200019E4)

/* Function 4: reads input registers. Count 1..60, no crossing of a 60-register boundary (else
   error 0x84). The block is built in modbus_reg_buf and the asked part copied after the 16-byte
   header:

     0-59     ir_regs
     60-119   by slave (header comment); slave 0x37 and others get zeros
     120-179  13 records of g_200019E4, 4 words each (nothing writes them)
     180-239  IR217-224 DSP fault words bytes 0-15, IR225 auto test state, then DSP page 4 values,
              IR227 = 0x012C
     240-299  IR240 RMS of ADC ch0, IR243 g_20000152, IR246 ch5 current, IR247/248 total PV
              power / 10 (PV1 + PV2 + |meter 2| + |meter 3|), IR258-260 and 265-267 offsets

   For a start of 300 or above no branch sets j, and the uninitialised j is then used as the CRC
   length and position (docs/findings.md). */
void modbus_fc04_read_input(uchar *req) {
    int v;
    ushort i;
    int start;
    ushort j;
    int count;
    ushort crc;
    uchar err = 0;

    start = req[2];
    start = start << 8 | req[3];
    count = req[4];
    count = count << 8 | req[5];
    if (count > 60 || count == 0) {
        err = 1;
    }
    if (start + count > (start / 60 + 1) * 60) {
        err = 1;
    }
    for (i = 0; i < 10; i++) {
        modbus_reply[2 + i] = cfg_serial_and_settings[i];
    }
    for (i = 0; i < 4; i++) {
        modbus_reply[12 + i] = (req + 2)[i];
    }
    if (err == 1) {
        modbus_reply[1] = 0x84;
        crc = crc16_modbus((int)modbus_reply, 16);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
        modbus_err_reply = 1;
        return;
    } else {
        for (i = 0; i < 0x78; i++) {
            modbus_reg_buf[i] = 0;
        }
        if (start < 60) {
            v = ir_regs[0];
            modbus_reg_buf[0] = v >> 8;
            modbus_reg_buf[1] = v;
            v = ir_regs[1];
            modbus_reg_buf[2] = v >> 8;
            modbus_reg_buf[3] = v;
            v = ir_regs[2];
            modbus_reg_buf[4] = v >> 8;
            modbus_reg_buf[5] = v;
            v = ir_regs[3];
            modbus_reg_buf[6] = v >> 8;
            modbus_reg_buf[7] = v;
            v = ir_regs[4];
            modbus_reg_buf[8] = v >> 8;
            modbus_reg_buf[9] = v;
            v = ir_regs[5];
            modbus_reg_buf[10] = v >> 8;
            modbus_reg_buf[11] = v;
            v = ir_regs[6];
            modbus_reg_buf[12] = v >> 8;
            modbus_reg_buf[13] = v;
            v = ir_regs[7];
            modbus_reg_buf[14] = v >> 8;
            modbus_reg_buf[15] = v;
            v = ir_regs[8];
            modbus_reg_buf[16] = v >> 8;
            modbus_reg_buf[17] = v;
            v = ir_regs[9];
            modbus_reg_buf[18] = v >> 8;
            modbus_reg_buf[19] = v;
            v = ir_regs[10];
            modbus_reg_buf[20] = v >> 8;
            modbus_reg_buf[21] = v;
            v = ir_regs[11];
            modbus_reg_buf[22] = v >> 8;
            modbus_reg_buf[23] = v;
            v = ir_regs[12];
            modbus_reg_buf[24] = v >> 8;
            modbus_reg_buf[25] = v;
            v = ir_regs[13];
            modbus_reg_buf[26] = v >> 8;
            modbus_reg_buf[27] = v;
            v = ir_regs[14];
            modbus_reg_buf[28] = v >> 8;
            modbus_reg_buf[29] = v;
            v = ir_regs[15];
            modbus_reg_buf[30] = v >> 8;
            modbus_reg_buf[31] = v;
            v = ir_regs[16];
            modbus_reg_buf[32] = v >> 8;
            modbus_reg_buf[33] = v;
            v = ir_regs[17];
            modbus_reg_buf[34] = v >> 8;
            modbus_reg_buf[35] = v;
            v = ir_regs[18];
            modbus_reg_buf[36] = v >> 8;
            modbus_reg_buf[37] = v;
            v = ir_regs[19];
            modbus_reg_buf[38] = v >> 8;
            modbus_reg_buf[39] = v;
            v = ir_regs[20];
            modbus_reg_buf[40] = v >> 8;
            modbus_reg_buf[41] = v;
            v = ir_regs[21];
            modbus_reg_buf[42] = v >> 8;
            modbus_reg_buf[43] = v;
            v = ir_regs[22];
            modbus_reg_buf[44] = v >> 8;
            modbus_reg_buf[45] = v;
            v = ir_regs[23];
            modbus_reg_buf[46] = v >> 8;
            modbus_reg_buf[47] = v;
            v = ir_regs[24];
            modbus_reg_buf[48] = v >> 8;
            modbus_reg_buf[49] = v;
            v = ir_regs[25];
            modbus_reg_buf[50] = v >> 8;
            modbus_reg_buf[51] = v;
            v = ir_regs[26];
            modbus_reg_buf[52] = v >> 8;
            modbus_reg_buf[53] = v;
            v = ir_regs[27];
            modbus_reg_buf[54] = v >> 8;
            modbus_reg_buf[55] = v;
            v = ir_regs[28];
            modbus_reg_buf[56] = v >> 8;
            modbus_reg_buf[57] = v;
            v = ir_regs[29];
            modbus_reg_buf[58] = v >> 8;
            modbus_reg_buf[59] = v;
            v = ir_regs[30];
            modbus_reg_buf[60] = v >> 8;
            modbus_reg_buf[61] = v;
            v = ir_regs[31];
            modbus_reg_buf[62] = v >> 8;
            modbus_reg_buf[63] = v;
            v = ir_regs[32];
            modbus_reg_buf[64] = v >> 8;
            modbus_reg_buf[65] = v;
            v = ir_regs[33];
            modbus_reg_buf[66] = v >> 8;
            modbus_reg_buf[67] = v;
            v = ir_regs[34];
            modbus_reg_buf[68] = v >> 8;
            modbus_reg_buf[69] = v;
            v = ir_regs[35];
            modbus_reg_buf[70] = v >> 8;
            modbus_reg_buf[71] = v;
            v = ir_regs[36];
            modbus_reg_buf[72] = v >> 8;
            modbus_reg_buf[73] = v;
            v = ir_regs[37];
            modbus_reg_buf[74] = v >> 8;
            modbus_reg_buf[75] = v;
            v = ir_regs[38];
            modbus_reg_buf[76] = v >> 8;
            modbus_reg_buf[77] = v;
            v = ir_regs[39];
            modbus_reg_buf[78] = v >> 8;
            modbus_reg_buf[79] = v;
            v = ir_regs[40];
            modbus_reg_buf[80] = v >> 8;
            modbus_reg_buf[81] = v;
            v = ir_regs[41];
            modbus_reg_buf[82] = v >> 8;
            modbus_reg_buf[83] = v;
            v = ir_regs[42];
            modbus_reg_buf[84] = v >> 8;
            modbus_reg_buf[85] = v;
            v = ir_regs[43];
            modbus_reg_buf[86] = v >> 8;
            modbus_reg_buf[87] = v;
            v = ir_regs[44];
            modbus_reg_buf[88] = v >> 8;
            modbus_reg_buf[89] = v;
            v = ir_regs[45];
            modbus_reg_buf[90] = v >> 8;
            modbus_reg_buf[91] = v;
            v = ir_regs[46];
            modbus_reg_buf[92] = v >> 8;
            modbus_reg_buf[93] = v;
            v = ir_regs[47];
            modbus_reg_buf[94] = v >> 8;
            modbus_reg_buf[95] = v;
            v = ir_regs[48];
            modbus_reg_buf[96] = v >> 8;
            modbus_reg_buf[97] = v;
            v = ir_regs[49];
            modbus_reg_buf[98] = v >> 8;
            modbus_reg_buf[99] = v;
            v = ir_regs[50];
            modbus_reg_buf[100] = v >> 8;
            modbus_reg_buf[101] = v;
            v = ir_regs[51];
            modbus_reg_buf[102] = v >> 8;
            modbus_reg_buf[103] = v;
            v = ir_regs[52];
            modbus_reg_buf[104] = v >> 8;
            modbus_reg_buf[105] = v;
            v = ir_regs[53];
            modbus_reg_buf[106] = v >> 8;
            modbus_reg_buf[107] = v;
            v = ir_regs[54];
            modbus_reg_buf[108] = v >> 8;
            modbus_reg_buf[109] = v;
            v = ir_regs[55];
            modbus_reg_buf[110] = v >> 8;
            modbus_reg_buf[111] = v;
            v = ir_regs[56];
            modbus_reg_buf[112] = v >> 8;
            modbus_reg_buf[113] = v;
            v = ir_regs[57];
            modbus_reg_buf[114] = v >> 8;
            modbus_reg_buf[115] = v;
            v = ir_regs[58];
            modbus_reg_buf[116] = v >> 8;
            modbus_reg_buf[117] = v;
            v = ir_regs[59];
            modbus_reg_buf[118] = v >> 8;
            modbus_reg_buf[119] = v;
            i = start * 2;
            j = 16;
            while (i < start * 2 + count * 2) {
                modbus_reply[j] = modbus_reg_buf[i];
                i++;
                j++;
            }
        } else if (start < 120) {
            modbus_ir_slave = req[0];
            if (modbus_ir_slave >= 1 && modbus_ir_slave <= 8) {
                modbus_meter_index = modbus_ir_slave - 1;
                modbus_fill_meter_ir(modbus_meter_index);
            } else if (modbus_ir_slave == 0x30) {
                modbus_fill_slave30_regs();
            } else if (modbus_ir_slave == 0x31) {
                modbus_fill_bcu_regs();
            } else if (modbus_ir_slave < 0x37 && modbus_ir_slave >= 0x32) {
                modbus_batt_index = modbus_ir_slave - 0x32;
                if ((modbus_batt_index > 0 || modbus_batt_index == 0) && modbus_batt_index < 5) {
                    modbus_fill_battery_regs(modbus_batt_index);
                }
            }
            i = start * 2 - 120;
            j = 16;
            while (i < start * 2 + count * 2 - 120) {
                modbus_reply[j] = modbus_reg_buf[i];
                i++;
                j++;
            }
        } else if (start < 180) {
            for (i = 0; i < 13; i++) {
                modbus_reg_buf[0 + i * 8] = REC[i].f2 >> 8;
                modbus_reg_buf[1 + i * 8] = REC[i].f2;
                modbus_reg_buf[2 + i * 8] = REC[i].f4 >> 8;
                modbus_reg_buf[3 + i * 8] = REC[i].f4;
                modbus_reg_buf[4 + i * 8] = REC[i].f6 >> 8;
                modbus_reg_buf[5 + i * 8] = REC[i].f6;
                modbus_reg_buf[6 + i * 8] = REC[i].f8 >> 8;
                modbus_reg_buf[7 + i * 8] = REC[i].f8;
            }
            i = start * 2 - 240;
            j = 16;
            while (i < start * 2 + count * 2 - 240) {
                modbus_reply[j] = modbus_reg_buf[i];
                i++;
                j++;
            }
        } else if (start < 240) {
            modbus_reg_buf[74] = dsp_fault_words[0];
            modbus_reg_buf[75] = dsp_fault_words[1];
            modbus_reg_buf[76] = dsp_fault_words[2];
            modbus_reg_buf[77] = dsp_fault_words[3];
            modbus_reg_buf[78] = dsp_fault_words[4];
            modbus_reg_buf[79] = dsp_fault_words[5];
            modbus_reg_buf[80] = dsp_fault_words[6];
            modbus_reg_buf[81] = dsp_fault_words[7];
            modbus_reg_buf[82] = dsp_fault_words[8];
            modbus_reg_buf[83] = dsp_fault_words[9];
            modbus_reg_buf[84] = dsp_fault_words[10];
            modbus_reg_buf[85] = dsp_fault_words[11];
            modbus_reg_buf[86] = dsp_fault_words[12];
            modbus_reg_buf[87] = dsp_fault_words[13];
            modbus_reg_buf[88] = dsp_fault_words[14];
            modbus_reg_buf[89] = dsp_fault_words[15];
            modbus_reg_buf[90] = auto_test_state;
            modbus_reg_buf[91] = dsp_p4_b02[0];
            modbus_reg_buf[92] = dsp_p4_w03 >> 8;
            modbus_reg_buf[93] = dsp_p4_w03;
            modbus_reg_buf[94] = 1;
            modbus_reg_buf[95] = 0x2c;
            modbus_reg_buf[96] = 0;
            modbus_reg_buf[97] = dsp_p4_b09[0];
            modbus_reg_buf[98] = dsp_p4_w10 / 256;
            modbus_reg_buf[99] = dsp_p4_w10 % 256;
            modbus_reg_buf[100] = dsp_p4_w14 / 256;
            modbus_reg_buf[101] = dsp_p4_w14 % 256;
            modbus_reg_buf[102] = g_2000025C / 256;
            modbus_reg_buf[103] = g_2000025C % 256;
            modbus_reg_buf[104] = dsp_p4_w05 / 256;
            modbus_reg_buf[105] = dsp_p4_w05 % 256;
            modbus_reg_buf[106] = dsp_p4_w12 / 256;
            modbus_reg_buf[107] = dsp_p4_w12 % 256;
            modbus_reg_buf[108] = dsp_p4_w07 / 256;
            modbus_reg_buf[109] = dsp_p4_w07 % 256;
            i = start * 2 - 360;
            j = 16;
            while (i < start * 2 + count * 2 - 360) {
                modbus_reply[j] = modbus_reg_buf[i];
                i++;
                j++;
            }
        } else if (start < 300) {
            v = adc_rms_ch0;
            modbus_reg_buf[0] = v >> 8;
            modbus_reg_buf[1] = v;
            v = 0;
            modbus_reg_buf[2] = v >> 8;
            modbus_reg_buf[3] = v;
            v = 0;
            modbus_reg_buf[4] = v >> 8;
            modbus_reg_buf[5] = v;
            v = g_20000152;
            modbus_reg_buf[6] = v >> 8;
            modbus_reg_buf[7] = v;
            v = 0;
            modbus_reg_buf[8] = v >> 8;
            modbus_reg_buf[9] = v;
            v = 0;
            modbus_reg_buf[10] = v >> 8;
            modbus_reg_buf[11] = v;
            v = i_rms_ch5;
            modbus_reg_buf[12] = v >> 8;
            modbus_reg_buf[13] = v;
            p_pv_total_calc =
                pv1_power + pv2_power + abs(meter_data[1].p_active) + abs(meter_data[2].p_active);
            v = p_pv_total_calc / 10 >> 16;
            modbus_reg_buf[14] = v >> 8;
            modbus_reg_buf[15] = v;
            v = (ushort)(p_pv_total_calc / 10);
            modbus_reg_buf[16] = v >> 8;
            modbus_reg_buf[17] = v;
            v = cal_gains.offset0;
            modbus_reg_buf[36] = v >> 8;
            modbus_reg_buf[37] = v;
            v = cal_gains.offset2;
            modbus_reg_buf[38] = v >> 8;
            modbus_reg_buf[39] = v;
            v = cal_gains.offset4;
            modbus_reg_buf[40] = v >> 8;
            modbus_reg_buf[41] = v;
            v = 0;
            modbus_reg_buf[42] = v >> 8;
            modbus_reg_buf[43] = v;
            v = 0;
            modbus_reg_buf[44] = v >> 8;
            modbus_reg_buf[45] = v;
            v = 0;
            modbus_reg_buf[46] = v >> 8;
            modbus_reg_buf[47] = v;
            v = 0;
            modbus_reg_buf[48] = v >> 8;
            modbus_reg_buf[49] = v;
            v = cal_gains.offset1;
            modbus_reg_buf[50] = v >> 8;
            modbus_reg_buf[51] = v;
            v = cal_gains.offset3;
            modbus_reg_buf[52] = v >> 8;
            modbus_reg_buf[53] = v;
            v = cal_gains.offset6;
            modbus_reg_buf[54] = v >> 8;
            modbus_reg_buf[55] = v;
            v = 0;
            modbus_reg_buf[56] = v >> 8;
            modbus_reg_buf[57] = v;
            v = 0;
            modbus_reg_buf[58] = v >> 8;
            modbus_reg_buf[59] = v;
            v = 0;
            modbus_reg_buf[60] = v >> 8;
            modbus_reg_buf[61] = v;
            v = 0;
            modbus_reg_buf[62] = v >> 8;
            modbus_reg_buf[63] = v;
            v = 0;
            modbus_reg_buf[64] = v >> 8;
            modbus_reg_buf[65] = v;
            v = 0;
            modbus_reg_buf[66] = v >> 8;
            modbus_reg_buf[67] = v;
            v = 0;
            modbus_reg_buf[68] = v >> 8;
            modbus_reg_buf[69] = v;
            v = 0;
            modbus_reg_buf[70] = v >> 8;
            modbus_reg_buf[71] = v;
            v = 0;
            modbus_reg_buf[72] = v >> 8;
            modbus_reg_buf[73] = v;
            v = 0;
            modbus_reg_buf[74] = v >> 8;
            modbus_reg_buf[75] = v;
            i = start * 2 - 480;
            j = 16;
            while (i < start * 2 + count * 2 - 480) {
                modbus_reply[j] = modbus_reg_buf[i];
                i++;
                j++;
            }
        }
        crc = crc16_modbus((int)modbus_reply, j);
        modbus_reply[j] = crc;
        modbus_reply[j + 1] = crc >> 8;
    }
}
#undef REC

/* match: the dev11_data and cal_gains offset reads here load a word at the field and keep its low
   half, (ushort)(*(int *)&x.f); as (ushort)x.f the load is a halfword and the bytes differ */
/* Function 3: reads holding registers. Count 1..60, no crossing of a 60-register boundary (else
   error 0x83). Ranges:

     0-59      hr_regs
     60-119    block 1 grid limits and battery settings; HR84-86 g_20000060
     120-179   block 1 power-quality settings, HR127 auto test, HR175 calibration request
     180-239   block 8 (HR180-186), the 0x11 device's data (HR202-231)
     240-299   block 6 slot times (the SOCs are not in this read)
     300-359   block 1 and block 6 (HR300-332, 351-353), HR329 forcing
     360-419   sched_table_b, 420-479 cfg_hr360_table
     480-539   a remote-control view: run mode, HR27, 29, slot 1 times and enables, HR50, HR53,
               SOC, battery power, load / 10, HR494, HR495, inverter power / 10, BMS info,
               warning word (| 1 while board_init_done), bus voltages, charge status, mode,
               battery voltage, capacity, the SOC targets now, HR314, model, HR311
     4080-4199, 4340-4399   calibration gains and offsets

   For any other start (540-4079, 4200-4339, 4400 and up) no branch sets reply_idx, and the
   uninitialised reply_idx is then used as the CRC length and position (docs/findings.md). */
void modbus_fc03_read_holding(uchar *req) {
    int reg_value;
    int i;
    int start_reg;
    ushort reply_idx;
    int reg_count;
    ushort crc;
    int read_error;

    read_error = 0;
    start_reg = req[2];
    start_reg = req[3] | start_reg << 8;
    reg_count = req[4];
    reg_count = req[5] | reg_count << 8;
    if (reg_count > 0x3c || reg_count == 0) {
        read_error = 1;
    }
    if (start_reg + reg_count > (start_reg / 0x3c + 1 << 4) - (start_reg / 0x3c + 1) << 2) {
        read_error = 1;
    }
    for (i = 0; i < 0xa; i = (ushort)(i + 1)) {
        modbus_reply[2 + i] = cfg_serial_and_settings[i];
    }
    for (i = 0; i < 4; i = (ushort)(i + 1)) {
        modbus_reply[0xc + i] = *(req + 2 + i);
    }
    if (read_error == 1) {
        modbus_reply[1] = 0x83;
        crc = crc16_modbus(&modbus_reply[0], 0x10);
        modbus_reply[16] = crc;
        modbus_reply[17] = crc >> 8;
        modbus_err_reply = 1;
        return;
    }
    for (i = 0; i < 0x78; i = (ushort)(i + 1)) {
        modbus_reg_buf[i] = 0;
    }
    if (start_reg < 0x3c) {
        reg_value = hr_regs[0];
        modbus_reg_buf[0] = (uint)reg_value >> 8;
        modbus_reg_buf[1] = reg_value;
        reg_value = hr_regs[1];
        modbus_reg_buf[2] = (uint)reg_value >> 8;
        modbus_reg_buf[3] = reg_value;
        reg_value = hr_regs[2];
        modbus_reg_buf[4] = (uint)reg_value >> 8;
        modbus_reg_buf[5] = reg_value;
        reg_value = hr_regs[3];
        modbus_reg_buf[6] = (uint)reg_value >> 8;
        modbus_reg_buf[7] = reg_value;
        reg_value = hr_regs[4];
        modbus_reg_buf[8] = (uint)reg_value >> 8;
        modbus_reg_buf[9] = reg_value;
        reg_value = hr_regs[5];
        modbus_reg_buf[10] = (uint)reg_value >> 8;
        modbus_reg_buf[11] = reg_value;
        reg_value = hr_regs[6];
        modbus_reg_buf[12] = (uint)reg_value >> 8;
        modbus_reg_buf[13] = reg_value;
        reg_value = hr_regs[7];
        modbus_reg_buf[14] = (uint)reg_value >> 8;
        modbus_reg_buf[15] = reg_value;
        reg_value = hr_regs[8];
        modbus_reg_buf[16] = (uint)reg_value >> 8;
        modbus_reg_buf[17] = reg_value;
        reg_value = hr_regs[9];
        modbus_reg_buf[18] = (uint)reg_value >> 8;
        modbus_reg_buf[19] = reg_value;
        reg_value = hr_regs[10];
        modbus_reg_buf[20] = (uint)reg_value >> 8;
        modbus_reg_buf[21] = reg_value;
        reg_value = hr_regs[11];
        modbus_reg_buf[22] = (uint)reg_value >> 8;
        modbus_reg_buf[23] = reg_value;
        reg_value = hr_regs[12];
        modbus_reg_buf[24] = (uint)reg_value >> 8;
        modbus_reg_buf[25] = reg_value;
        reg_value = hr_regs[13];
        modbus_reg_buf[26] = (uint)reg_value >> 8;
        modbus_reg_buf[27] = reg_value;
        reg_value = hr_regs[14];
        modbus_reg_buf[28] = (uint)reg_value >> 8;
        modbus_reg_buf[29] = reg_value;
        reg_value = hr_regs[15];
        modbus_reg_buf[30] = (uint)reg_value >> 8;
        modbus_reg_buf[31] = reg_value;
        reg_value = hr_regs[16];
        modbus_reg_buf[32] = (uint)reg_value >> 8;
        modbus_reg_buf[33] = reg_value;
        reg_value = hr_regs[17];
        modbus_reg_buf[34] = (uint)reg_value >> 8;
        modbus_reg_buf[35] = reg_value;
        reg_value = hr_regs[18];
        modbus_reg_buf[36] = (uint)reg_value >> 8;
        modbus_reg_buf[37] = reg_value;
        reg_value = hr_regs[19];
        modbus_reg_buf[38] = (uint)reg_value >> 8;
        modbus_reg_buf[39] = reg_value;
        reg_value = hr_regs[20];
        modbus_reg_buf[40] = (uint)reg_value >> 8;
        modbus_reg_buf[41] = reg_value;
        reg_value = hr_regs[21];
        modbus_reg_buf[42] = (uint)reg_value >> 8;
        modbus_reg_buf[43] = reg_value;
        reg_value = hr_regs[22];
        modbus_reg_buf[44] = (uint)reg_value >> 8;
        modbus_reg_buf[45] = reg_value;
        reg_value = hr_regs[23];
        modbus_reg_buf[46] = (uint)reg_value >> 8;
        modbus_reg_buf[47] = reg_value;
        reg_value = hr_regs[24];
        modbus_reg_buf[48] = (uint)reg_value >> 8;
        modbus_reg_buf[49] = reg_value;
        reg_value = hr_regs[25];
        modbus_reg_buf[50] = (uint)reg_value >> 8;
        modbus_reg_buf[51] = reg_value;
        reg_value = hr_regs[26];
        modbus_reg_buf[52] = (uint)reg_value >> 8;
        modbus_reg_buf[53] = reg_value;
        reg_value = hr_regs[27];
        modbus_reg_buf[54] = (uint)reg_value >> 8;
        modbus_reg_buf[55] = reg_value;
        reg_value = hr_regs[28];
        modbus_reg_buf[56] = (uint)reg_value >> 8;
        modbus_reg_buf[57] = reg_value;
        reg_value = hr_regs[29];
        modbus_reg_buf[58] = (uint)reg_value >> 8;
        modbus_reg_buf[59] = reg_value;
        reg_value = hr_regs[30];
        modbus_reg_buf[60] = (uint)reg_value >> 8;
        modbus_reg_buf[61] = reg_value;
        reg_value = hr_regs[31];
        modbus_reg_buf[62] = (uint)reg_value >> 8;
        modbus_reg_buf[63] = reg_value;
        reg_value = hr_regs[32];
        modbus_reg_buf[64] = (uint)reg_value >> 8;
        modbus_reg_buf[65] = reg_value;
        reg_value = hr_regs[33];
        modbus_reg_buf[66] = (uint)reg_value >> 8;
        modbus_reg_buf[67] = reg_value;
        reg_value = hr_regs[34];
        modbus_reg_buf[68] = (uint)reg_value >> 8;
        modbus_reg_buf[69] = reg_value;
        reg_value = hr_regs[35];
        modbus_reg_buf[70] = (uint)reg_value >> 8;
        modbus_reg_buf[71] = reg_value;
        reg_value = hr_regs[36];
        modbus_reg_buf[72] = (uint)reg_value >> 8;
        modbus_reg_buf[73] = reg_value;
        reg_value = hr_regs[37];
        modbus_reg_buf[74] = (uint)reg_value >> 8;
        modbus_reg_buf[75] = reg_value;
        reg_value = hr_regs[38];
        modbus_reg_buf[76] = (uint)reg_value >> 8;
        modbus_reg_buf[77] = reg_value;
        reg_value = hr_regs[39];
        modbus_reg_buf[78] = (uint)reg_value >> 8;
        modbus_reg_buf[79] = reg_value;
        reg_value = hr_regs[40];
        modbus_reg_buf[80] = (uint)reg_value >> 8;
        modbus_reg_buf[81] = reg_value;
        reg_value = hr_regs[41];
        modbus_reg_buf[82] = (uint)reg_value >> 8;
        modbus_reg_buf[83] = reg_value;
        reg_value = hr_regs[42];
        modbus_reg_buf[84] = (uint)reg_value >> 8;
        modbus_reg_buf[85] = reg_value;
        reg_value = hr_regs[43];
        modbus_reg_buf[86] = (uint)reg_value >> 8;
        modbus_reg_buf[87] = reg_value;
        reg_value = hr_regs[44];
        modbus_reg_buf[88] = (uint)reg_value >> 8;
        modbus_reg_buf[89] = reg_value;
        reg_value = hr_regs[45];
        modbus_reg_buf[90] = (uint)reg_value >> 8;
        modbus_reg_buf[91] = reg_value;
        reg_value = hr_regs[46];
        modbus_reg_buf[92] = (uint)reg_value >> 8;
        modbus_reg_buf[93] = reg_value;
        reg_value = hr_regs[47];
        modbus_reg_buf[94] = (uint)reg_value >> 8;
        modbus_reg_buf[95] = reg_value;
        reg_value = hr_regs[48];
        modbus_reg_buf[96] = (uint)reg_value >> 8;
        modbus_reg_buf[97] = reg_value;
        reg_value = hr_regs[49];
        modbus_reg_buf[98] = (uint)reg_value >> 8;
        modbus_reg_buf[99] = reg_value;
        reg_value = hr_regs[50];
        modbus_reg_buf[100] = (uint)reg_value >> 8;
        modbus_reg_buf[101] = reg_value;
        reg_value = hr_regs[51];
        modbus_reg_buf[102] = (uint)reg_value >> 8;
        modbus_reg_buf[103] = reg_value;
        reg_value = hr_regs[52];
        modbus_reg_buf[104] = (uint)reg_value >> 8;
        modbus_reg_buf[105] = reg_value;
        reg_value = hr_regs[53];
        modbus_reg_buf[106] = (uint)reg_value >> 8;
        modbus_reg_buf[107] = reg_value;
        reg_value = hr_regs[54];
        modbus_reg_buf[108] = (uint)reg_value >> 8;
        modbus_reg_buf[109] = reg_value;
        reg_value = hr_regs[55];
        modbus_reg_buf[110] = (uint)reg_value >> 8;
        modbus_reg_buf[111] = reg_value;
        reg_value = hr_regs[56];
        modbus_reg_buf[112] = (uint)reg_value >> 8;
        modbus_reg_buf[113] = reg_value;
        reg_value = hr_regs[57];
        modbus_reg_buf[114] = (uint)reg_value >> 8;
        modbus_reg_buf[115] = reg_value;
        reg_value = hr_regs[58];
        modbus_reg_buf[116] = (uint)reg_value >> 8;
        modbus_reg_buf[117] = reg_value;
        reg_value = hr_regs[59];
        modbus_reg_buf[118] = (uint)reg_value >> 8;
        modbus_reg_buf[119] = reg_value;
        i = 0xffff & start_reg << 1;
        reply_idx = 0x10;
        while ((start_reg << 1) + (reg_count << 1) > i) {
            modbus_reply[reply_idx] = modbus_reg_buf[i];
            i = (ushort)(i + 1);
            reply_idx++;
        }
    L_23:;
    } else {
        if (start_reg < 0x78) {
            modbus_reg_buf[0] = cfg_settings.v_pv_input_start >> 8;
            modbus_reg_buf[1] = (uchar)cfg_settings.v_pv_input_start;
            modbus_reg_buf[2] = cfg_settings.inverter_start_time >> 8;
            modbus_reg_buf[3] = (uchar)cfg_settings.inverter_start_time;
            reg_value = cfg_settings.inverter_restart_delay;
            modbus_reg_buf[4] = (uint)reg_value >> 8;
            modbus_reg_buf[5] = reg_value;
            modbus_reg_buf[6] = cfg_settings.v_ac_low_out >> 8;
            modbus_reg_buf[7] = (uchar)cfg_settings.v_ac_low_out;
            modbus_reg_buf[8] = cfg_settings.v_ac_high_out >> 8;
            modbus_reg_buf[9] = (uchar)cfg_settings.v_ac_high_out;
            modbus_reg_buf[10] = cfg_settings.f_ac_low_out >> 8;
            modbus_reg_buf[11] = (uchar)cfg_settings.f_ac_low_out;
            modbus_reg_buf[12] = cfg_settings.f_ac_high_out >> 8;
            modbus_reg_buf[13] = (uchar)cfg_settings.f_ac_high_out;
            reg_value = cfg_settings.v_ac_low_out_time;
            modbus_reg_buf[14] = (uint)reg_value >> 8;
            modbus_reg_buf[15] = reg_value;
            reg_value = cfg_settings.v_ac_high_out_time;
            modbus_reg_buf[16] = (uint)reg_value >> 8;
            modbus_reg_buf[17] = reg_value;
            reg_value = cfg_settings.f_ac_low_out_time;
            modbus_reg_buf[18] = (uint)reg_value >> 8;
            modbus_reg_buf[19] = reg_value;
            reg_value = cfg_settings.f_ac_high_out_time;
            modbus_reg_buf[20] = (uint)reg_value >> 8;
            modbus_reg_buf[21] = reg_value;
            reg_value = cfg_settings.v_ac_low_in;
            modbus_reg_buf[22] = (uint)reg_value >> 8;
            modbus_reg_buf[23] = reg_value;
            reg_value = cfg_settings.v_ac_high_in;
            modbus_reg_buf[24] = (uint)reg_value >> 8;
            modbus_reg_buf[25] = reg_value;
            reg_value = cfg_settings.f_ac_low_in;
            modbus_reg_buf[26] = (uint)reg_value >> 8;
            modbus_reg_buf[27] = reg_value;
            reg_value = cfg_settings.f_ac_high_in;
            modbus_reg_buf[28] = (uint)reg_value >> 8;
            modbus_reg_buf[29] = reg_value;
            reg_value = cfg_settings.v_ac_low_in_time;
            modbus_reg_buf[30] = (uint)reg_value >> 8;
            modbus_reg_buf[31] = reg_value;
            reg_value = cfg_settings.v_ac_high_in_time;
            modbus_reg_buf[32] = (uint)reg_value >> 8;
            modbus_reg_buf[33] = reg_value;
            reg_value = cfg_settings.f_ac_low_in_time;
            modbus_reg_buf[34] = (uint)reg_value >> 8;
            modbus_reg_buf[35] = reg_value;
            reg_value = cfg_settings.f_ac_high_in_time;
            modbus_reg_buf[36] = (uint)reg_value >> 8;
            modbus_reg_buf[37] = reg_value;
            reg_value = cfg_settings.v_ac_low_c;
            modbus_reg_buf[38] = (uint)reg_value >> 8;
            modbus_reg_buf[39] = reg_value;
            reg_value = cfg_settings.v_ac_high_c;
            modbus_reg_buf[40] = (uint)reg_value >> 8;
            modbus_reg_buf[41] = reg_value;
            reg_value = cfg_settings.f_ac_low_c;
            modbus_reg_buf[42] = (uint)reg_value >> 8;
            modbus_reg_buf[43] = reg_value;
            reg_value = cfg_settings.f_ac_high_c;
            modbus_reg_buf[44] = (uint)reg_value >> 8;
            modbus_reg_buf[45] = reg_value;
            reg_value = cfg_settings.v_10_min_protection;
            modbus_reg_buf[46] = (uint)reg_value >> 8;
            modbus_reg_buf[47] = reg_value;
            reg_value = g_20000060[1] | g_20000060[0] << 8;
            modbus_reg_buf[48] = (uint)reg_value >> 8;
            modbus_reg_buf[49] = reg_value;
            reg_value = g_20000060[3] | g_20000060[2] << 8;
            modbus_reg_buf[50] = (uint)reg_value >> 8;
            modbus_reg_buf[51] = reg_value;
            reg_value = g_20000060[5] | g_20000060[4] << 8;
            modbus_reg_buf[52] = (uint)reg_value >> 8;
            modbus_reg_buf[53] = reg_value;
            reg_value = cfg_settings.charge_slot_1_start;
            modbus_reg_buf[68] = (uint)reg_value >> 8;
            modbus_reg_buf[69] = reg_value;
            reg_value = cfg_settings.charge_slot_1_end;
            modbus_reg_buf[70] = (uint)reg_value >> 8;
            modbus_reg_buf[71] = reg_value;
            reg_value = cfg_settings.enable_charge;
            modbus_reg_buf[72] = 0;
            modbus_reg_buf[73] = reg_value;
            reg_value = cfg_settings.battery_low_voltage_limit;
            modbus_reg_buf[74] = (uint)reg_value >> 8;
            modbus_reg_buf[75] = reg_value;
            reg_value = cfg_settings.battery_high_voltage_limit;
            modbus_reg_buf[76] = (uint)reg_value >> 8;
            modbus_reg_buf[77] = reg_value;
            /* "+ 0x100 - 0x100" hides from armcc that the value fits a byte; with it the store below
         uses r1 directly (no mov r0, r1), as the image does */
            reg_value = cfg_settings.hr101 + 0x100 - 0x100;
            modbus_reg_buf[82] = (uint)reg_value >> 8;
            modbus_reg_buf[83] = reg_value;
            reg_value = (uint)(uchar)cfg_settings.hr102 >> 6 & 1;
            modbus_reg_buf[84] = (uint)reg_value >> 8;
            modbus_reg_buf[85] = reg_value;
            reg_value = cfg_settings.hr108;
            modbus_reg_buf[96] = (uint)reg_value >> 8;
            modbus_reg_buf[97] = reg_value;
            reg_value = cfg_settings.hr109;
            modbus_reg_buf[98] = (uint)reg_value >> 8;
            modbus_reg_buf[99] = reg_value;
            reg_value = cfg_settings.battery_soc_reserve;
            modbus_reg_buf[100] = (uint)reg_value >> 8;
            modbus_reg_buf[101] = reg_value;
            reg_value = cfg_settings.battery_charge_limit;
            modbus_reg_buf[102] = (uint)reg_value >> 8;
            modbus_reg_buf[103] = reg_value;
            reg_value = cfg_settings.battery_discharge_limit;
            modbus_reg_buf[104] = (uint)reg_value >> 8;
            modbus_reg_buf[105] = reg_value;
            reg_value = cfg_settings.hr113;
            modbus_reg_buf[106] = (uint)reg_value >> 8;
            modbus_reg_buf[107] = reg_value;
            reg_value = cfg_settings.battery_discharge_min_power_reserve;
            modbus_reg_buf[108] = (uint)reg_value >> 8;
            modbus_reg_buf[109] = reg_value;
            reg_value = cfg_settings.hr115;
            modbus_reg_buf[110] = (uint)reg_value >> 8;
            modbus_reg_buf[111] = reg_value;
            reg_value = cfg_settings.charge_target_soc;
            modbus_reg_buf[112] = (uint)reg_value >> 8;
            modbus_reg_buf[113] = reg_value;
            i = (ushort)((start_reg << 1) - 0x78);
            reply_idx = 0x10;
            while ((start_reg << 1) + (reg_count << 1) - 0x78 > i) {
                modbus_reply[reply_idx] = modbus_reg_buf[i];
                i = (ushort)(i + 1);
                reply_idx++;
            }
            goto L_23;
        }
        if (start_reg < 0xb4) {
            reg_value = cfg_settings.hr121;
            modbus_reg_buf[2] = (uint)reg_value >> 8;
            modbus_reg_buf[3] = reg_value;
            reg_value = cfg_settings.hr122;
            modbus_reg_buf[4] = (uint)reg_value >> 8;
            modbus_reg_buf[5] = reg_value;
            reg_value = cfg_settings.hr123;
            modbus_reg_buf[6] = (uint)reg_value >> 8;
            modbus_reg_buf[7] = reg_value;
            reg_value = cfg_block6.hr124;
            modbus_reg_buf[8] = (uint)reg_value >> 8;
            modbus_reg_buf[9] = reg_value;
            reg_value = cfg_settings.hr125;
            modbus_reg_buf[10] = (uint)reg_value >> 8;
            modbus_reg_buf[11] = reg_value;
            reg_value = 0;
            modbus_reg_buf[12] = (uint)reg_value >> 8;
            modbus_reg_buf[13] = reg_value;
            reg_value = auto_test_state;
            modbus_reg_buf[14] = (uint)reg_value >> 8;
            modbus_reg_buf[15] = reg_value;
            reg_value = cfg_settings.hr128;
            modbus_reg_buf[16] = (uint)reg_value >> 8;
            modbus_reg_buf[17] = reg_value;
            modbus_reg_buf[18] = cfg_settings.hr129 >> 8;
            modbus_reg_buf[19] = cfg_settings.hr129;
            reg_value = cfg_settings.hr130;
            modbus_reg_buf[20] = (uint)reg_value >> 8;
            modbus_reg_buf[21] = reg_value;
            reg_value = cfg_settings.hr131;
            modbus_reg_buf[22] = (uint)reg_value >> 8;
            modbus_reg_buf[23] = reg_value;
            reg_value = cfg_settings.hr132;
            modbus_reg_buf[24] = (uint)reg_value >> 8;
            modbus_reg_buf[25] = reg_value;
            reg_value = cfg_settings.hr133;
            modbus_reg_buf[26] = (uint)reg_value >> 8;
            modbus_reg_buf[27] = reg_value;
            reg_value = cfg_settings.hr134;
            modbus_reg_buf[28] = (uint)reg_value >> 8;
            modbus_reg_buf[29] = reg_value;
            reg_value = cfg_settings.hr135;
            modbus_reg_buf[30] = (uint)reg_value >> 8;
            modbus_reg_buf[31] = reg_value;
            reg_value = cfg_settings.hr136;
            modbus_reg_buf[32] = (uint)reg_value >> 8;
            modbus_reg_buf[33] = reg_value;
            reg_value = cfg_settings.hr137;
            modbus_reg_buf[34] = (uint)reg_value >> 8;
            modbus_reg_buf[35] = reg_value;
            reg_value = cfg_settings.hr138;
            modbus_reg_buf[36] = (uint)reg_value >> 8;
            modbus_reg_buf[37] = reg_value;
            reg_value = cfg_settings.hr139;
            modbus_reg_buf[38] = (uint)reg_value >> 8;
            modbus_reg_buf[39] = reg_value;
            reg_value = cfg_settings.hr140;
            modbus_reg_buf[40] = (uint)reg_value >> 8;
            modbus_reg_buf[41] = reg_value;
            reg_value = cfg_settings.hr141;
            modbus_reg_buf[42] = (uint)reg_value >> 8;
            modbus_reg_buf[43] = reg_value;
            reg_value = cfg_settings.hr142;
            modbus_reg_buf[44] = (uint)reg_value >> 8;
            modbus_reg_buf[45] = reg_value;
            reg_value = cfg_settings.hr143;
            modbus_reg_buf[46] = (uint)reg_value >> 8;
            modbus_reg_buf[47] = reg_value;
            reg_value = cfg_settings.hr144;
            modbus_reg_buf[48] = (uint)reg_value >> 8;
            modbus_reg_buf[49] = reg_value;
            reg_value = cfg_settings.hr145;
            modbus_reg_buf[50] = (uint)reg_value >> 8;
            modbus_reg_buf[51] = reg_value;
            reg_value = cfg_settings.hr166;
            modbus_reg_buf[92] = (uint)reg_value >> 8;
            modbus_reg_buf[93] = reg_value;
            reg_value = batt_cal_request;
            modbus_reg_buf[110] = (uint)reg_value >> 8;
            modbus_reg_buf[111] = reg_value;
            reg_value = cfg_settings.hr176;
            modbus_reg_buf[112] = (uint)reg_value >> 8;
            modbus_reg_buf[113] = reg_value;
            reg_value = 0;
            modbus_reg_buf[114] = 0;
            modbus_reg_buf[115] = cfg_settings.hr177;
            reg_value = 0;
            modbus_reg_buf[116] = 0;
            modbus_reg_buf[117] = cfg_settings.hr178;
            reg_value = 0;
            modbus_reg_buf[118] = 0;
            modbus_reg_buf[119] = cfg_settings.hr179;
            i = (ushort)((start_reg << 1) - 0xf0);
            reply_idx = 0x10;
            while ((start_reg << 1) + (reg_count << 1) - 0xf0 > i) {
                modbus_reply[reply_idx] = modbus_reg_buf[i];
                i = (ushort)(i + 1);
                reply_idx++;
            }

            goto L_23;
        }
        if (start_reg < 0xf0) {
            reg_value = (ushort)(cfg_block8[1] + (cfg_block8[0] << 8));
            modbus_reg_buf[0] = (uint)reg_value >> 8;
            modbus_reg_buf[1] = reg_value;
            reg_value = (ushort)(cfg_block8[3] + (cfg_block8[2] << 8));
            modbus_reg_buf[2] = (uint)reg_value >> 8;
            modbus_reg_buf[3] = reg_value;
            reg_value = (ushort)(cfg_block8[5] + (cfg_block8[4] << 8));
            modbus_reg_buf[4] = (uint)reg_value >> 8;
            modbus_reg_buf[5] = reg_value;
            reg_value = (ushort)(cfg_block8[7] + (cfg_block8[6] << 8));
            modbus_reg_buf[6] = (uint)reg_value >> 8;
            modbus_reg_buf[7] = reg_value;
            reg_value = (ushort)(cfg_block8[9] + (cfg_block8[8] << 8));
            modbus_reg_buf[8] = (uint)reg_value >> 8;
            modbus_reg_buf[9] = reg_value;
            reg_value = (ushort)(cfg_block8[11] + (cfg_block8[10] << 8));
            modbus_reg_buf[10] = (uint)reg_value >> 8;
            modbus_reg_buf[11] = reg_value;
            reg_value = (ushort)(cfg_block8[13] + (cfg_block8[12] << 8));
            modbus_reg_buf[12] = (uint)reg_value >> 8;
            modbus_reg_buf[13] = reg_value;
            reg_value = dev11_data.h00;
            modbus_reg_buf[44] = (uint)reg_value >> 8;
            modbus_reg_buf[45] = reg_value;
            reg_value = dev11_data.h02;
            modbus_reg_buf[46] = (uint)reg_value >> 8;
            modbus_reg_buf[47] = reg_value;
            reg_value = dev11_data.h04;
            modbus_reg_buf[48] = (uint)reg_value >> 8;
            modbus_reg_buf[49] = reg_value;
            reg_value = (ushort)(*(int *)&dev11_data.h06);
            modbus_reg_buf[50] = (uint)reg_value >> 8;
            modbus_reg_buf[51] = reg_value;
            reg_value = (ushort)(*(int *)&dev11_data.h08);
            modbus_reg_buf[52] = (uint)reg_value >> 8;
            modbus_reg_buf[53] = reg_value;
            reg_value = (ushort)(*(int *)&dev11_data.h0a);
            modbus_reg_buf[54] = (uint)reg_value >> 8;
            modbus_reg_buf[55] = reg_value;
            reg_value = dev11_data.h0c;
            modbus_reg_buf[56] = (uint)reg_value >> 8;
            modbus_reg_buf[57] = reg_value;
            reg_value = dev11_data.h0e;
            modbus_reg_buf[58] = (uint)reg_value >> 8;
            modbus_reg_buf[59] = reg_value;
            reg_value = dev11_data.h10;
            modbus_reg_buf[60] = (uint)reg_value >> 8;
            modbus_reg_buf[61] = reg_value;
            reg_value = dev11_data.h12;
            modbus_reg_buf[62] = (uint)reg_value >> 8;
            modbus_reg_buf[63] = reg_value;
            reg_value = dev11_data.h14;
            modbus_reg_buf[64] = (uint)reg_value >> 8;
            modbus_reg_buf[65] = reg_value;
            reg_value = dev11_data.h16;
            modbus_reg_buf[66] = (uint)reg_value >> 8;
            modbus_reg_buf[67] = reg_value;
            reg_value = (ushort)(dev11_data.b18[1] + (dev11_data.b18[0] << 8));
            modbus_reg_buf[68] = (uint)reg_value >> 8;
            modbus_reg_buf[69] = reg_value;
            reg_value = (ushort)(dev11_data.b18[3] + dev11_data.b18[2] * 0x100);
            modbus_reg_buf[70] = (uint)reg_value >> 8;
            modbus_reg_buf[71] = reg_value;
            reg_value = (ushort)(dev11_data.b18[5] + dev11_data.b18[4] * 0x100);
            modbus_reg_buf[72] = (uint)reg_value >> 8;
            modbus_reg_buf[73] = reg_value;
            reg_value = (ushort)(dev11_data.b18[7] + dev11_data.b18[6] * 0x100);
            modbus_reg_buf[74] = (uint)reg_value >> 8;
            modbus_reg_buf[75] = reg_value;
            reg_value = (ushort)(dev11_data.b18[9] + dev11_data.b18[8] * 0x100);
            modbus_reg_buf[76] = (uint)reg_value >> 8;
            modbus_reg_buf[77] = reg_value;
            reg_value = dev11_data.h22;
            modbus_reg_buf[78] = (uint)reg_value >> 8;
            modbus_reg_buf[79] = reg_value;
            reg_value = dev11_data.h24;
            modbus_reg_buf[80] = (uint)reg_value >> 8;
            modbus_reg_buf[81] = reg_value;
            reg_value = dev11_data.h26;
            modbus_reg_buf[82] = (uint)reg_value >> 8;
            modbus_reg_buf[83] = reg_value;
            reg_value = dev11_data.h28;
            modbus_reg_buf[84] = (uint)reg_value >> 8;
            modbus_reg_buf[85] = reg_value;
            reg_value = dev11_data.w30 >> 0x10;
            modbus_reg_buf[86] = (uint)reg_value >> 8;
            modbus_reg_buf[87] = reg_value;
            reg_value = (ushort)dev11_data.w30;
            modbus_reg_buf[88] = (uint)reg_value >> 8;
            modbus_reg_buf[89] = reg_value;
            modbus_reg_buf[100] = 0;
            modbus_reg_buf[101] = cfg_block6.hr230;
            reg_value = dev11_data.h34;
            modbus_reg_buf[102] = (uint)reg_value >> 8;
            modbus_reg_buf[103] = reg_value;
            i = (ushort)((start_reg << 1) - 0x168);
            reply_idx = 0x10;
            while ((start_reg << 1) + (reg_count << 1) - 0x168 > i) {
                modbus_reply[reply_idx] = modbus_reg_buf[i];
                i = (ushort)(i + 1);
                reply_idx++;
            }
            goto L_23;
        }
        if (start_reg < 0x12c) {
            reg_value = 0;
            modbus_reg_buf[0] = 0;
            modbus_reg_buf[1] = reg_value;
            reg_value = 0;
            modbus_reg_buf[2] = (uint)reg_value >> 8;
            modbus_reg_buf[3] = reg_value;
            reg_value = cfg_block6.hr242;
            modbus_reg_buf[4] = 0;
            modbus_reg_buf[5] = reg_value;
            reg_value = cfg_block6.slot_b1_start;
            modbus_reg_buf[6] = (uint)reg_value >> 8;
            modbus_reg_buf[7] = reg_value;
            reg_value = cfg_block6.slot_b1_end;
            modbus_reg_buf[8] = (uint)reg_value >> 8;
            modbus_reg_buf[9] = reg_value;
            reg_value = cfg_block6.slot_b1_soc;
            modbus_reg_buf[10] = 0;
            modbus_reg_buf[11] = reg_value;
            reg_value = cfg_block6.slot_b2_start;
            modbus_reg_buf[12] = (uint)reg_value >> 8;
            modbus_reg_buf[13] = reg_value;
            reg_value = cfg_block6.slot_b2_end;
            modbus_reg_buf[14] = (uint)reg_value >> 8;
            modbus_reg_buf[15] = reg_value;
            reg_value = cfg_block6.slot_b2_soc;
            modbus_reg_buf[16] = 0;
            modbus_reg_buf[17] = reg_value;
            reg_value = cfg_block6.slot_b3_start;
            modbus_reg_buf[18] = (uint)reg_value >> 8;
            modbus_reg_buf[19] = reg_value;
            reg_value = cfg_block6.slot_b3_end;
            modbus_reg_buf[20] = (uint)reg_value >> 8;
            modbus_reg_buf[21] = reg_value;
            reg_value = cfg_block6.slot_b3_soc;
            modbus_reg_buf[22] = 0;
            modbus_reg_buf[23] = reg_value;
            reg_value = cfg_block6.slot_b4_start;
            modbus_reg_buf[24] = (uint)reg_value >> 8;
            modbus_reg_buf[25] = reg_value;
            reg_value = cfg_block6.slot_b4_end;
            modbus_reg_buf[26] = (uint)reg_value >> 8;
            modbus_reg_buf[27] = reg_value;
            reg_value = cfg_block6.slot_b4_soc;
            modbus_reg_buf[28] = 0;
            modbus_reg_buf[29] = reg_value;
            reg_value = cfg_block6.slot_b5_start;
            modbus_reg_buf[30] = (uint)reg_value >> 8;
            modbus_reg_buf[31] = reg_value;
            reg_value = cfg_block6.slot_b5_end;
            modbus_reg_buf[32] = (uint)reg_value >> 8;
            modbus_reg_buf[33] = reg_value;
            reg_value = cfg_block6.slot_b5_soc;
            modbus_reg_buf[34] = 0;
            modbus_reg_buf[35] = reg_value;
            reg_value = cfg_block6.slot_b6_start;
            modbus_reg_buf[36] = (uint)reg_value >> 8;
            modbus_reg_buf[37] = reg_value;
            reg_value = cfg_block6.slot_b6_end;
            modbus_reg_buf[38] = (uint)reg_value >> 8;
            modbus_reg_buf[39] = reg_value;
            reg_value = cfg_block6.slot_b6_soc;
            modbus_reg_buf[40] = 0;
            modbus_reg_buf[41] = reg_value;
            reg_value = cfg_block6.slot_b7_start;
            modbus_reg_buf[42] = (uint)reg_value >> 8;
            modbus_reg_buf[43] = reg_value;
            reg_value = cfg_block6.slot_b7_end;
            modbus_reg_buf[44] = (uint)reg_value >> 8;
            modbus_reg_buf[45] = reg_value;
            reg_value = cfg_block6.slot_b7_soc;
            modbus_reg_buf[46] = 0;
            modbus_reg_buf[47] = reg_value;
            reg_value = cfg_block6.slot_b8_start;
            modbus_reg_buf[48] = (uint)reg_value >> 8;
            modbus_reg_buf[49] = reg_value;
            reg_value = cfg_block6.slot_b8_end;
            modbus_reg_buf[50] = (uint)reg_value >> 8;
            modbus_reg_buf[51] = reg_value;
            reg_value = cfg_block6.slot_b8_soc;
            modbus_reg_buf[52] = 0;
            modbus_reg_buf[53] = reg_value;
            reg_value = cfg_block6.slot_b9_start;
            modbus_reg_buf[54] = (uint)reg_value >> 8;
            modbus_reg_buf[55] = reg_value;
            reg_value = cfg_block6.slot_b9_end;
            modbus_reg_buf[56] = (uint)reg_value >> 8;
            modbus_reg_buf[57] = reg_value;
            reg_value = cfg_block6.slot_b9_soc;
            modbus_reg_buf[58] = 0;
            modbus_reg_buf[59] = reg_value;
            reg_value = 0;
            modbus_reg_buf[60] = 0;
            modbus_reg_buf[61] = reg_value;
            reg_value = 0;
            modbus_reg_buf[62] = (uint)reg_value >> 8;
            modbus_reg_buf[63] = reg_value;
            reg_value = cfg_block6.hr272;
            modbus_reg_buf[64] = 0;
            modbus_reg_buf[65] = reg_value;
            reg_value = 0;
            modbus_reg_buf[66] = (uint)reg_value >> 8;
            modbus_reg_buf[67] = reg_value;
            reg_value = 0;
            modbus_reg_buf[68] = (uint)reg_value >> 8;
            modbus_reg_buf[69] = reg_value;
            reg_value = cfg_block6.hr275;
            modbus_reg_buf[70] = 0;
            modbus_reg_buf[71] = reg_value;
            reg_value = cfg_block6.slot_a1_start;
            modbus_reg_buf[72] = (uint)reg_value >> 8;
            modbus_reg_buf[73] = reg_value;
            reg_value = cfg_block6.slot_a1_end;
            modbus_reg_buf[74] = (uint)reg_value >> 8;
            modbus_reg_buf[75] = reg_value;
            reg_value = cfg_block6.slot_a1_soc;
            modbus_reg_buf[76] = 0;
            modbus_reg_buf[77] = reg_value;
            reg_value = cfg_block6.slot_a2_start;
            modbus_reg_buf[78] = (uint)reg_value >> 8;
            modbus_reg_buf[79] = reg_value;
            reg_value = cfg_block6.slot_a2_end;
            modbus_reg_buf[80] = (uint)reg_value >> 8;
            modbus_reg_buf[81] = reg_value;
            reg_value = cfg_block6.slot_a2_soc;
            modbus_reg_buf[82] = 0;
            modbus_reg_buf[83] = reg_value;
            reg_value = cfg_block6.slot_a3_start;
            modbus_reg_buf[84] = (uint)reg_value >> 8;
            modbus_reg_buf[85] = reg_value;
            reg_value = cfg_block6.slot_a3_end;
            modbus_reg_buf[86] = (uint)reg_value >> 8;
            modbus_reg_buf[87] = reg_value;
            reg_value = cfg_block6.slot_a3_soc;
            modbus_reg_buf[88] = 0;
            modbus_reg_buf[89] = reg_value;
            reg_value = cfg_block6.slot_a4_start;
            modbus_reg_buf[90] = (uint)reg_value >> 8;
            modbus_reg_buf[91] = reg_value;
            reg_value = cfg_block6.slot_a4_end;
            modbus_reg_buf[92] = (uint)reg_value >> 8;
            modbus_reg_buf[93] = reg_value;
            reg_value = cfg_block6.slot_a4_soc;
            modbus_reg_buf[94] = 0;
            modbus_reg_buf[95] = reg_value;
            reg_value = cfg_block6.slot_a5_start;
            modbus_reg_buf[96] = (uint)reg_value >> 8;
            modbus_reg_buf[97] = reg_value;
            reg_value = cfg_block6.slot_a5_end;
            modbus_reg_buf[98] = (uint)reg_value >> 8;
            modbus_reg_buf[99] = reg_value;
            reg_value = cfg_block6.slot_a5_soc;
            modbus_reg_buf[100] = 0;
            modbus_reg_buf[101] = reg_value;
            reg_value = cfg_block6.slot_a6_start;
            modbus_reg_buf[102] = (uint)reg_value >> 8;
            modbus_reg_buf[103] = reg_value;
            reg_value = cfg_block6.slot_a6_end;
            modbus_reg_buf[104] = (uint)reg_value >> 8;
            modbus_reg_buf[105] = reg_value;
            reg_value = cfg_block6.slot_a6_soc;
            modbus_reg_buf[106] = 0;
            modbus_reg_buf[107] = reg_value;
            reg_value = cfg_block6.slot_a7_start;
            modbus_reg_buf[108] = (uint)reg_value >> 8;
            modbus_reg_buf[109] = reg_value;
            reg_value = cfg_block6.slot_a7_end;
            modbus_reg_buf[110] = (uint)reg_value >> 8;
            modbus_reg_buf[111] = reg_value;
            reg_value = cfg_block6.slot_a7_soc;
            modbus_reg_buf[112] = 0;
            modbus_reg_buf[113] = reg_value;
            reg_value = cfg_block6.slot_a8_start;
            modbus_reg_buf[114] = (uint)reg_value >> 8;
            modbus_reg_buf[115] = reg_value;
            reg_value = cfg_block6.slot_a8_end;
            modbus_reg_buf[116] = (uint)reg_value >> 8;
            modbus_reg_buf[117] = reg_value;
            reg_value = cfg_block6.slot_a8_soc;
            modbus_reg_buf[118] = 0;
            modbus_reg_buf[119] = reg_value;
            i = (ushort)((start_reg << 1) - 0x1e0);
            reply_idx = 0x10;
            while ((start_reg << 1) + (reg_count << 1) - 0x1e0 > i) {
                modbus_reply[reply_idx] = modbus_reg_buf[i];
                i = (ushort)(i + 1);
                reply_idx++;
            }
            goto L_23;
        }
        if (start_reg < 0x168) {
            reg_value = cfg_settings.hr300;
            modbus_reg_buf[0] = (uint)reg_value >> 8;
            modbus_reg_buf[1] = reg_value;
            reg_value = cfg_settings.hr301;
            modbus_reg_buf[2] = (uint)reg_value >> 8;
            modbus_reg_buf[3] = reg_value;
            reg_value = cfg_settings.hr302;
            modbus_reg_buf[4] = (uint)reg_value >> 8;
            modbus_reg_buf[5] = reg_value;
            reg_value = cfg_settings.hr303;
            modbus_reg_buf[6] = (uint)reg_value >> 8;
            modbus_reg_buf[7] = reg_value;
            reg_value = cfg_settings.hr305;
            modbus_reg_buf[10] = 0;
            modbus_reg_buf[11] = reg_value;
            reg_value = cfg_settings.hr306;
            modbus_reg_buf[12] = (uint)reg_value >> 8;
            modbus_reg_buf[13] = reg_value;
            reg_value = cfg_settings.hr307;
            modbus_reg_buf[14] = (uint)reg_value >> 8;
            modbus_reg_buf[15] = reg_value;
            reg_value = cfg_settings.hr308;
            modbus_reg_buf[16] = (uint)reg_value >> 8;
            modbus_reg_buf[17] = reg_value;
            reg_value = cfg_settings.hr309;
            modbus_reg_buf[18] = (uint)reg_value >> 8;
            modbus_reg_buf[19] = reg_value;
            reg_value = cfg_settings.hr310;
            modbus_reg_buf[20] = 0;
            modbus_reg_buf[21] = reg_value;
            reg_value = cfg_settings.hr311;
            modbus_reg_buf[22] = 0;
            modbus_reg_buf[23] = reg_value;
            reg_value = cfg_settings.hr312;
            modbus_reg_buf[24] = (uint)reg_value >> 8;
            modbus_reg_buf[25] = reg_value;
            reg_value = cfg_settings.hr313;
            modbus_reg_buf[26] = 0;
            modbus_reg_buf[27] = reg_value;
            reg_value = cfg_settings.hr314;
            modbus_reg_buf[28] = 0;
            modbus_reg_buf[29] = reg_value;
            reg_value = cfg_settings.hr315;
            modbus_reg_buf[30] = (uint)reg_value >> 8;
            modbus_reg_buf[31] = reg_value;
            reg_value = cfg_settings.hr316;
            modbus_reg_buf[32] = (uint)reg_value >> 8;
            modbus_reg_buf[33] = reg_value;
            reg_value = (uchar)cfg_settings.hr102 & 1;
            modbus_reg_buf[34] = (uint)reg_value >> 8;
            modbus_reg_buf[35] = reg_value;
            reg_value = cfg_settings.hr318;
            modbus_reg_buf[36] = 0;
            modbus_reg_buf[37] = reg_value;
            reg_value = cfg_settings.hr319;
            modbus_reg_buf[38] = (uint)reg_value >> 8;
            modbus_reg_buf[39] = reg_value;
            reg_value = cfg_settings.hr320;
            modbus_reg_buf[40] = (uint)reg_value >> 8;
            modbus_reg_buf[41] = reg_value;
            reg_value = cfg_settings.hr321;
            modbus_reg_buf[42] = (uint)reg_value >> 8;
            modbus_reg_buf[43] = reg_value;
            reg_value = cfg_block6.hr322;
            modbus_reg_buf[44] = (uint)reg_value >> 8;
            modbus_reg_buf[45] = reg_value;
            reg_value = cfg_block6.hr323;
            modbus_reg_buf[46] = (uint)reg_value >> 8;
            modbus_reg_buf[47] = reg_value;
            reg_value = cfg_block6.hr324;
            modbus_reg_buf[48] = (uint)reg_value >> 8;
            modbus_reg_buf[49] = reg_value;
            reg_value = cfg_block6.hr325;
            modbus_reg_buf[50] = (uint)reg_value >> 8;
            modbus_reg_buf[51] = reg_value;
            reg_value = cfg_settings.hr326;
            modbus_reg_buf[52] = (uint)reg_value >> 8;
            modbus_reg_buf[53] = reg_value;
            reg_value = cfg_settings.hr327;
            modbus_reg_buf[54] = (uint)reg_value >> 8;
            modbus_reg_buf[55] = reg_value;
            reg_value = cfg_settings.hr328;
            modbus_reg_buf[56] = (uint)reg_value >> 8;
            modbus_reg_buf[57] = reg_value;
            reg_value = force_state_hr329;
            modbus_reg_buf[58] = (uint)reg_value >> 8;
            modbus_reg_buf[59] = reg_value;
            reg_value = cfg_settings.hr330;
            modbus_reg_buf[60] = (uint)reg_value >> 8;
            modbus_reg_buf[61] = reg_value;
            reg_value = cfg_settings.hr331;
            modbus_reg_buf[62] = (uint)reg_value >> 8;
            modbus_reg_buf[63] = reg_value;
            reg_value = cfg_settings.fD2;
            modbus_reg_buf[64] = (uint)reg_value >> 8;
            modbus_reg_buf[65] = reg_value;
            reg_value = cfg_block6.hr351;
            modbus_reg_buf[102] = (uint)reg_value >> 8;
            modbus_reg_buf[103] = reg_value;
            reg_value = cfg_block6.hr352;
            modbus_reg_buf[104] = (uint)reg_value >> 8;
            modbus_reg_buf[105] = reg_value;
            reg_value = cfg_block6.hr353;
            modbus_reg_buf[106] = (uint)reg_value >> 8;
            modbus_reg_buf[107] = reg_value;
            i = (ushort)((start_reg << 1) - 0x258);
            reply_idx = 0x10;
            while ((start_reg << 1) + (reg_count << 1) - 0x258 > i) {
                modbus_reply[reply_idx] = modbus_reg_buf[i];
                i = (ushort)(i + 1);
                reply_idx++;
            }
        } else {
            if (start_reg < 0x1a4) {
                for (i = 0; i < 0x30; i = (ushort)(i + 1)) {
                    reg_value = sched_table_b[i];
                    modbus_reg_buf[i * 2] = (uint)reg_value >> 8;
                    modbus_reg_buf[(i << 1) + 1] = reg_value;
                }
                i = (ushort)((start_reg << 1) - 0x2d0);
                reply_idx = 0x10;
                while ((start_reg << 1) + (reg_count << 1) - 0x2d0 > i) {
                    modbus_reply[reply_idx] = modbus_reg_buf[i];
                    i = (ushort)(i + 1);
                    reply_idx++;
                }
            } else {
                if (start_reg < 0x1e0) {
                    for (i = 0; i < 0x30; i = (ushort)(i + 1)) {
                        /* match: cfg_hr360_table[0][i], reached from sched_table_b, which is
                           cfg_hr360_table[1]; written as cfg_hr360_table the literal differs */
                        reg_value = (sched_table_b - 48)[i];
                        modbus_reg_buf[i * 2] = (uint)reg_value >> 8;
                        modbus_reg_buf[(i << 1) + 1] = reg_value;
                    }
                    i = (ushort)((start_reg << 1) - 0x348);
                    reply_idx = 0x10;
                    while ((start_reg << 1) + (reg_count << 1) - 0x348 > i) {
                        modbus_reply[reply_idx] = modbus_reg_buf[i];
                        i = (ushort)(i + 1);
                        reply_idx++;
                    }
                } else {
                    if (start_reg < 0x21c) {
                        reg_value = inv_run_mode;
                        modbus_reg_buf[0] = (uint)reg_value >> 8;
                        modbus_reg_buf[1] = reg_value;
                        modbus_reg_buf[2] = 0;
                        modbus_reg_buf[3] = cfg_settings.battery_power_mode;
                        modbus_reg_buf[4] = 0;
                        modbus_reg_buf[5] = cfg_settings.soc_force_adjust;
                        reg_value = cfg_settings.discharge_slot_1_start;
                        modbus_reg_buf[6] = (uint)reg_value >> 8;
                        modbus_reg_buf[7] = reg_value;
                        reg_value = cfg_settings.discharge_slot_1_end;
                        modbus_reg_buf[8] = (uint)reg_value >> 8;
                        modbus_reg_buf[9] = reg_value;
                        reg_value = cfg_settings.enable_discharge;
                        modbus_reg_buf[10] = (uint)reg_value >> 8;
                        modbus_reg_buf[11] = reg_value;
                        reg_value = cfg_settings.charge_slot_1_start;
                        modbus_reg_buf[12] = (uint)reg_value >> 8;
                        modbus_reg_buf[13] = reg_value;
                        reg_value = cfg_settings.charge_slot_1_end;
                        modbus_reg_buf[14] = (uint)reg_value >> 8;
                        modbus_reg_buf[15] = reg_value;
                        reg_value = cfg_settings.enable_charge;
                        modbus_reg_buf[16] = 0;
                        modbus_reg_buf[17] = reg_value;
                        modbus_reg_buf[18] = cfg_settings.active_power_rate >> 8;
                        modbus_reg_buf[19] = cfg_settings.active_power_rate;
                        reg_value = cfg_settings.f09;
                        reg_value = cfg_settings.inverter_state | reg_value << 8;
                        modbus_reg_buf[20] = (uint)reg_value >> 8;
                        modbus_reg_buf[21] = reg_value;
                        modbus_reg_buf[22] = batt_soc >> 8;
                        modbus_reg_buf[23] = batt_soc;
                        reg_value = 0;
                        modbus_reg_buf[24] = (uint)p_battery >> 8;
                        modbus_reg_buf[25] = (uchar)p_battery;
                        reg_value = (ushort)(p_load_demand / 0xa);
                        modbus_reg_buf[26] = (uint)reg_value >> 8;
                        modbus_reg_buf[27] = reg_value;
                        reg_value = 0;
                        modbus_reg_buf[28] = 0;
                        modbus_reg_buf[29] = cfg_block6.hr494;
                        modbus_reg_buf[30] = batt_power_remote >> 8;
                        modbus_reg_buf[31] = (uchar)batt_power_remote;
                        reg_value = p_inverter_out / 0xa;
                        modbus_reg_buf[34] = (uint)reg_value >> 8;
                        modbus_reg_buf[35] = reg_value;
                        modbus_reg_buf[36] = dsp_fault_words[0];
                        modbus_reg_buf[37] = dsp_fault_words[1];
                        modbus_reg_buf[38] = dsp_fault_words[2];
                        modbus_reg_buf[39] = dsp_fault_words[3];
                        modbus_reg_buf[40] = dsp_fault_words[4];
                        modbus_reg_buf[41] = dsp_fault_words[5];
                        modbus_reg_buf[42] = dsp_fault_words[6];
                        modbus_reg_buf[43] = dsp_fault_words[7];
                        modbus_reg_buf[44] = dsp_fault_words[8];
                        modbus_reg_buf[45] = dsp_fault_words[9];
                        modbus_reg_buf[46] = dsp_fault_words[10];
                        modbus_reg_buf[47] = dsp_fault_words[11];
                        modbus_reg_buf[48] = dsp_fault_words[12];
                        modbus_reg_buf[49] = dsp_fault_words[13];
                        modbus_reg_buf[50] = dsp_fault_words[14];
                        modbus_reg_buf[51] = dsp_fault_words[15];
                        reg_value = dsp_warning_word;
                        if (board_init_done >= 1) {
                            reg_value = reg_value | 1;
                        }
                        modbus_reg_buf[52] = (uint)reg_value >> 8;
                        modbus_reg_buf[53] = reg_value;
                        modbus_reg_buf[54] = bus_p_voltage >> 8;
                        modbus_reg_buf[55] = (uchar)bus_p_voltage;
                        reg_value = bus_hv_voltage;
                        modbus_reg_buf[56] = (uint)reg_value >> 8;
                        modbus_reg_buf[57] = reg_value;
                        reg_value = charge_status;
                        modbus_reg_buf[58] = (uint)reg_value >> 8;
                        modbus_reg_buf[59] = reg_value;
                        modbus_reg_buf[60] = sys_mode >> 8;
                        modbus_reg_buf[61] = (uchar)sys_mode;
                        modbus_reg_buf[62] = batt_voltage >> 8;
                        modbus_reg_buf[63] = (uchar)batt_voltage;
                        if (batt_capacity_bms > 0) {
                            reg_value = batt_capacity_bms;
                        } else {
                            reg_value = cfg_settings.battery_capacity;
                        }
                        modbus_reg_buf[64] = (uint)reg_value >> 8;
                        modbus_reg_buf[65] = reg_value;
                        modbus_reg_buf[66] = 0;
                        modbus_reg_buf[67] = discharge_min_soc_now;
                        modbus_reg_buf[68] = 0;
                        modbus_reg_buf[69] = charge_target_soc_now[0];
                        modbus_reg_buf[70] = 0;
                        modbus_reg_buf[71] = cfg_settings.hr314;
                        modbus_reg_buf[72] = 0;
                        modbus_reg_buf[73] = (uchar)cfg_settings.inverter_module_l;
                        modbus_reg_buf[74] = 0;
                        modbus_reg_buf[75] = cfg_settings.hr311;
                        i = (ushort)((start_reg << 1) - 0x3c0);
                        reply_idx = 0x10;
                        while ((start_reg << 1) + (reg_count << 1) - 0x3c0 > i) {
                            modbus_reply[reply_idx] = modbus_reg_buf[i];
                            i = (ushort)(i + 1);
                            reply_idx++;
                        }
                    } else {
                        if (start_reg >= 0xff0 && start_reg < 0x102c) {
                            reg_value = cal_gains.gain0;
                            modbus_reg_buf[34] = (uint)reg_value >> 8;
                            modbus_reg_buf[35] = reg_value;
                            reg_value = cal_gains.gain2;
                            modbus_reg_buf[36] = (uint)reg_value >> 8;
                            modbus_reg_buf[37] = reg_value;
                            reg_value = cal_gains.gain4;
                            modbus_reg_buf[38] = (uint)reg_value >> 8;
                            modbus_reg_buf[39] = reg_value;
                            reg_value = cal_gains.gain5;
                            modbus_reg_buf[42] = (uint)reg_value >> 8;
                            modbus_reg_buf[43] = reg_value;
                            reg_value = 0;
                            modbus_reg_buf[44] = (uint)reg_value >> 8;
                            modbus_reg_buf[45] = reg_value;
                            reg_value = 0;
                            modbus_reg_buf[46] = (uint)reg_value >> 8;
                            modbus_reg_buf[47] = reg_value;
                            reg_value = cal_gains.gain1;
                            modbus_reg_buf[48] = (uint)reg_value >> 8;
                            modbus_reg_buf[49] = reg_value;
                            reg_value = cal_gains.gain3;
                            modbus_reg_buf[50] = (uint)reg_value >> 8;
                            modbus_reg_buf[51] = reg_value;
                            reg_value = cal_gains.gain6;
                            modbus_reg_buf[52] = (uint)reg_value >> 8;
                            modbus_reg_buf[53] = reg_value;
                            reg_value = 0;
                            modbus_reg_buf[54] = (uint)reg_value >> 8;
                            modbus_reg_buf[55] = reg_value;
                            reg_value = 0;
                            modbus_reg_buf[56] = (uint)reg_value >> 8;
                            modbus_reg_buf[57] = reg_value;
                            reg_value = cal_gains.gain7;
                            modbus_reg_buf[80] = (uint)reg_value >> 8;
                            modbus_reg_buf[81] = reg_value;
                            i = (ushort)((start_reg << 1) - 0x1fe0);
                            reply_idx = 0x10;
                            while ((start_reg << 1) + (reg_count << 1) - 0x1fe0 > i) {
                                modbus_reply[reply_idx] = modbus_reg_buf[i];
                                i = (ushort)(i + 1);
                                reply_idx++;
                            }
                        } else if (start_reg >= 0x102c && start_reg < 0x1068) {
                            reg_value = cal_gains.gain8;
                            modbus_reg_buf[12] = (uint)reg_value >> 8;
                            modbus_reg_buf[13] = reg_value;
                            i = (ushort)((start_reg << 1) - 0x2058);
                            reply_idx = 0x10;
                            while ((start_reg << 1) + (reg_count << 1) - 0x2000 - 0x58 > i) {
                                modbus_reply[reply_idx] = modbus_reg_buf[i];
                                i = (ushort)(i + 1);
                                reply_idx++;
                            }
                        } else if (start_reg >= 0x10f4 && start_reg < 0x1130) {
                            reg_value = (ushort)(*(int *)&cal_gains.offset0);
                            modbus_reg_buf[28] = (uint)reg_value >> 8;
                            modbus_reg_buf[29] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset2);
                            modbus_reg_buf[30] = (uint)reg_value >> 8;
                            modbus_reg_buf[31] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset4);
                            modbus_reg_buf[32] = (uint)reg_value >> 8;
                            modbus_reg_buf[33] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset5);
                            modbus_reg_buf[36] = (uint)reg_value >> 8;
                            modbus_reg_buf[37] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset1);
                            modbus_reg_buf[42] = (uint)reg_value >> 8;
                            modbus_reg_buf[43] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset3);
                            modbus_reg_buf[44] = (uint)reg_value >> 8;
                            modbus_reg_buf[45] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset6);
                            modbus_reg_buf[46] = (uint)reg_value >> 8;
                            modbus_reg_buf[47] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset7);
                            modbus_reg_buf[52] = (uint)reg_value >> 8;
                            modbus_reg_buf[53] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset8);
                            modbus_reg_buf[76] = (uint)reg_value >> 8;
                            modbus_reg_buf[77] = reg_value;
                            reg_value = cal_gains.gain11;
                            modbus_reg_buf[80] = (uint)reg_value >> 8;
                            modbus_reg_buf[81] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset11);
                            modbus_reg_buf[82] = (uint)reg_value >> 8;
                            modbus_reg_buf[83] = reg_value;
                            reg_value = cal_gains.gain9;
                            modbus_reg_buf[84] = (uint)reg_value >> 8;
                            modbus_reg_buf[85] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset9);
                            modbus_reg_buf[86] = (uint)reg_value >> 8;
                            modbus_reg_buf[87] = reg_value;
                            reg_value = cal_gains.gain10;
                            modbus_reg_buf[88] = (uint)reg_value >> 8;
                            modbus_reg_buf[89] = reg_value;
                            reg_value = (ushort)(*(int *)&cal_gains.offset10);
                            modbus_reg_buf[90] = (uint)reg_value >> 8;
                            modbus_reg_buf[91] = reg_value;
                            i = (ushort)((start_reg << 1) - 0x21e8);
                            reply_idx = 0x10;
                            while ((start_reg << 1) + (reg_count << 1) - 0x2100 - 0xe8 > i) {
                                modbus_reply[reply_idx] = modbus_reg_buf[i];
                                i = (ushort)(i + 1);
                                reply_idx++;
                            }
                        }
                    }
                }
            }
        }
    }
    crc = crc16_modbus(&modbus_reply[0], reply_idx);
    modbus_reply[reply_idx] = crc;
    modbus_reply[reply_idx + 1] = crc >> 8;
    return;
}

/* Handles one request from either transport (modbus_src 1 = USB CDC, 2 = RS485).

   The CRC check differs by transport. USB: an 8-byte request must carry its CRC low byte first
   at 6-7; an FC16 request (CRC over 6 + 2 x count bytes, count capped at 100) is checked at its
   end high byte first. RS485: the CRC must be high byte first (at 6-7, or at the end of an FC16
   request). The replies always send the CRC low byte first. So a standard Modbus master on
   RS485, which sends low byte first, is refused (docs/findings.md).

   A good request for an accepted slave is dispatched (3, 4, 6, 16, 22); other function codes get
   no answer. After a write with anything dirty the EEPROM save is requested with a 100-pass
   delay. Writes to slave 0 that came over RS485 get no reply. The reply length is 18 bytes for
   writes and
   errors and 18 + 2 x count (at most 150) for reads; it goes out on CDC after waiting for
   modbus_usb_wait (SysTick clears it at the next tick) or on USART3 with the driver enable and a
   3-tick pause. */
void modbus_slave_process(uchar *req) {
    int reg_count;
    uchar i;
    int address_match;
    int start_reg;
    int request_valid;

    address_match = 0;
    request_valid = address_match;
    start_reg = address_match;
    reg_count = 0;
    start_reg = req[2];
    start_reg = req[3] | start_reg << 8;
    reg_count = req[4];
    reg_count = req[5] | reg_count << 8;
    if (reg_count > 0x64) {
        reg_count = 0x64;
    }
    if (modbus_src == 1) {
        if (req[1] == MB_WRITE_MULTIPLE) {
            modbus_crc = crc16_modbus(req, (ushort)((reg_count << 1) + 6));
        } else {
            modbus_crc = crc16_modbus(req, 6);
        }
    } else {
        modbus_crc = crc16_modbus(req, (ushort)(modbus_req_len - 2));
    }
    if (modbus_src == 1) {
        if ((uchar)modbus_crc == req[6] && req[7] == modbus_crc >> 8) {
            request_valid = 1;
        } else {
            if ((uchar)modbus_crc == req[(reg_count << 1) + 8 - 1] &&
                req[(reg_count << 1) + 8 - 2] == modbus_crc >> 8) {
                request_valid = 1;
            } else {
                request_valid = 0;
            }
        }
        usb_cdc_timer = 0;
    } else {
        if (modbus_src == 2) {
            if ((uchar)modbus_crc == req[7] && req[6] == modbus_crc >> 8) {
                request_valid = 1;
            } else {
                if ((uchar)modbus_crc == req[modbus_req_len - 1] &&
                    req[modbus_req_len - 2] == modbus_crc >> 8) {
                    request_valid = 1;
                } else {
                    request_valid = 0;
                }
            }
        } else {
            request_valid = 0;
        }
    }
    if (request_valid == 1) {
        if (*req == cfg_settings.modbus_address || *req >= 1 && *req <= 8 || *req == 0x11 ||
            *req == 0 || *req == 0x30 || *req == 0x31 || *req >= 0x32 && *req <= 0x37) {
            address_match = 1;
        }
        if (address_match != 1) {
            modbus_src = 0;
            return;
        }
        modbus_reply[0] = *req;
        modbus_reply[1] = req[1];
        if (address_match == 1 && req[1] >= 3 && req[1] <= 0x16) {
            switch (req[1]) {
            case MB_READ_HOLDING:
                modbus_fc03_read_holding(req);
                break;
            case MB_READ_INPUT:
                modbus_fc04_read_input(req);
                break;
            case MB_WRITE_SINGLE:
                modbus_fc06_write_reg(req);
                if (eeprom_dirty > 0) {
                    save_req_pending = 1;
                    eeprom_save_delay = 0x64;
                }
                if (*req == 0 && modbus_src == 2) {
                    modbus_no_reply = 1;
                }
                break;
            case MB_WRITE_MULTIPLE:
                modbus_fc16_write_regs(start_reg, reg_count, &modbus_reply[0], req);
                if (eeprom_dirty > 0) {
                    save_req_pending = 1;
                    eeprom_save_delay = 0x64;
                }
                if (*req == 0 && modbus_src == 2) {
                    modbus_no_reply = 1;
                }
                break;
            case MB_READ_METER:
                modbus_fc22_read_meter(req);
                break;
            default:
                modbus_no_reply = 1;
            }
        } else {
            modbus_no_reply = 1;
        }
        if (modbus_no_reply == 1) {
            modbus_no_reply = 0;
            modbus_src = 0;
            return;
        }
        if (modbus_reply[1] == MB_WRITE_SINGLE || modbus_reply[1] == MB_WRITE_MULTIPLE) {
            modbus_reply_len = 0x12;
        } else {
            modbus_reply_len = modbus_reply[15] * 2 + 0x12;
            if (modbus_reply_len > 0x96) {
                modbus_reply_len = 0x96;
            }
        }
        if (modbus_err_reply == 1) {
            modbus_reply_len = 0x12;
            modbus_err_reply = 0;
        }
        for (i = 0; i < modbus_reply_len; i++) {
            modbus_tx_buf[i] = modbus_reply[i];
        }
        if (modbus_src == 1) {
            modbus_usb_wait = 0x14;
            do {
            } while (modbus_usb_wait != 0);
            CDC_SendData(&modbus_tx_buf[0], modbus_reply_len);
        } else {
            if (modbus_src == 2) {
                rs485_modbus_de_set(1);
                os_task_delay((uchar)os_task_dly_idx, 3, 5);
                uart_tx_start(2, &modbus_tx_buf[0], modbus_reply_len);
            }
        }
    }
    modbus_src = 0;
    return;
}

/* Task 3 (priority 3), every 5 ticks: assembles RS485 requests from USART3. Byte 0 must be HR30,
   1-8 or 0x30-0x36; byte 1 a function 3, 4, 6, 16, 17 or 22; the length is 8, or 8 + 2 x count
   for FC16 (at most 300). A complete request is handled at once when the slave is idle. 30
   passes without a byte reset the receiver. */
void task_modbus_rs485() {

    rs485_modbus_de_set(0);
    uart_rx_reset(2, 0x96);
    while (1) {
        while (uart_rx_getc(2, &modbus_rx_byte) == 0) {
            switch (modbus_rx_idx) {
            case 0:
                if (cfg_settings.modbus_address == modbus_rx_byte ||
                    modbus_rx_byte >= 1 && modbus_rx_byte <= 8 ||
                    modbus_rx_byte >= 0x30 && modbus_rx_byte <= 0x36) {
                    modbus_rx_buf[modbus_rx_idx] = modbus_rx_byte;
                    modbus_rx_idx = modbus_rx_idx + 1;
                } else {
                    modbus_rx_idx = 0;
                }
                break;
            case 1:
                if (modbus_rx_byte == 3 || modbus_rx_byte == 4 || modbus_rx_byte == 6 ||
                    modbus_rx_byte == 0x16 || modbus_rx_byte == 0x10 || modbus_rx_byte == 0x11) {
                    modbus_rx_buf[modbus_rx_idx] = modbus_rx_byte;
                    modbus_rx_idx = modbus_rx_idx + 1;
                } else {
                    modbus_rx_idx = 0;
                }
                break;
            default:
                if (modbus_rx_buf[1] == 0x10) {
                    modbus_req_len = modbus_rx_buf[5] * 2 + 8;
                    if (modbus_req_len > 0x12c) {
                        modbus_req_len = 0x12c;
                    }
                } else {
                    modbus_req_len = 8;
                }
                modbus_rx_buf[modbus_rx_idx] = modbus_rx_byte;
                modbus_rx_idx = modbus_rx_idx + 1;
                if (modbus_rx_idx >= modbus_req_len) {
                    if (modbus_src == 0) {
                        modbus_src = 2;
                        modbus_slave_process(&modbus_rx_buf[0]);
                    }
                    modbus_rx_idx = 0;
                }
            }
            modbus_rx_idle = 0;
        }
        if (modbus_rx_idle >= 0x1e) {
            modbus_rx_idx = 0;
            uart_rx_reset(2, 0x96);
            modbus_rx_idle = 0;
        } else {
            modbus_rx_idle = modbus_rx_idle + 1;
        }
        task_tbl[TASK_MODBUS].count = 0;
        task_tbl[TASK_MODBUS].period = 5;
        os_flags_pend(0);
    }
}
