// clang-format off
/* at: be_bytes_to_float 0x0802AFD4, bin_to_bcd 0x0802AFF2 */
/* at: meter_poll 0x0802B028, meter_parse_fc04 0x0802C0D6 */
/* at: meter_poll_inflight 0x2000046A, meter_power_fresh 0x20000473, meter_silence 0x2000047C, cfg_settings 0x20001AAE, energy_totals 0x20001BC5, dev11_data 0x20002028, meter_tx_buf 0x20003B4E, meter_data 0x20003C7A */
/* at: meter_parse_fc03 0x0802C94C, dev11_power_cmd_update 0x0802CA80, meter_rx_frame 0x0802CAF8, dev11_parse_fc03 0x0802CBC0, modbus_hr_update 0x0802CE2E, modbus_ir_update 0x0802D0EA, modbus_regs_update 0x0802D412, task_meter 0x0802D41E */
/* at: fault_code_word 0x20000120, inv_countdown 0x2000018E, dsp_hr4_hr5 0x20000190, num_mppt_phases 0x200001A0, hr31_value 0x200001AC, hr32_value 0x200001AD, inv_run_mode 0x200001AE, grid_current 0x200001B8, bus_p_voltage 0x200001DC, v_n_bus 0x200001DE, pv1_voltage 0x200001E0, pv2_voltage 0x200001E4, pv1_current 0x200001EC, pv2_current 0x200001F0, pv1_power 0x200001F4, pv2_power 0x200001F8, dsp_version_hr19 0x20000240, rtc_time 0x2000028F, sys_mode 0x200002D2, charge_status 0x200002D5, dsp_warning_word 0x200002D6, charge_active 0x200002D8, discharge_active 0x200002D9, batt_voltage 0x200002DC, batt_current 0x200002DE, batt_soc 0x200002E2, batt_capacity_bms 0x200002E4, bus_hv_voltage 0x200002E8, eps_voltage 0x200002EA, eps_frequency 0x200002EC, usb_device_inserted 0x200002EE, peek_chip 0x200002EF, peek_addr 0x200002F2, peek_value 0x200002F4, charger_temp 0x200002FC, heatsink_temp 0x20000300, batt_temp 0x20000304, p_inverter_out 0x20000320, p_battery 0x20000324, p_eps_backup 0x20000328, p_grid_out 0x2000032C, e_inverter_in_day 0x2000034C, e_inverter_out_day 0x20000350, e_inverter_in_total 0x20000354, e_inverter_out_total 0x20000358, p_load_demand 0x2000035C, v_ac1 0x20000360, f_ac1 0x20000362, i_grid_port 0x20000364, pf_inverter_output 0x20000374, p_grid_apparent 0x20000378, e_grid_in_day 0x2000037C, e_grid_out_day 0x20000380, e_grid_in_total 0x20000384, e_grid_out_total 0x20000388, board_init_done 0x20000396, bms_chip_version 0x2000039A, bms_fw_version 0x200003A2, e_pv_meter2_total 0x200003B0, e_pv_meter2_day 0x200003B4, e_pv_meter3_total 0x200003B8, e_pv_meter3_day 0x200003BC, meter_poll_inflight 0x2000046A, g_20000474 0x20000474, g_20000475 0x20000475, meter_silence 0x2000047C, meter_poll_timer 0x20000482, meter_fc03_addr 0x20000489, meter_rx_crc 0x2000048A, dev11_rx_crc 0x2000048C, meter_rx_byte 0x2000048E, meter_task_div 0x2000048F, meter_rx_idle 0x20000490, ir_regs 0x2000060A, hr_regs 0x20000682, cfg_settings 0x20001AAE, energy_totals 0x20001BC5, cfg_block6 0x20001C82, dev11_data 0x20002028, bms_serial_bytes 0x20002360, meter_rx_buf 0x20003AB8, meter_data 0x20003C7A, task_tbl 0x20007C6C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* meter.c: the RS485 Modbus master on USART2 (9600; 8N1 for HR47 = 1, 8E1 for HR47 = 0) that
   polls up to eight energy meters at addresses 1-8 and one more device at 0x11, and the code
   that builds the Modbus register images HR0-59 and IR0-59 from the live values.

   Entry points: task_meter (task 4, every 5 ticks); modbus_regs_update (every fifth pass).
   Dead: bin_to_bcd.
   Data: meter_data[8] (meter_t, meter.h), dev11_data (the 0x11 device's registers and its
   command), the poll state (meter_poll_step, meter_poll_addr, meter_poll_inflight, timers),
   meter_rx_buf, meter_tx_buf.
   Neighbours: drivers/uart.c (port 1), board.c (rs485_meter_de_set), control/monitor.c
   (power_flow_update turns meter_data into grid, PV and load power and energy), the Modbus slave
   (serves hr_regs, ir_regs and meter_data).

   Meters: HR7 is a bit mask of which addresses exist. Inference from power_flow_update: meter 1
   is the grid connection, meters 2 and 3 are AC-coupled PV, meter 4 a load, meters 5-8 further
   sources that are subtracted from the grid power. HR47 = 1 means the "115" meter type (values
   x10), 0 the "418" type (power x10000, so the meter reports kW). HR48/HR49 reverse single
   meters per type. A meter that has not answered for 2000 passes sets the meter fault
   (board_init_done = 1; IR0 then reads 2 and the meter values are cleared).

   The 0x11 device speaks GivEnergy's own framing (the FC3 reply has a 16-byte header) and takes
   its CRC high byte first. Each cycle the ARM writes a power command to its register 0xE7 and
   reads 38 registers from 0xCA (served as HR202-231). Inference: an export limiter, diverter or
   charger.

   name: descriptive (map/filenames.tsv). Three parts joined, the joined file compiles to the
   same bytes. */
#include "../../include/ghidra_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/meter.h"
#include "../../include/rtc.h"
#include "../../include/board.h"
#include "../../include/energy.h"
#include "../../include/modbus.h"
#include "../../include/settings.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"
#include "../../include/battery.h"
#include "../../include/dsp_link.h"
#include "../../include/faults.h"
#include "../../include/ff.h"
#include "../../include/measure.h"
#include "../../include/schedule.h"
#include "../../include/sys.h"
#include "../../include/usb.h"

/* Reads n bytes at p as a big-endian value into a float (used for the meters' IEEE 754
   registers). */
float be_bytes_to_float(uchar *data, int len) {
    int i;
    float r;
    union {
        uchar b[4];
        float f;
    } u;
    for (i = 0; i < len; i++) {
        u.b[i] = data[len - i - 1];
    }
    r = u.f;
    return r;
}

/* Dead. Meant to turn a number into packed BCD; returns only the last pair of digits it
   converted. */
uchar bin_to_bcd(int value, int digits) {
    uchar bcd_byte;
    int remainder;
    int digit_idx;

    digit_idx = digits - 1;
    while (digit_idx >= 0) {
        remainder = value % 0x64;
        bcd_byte = remainder % 0xa & 0xf | remainder / 0xa << 4;
        value = value / 0x64;
        digit_idx--;
    }
    return bcd_byte;
}

/* Sends the next meter request, one every 40 passes (about 200 ms, inference from 5-tick
   passes), for the current address while its bit is set in HR7:

     step 0-8   FC4, 2 registers each at 0x10, 0x4E, 0x52, 0x92, 0xD2, 0x112, 0x152, 0x160,
                0x166 (voltage, current, frequency, active power, +0x0A, apparent power,
                PF, import and export energy; meter_parse_fc04)
     step 9-15  2 registers at 0xFF00, 0xFF02, 0xFF04, 0xFF05, 0xFF06, 0x524, 0x525 with FC4
                (HR47 = 0) or FC3 (HR47 = 1): raw words
     step 16    to 0x11: FC6 register 0xE7 = the pending power command
     step 17    to 0x11: FC3 38 registers from 0xCA
     step 18    next address; after a full round meter_cycle_done = 1

   While a grid-side meter (1, 5, 6, 7 or 8) is present, every regular step is followed by
   active-power reads of those meters (steps 100-104), so the grid power stays fresh. When no
   meter has answered for 2000 passes it sets board_init_done = 1 (the meter fault) and starts
   again at address 1. With HR7 = 0 it clears that flag on every pass. */
void meter_poll() {
    ushort reg = 0;
    ushort crc = 0;
    ushort regs[17];

    regs[0] = 0x10;
    regs[1] = 0x4e;
    regs[2] = 0x52;
    regs[3] = 0x92;
    regs[4] = 0xd2;
    regs[5] = 0x112;
    regs[6] = 0x152;
    regs[7] = 0x160;
    regs[8] = 0x166;
    regs[9] = 0x702;
    regs[10] = 0xff00;
    regs[11] = 0xff02;
    regs[12] = 0xff04;
    regs[13] = 0xff05;
    regs[14] = 0xff06;
    regs[15] = 0x524;
    regs[16] = 0x525;
    if (cfg_settings.meter_type == 0) {
        g_20000487[0] = 4;
    } else {
        g_20000487[0] = 3;
    }
    if (cfg_settings.enable_ammeter == 0) {
        meter_silence = 0;
        board_init_done = 0;
        meter_poll_addr[0] = 1;
    } else {
        meter_enable_mask = cfg_settings.enable_ammeter;
        if (meter_silence < 2000) {
            meter_silence = meter_silence + 1;
        } else {
            board_init_done = 1;
            meter_poll_addr[0] = 1;
            g_20000484 = 0;
        }
        if ((meter_enable_mask & 1) != 0 && meter_poll_addr[0] == 1 ||
            (meter_enable_mask & 2) == 2 && meter_poll_addr[0] == 2 ||
            (meter_enable_mask & 4) == 4 && meter_poll_addr[0] == 3 ||
            (meter_enable_mask & 8) == 8 && meter_poll_addr[0] == 4 ||
            (meter_enable_mask & 0x10) == 0x10 && meter_poll_addr[0] == 5 ||
            (meter_enable_mask & 0x20) == 0x20 && meter_poll_addr[0] == 6 ||
            (meter_enable_mask & 0x40) == 0x40 && meter_poll_addr[0] == 7 ||
            (meter_enable_mask & 0x80) == 0x80 && meter_poll_addr[0] == 8) {
            if (meter_poll_timer < 40) {
                meter_poll_timer = meter_poll_timer + 1;
            } else {
                meter_poll_timer = 0;
                if (meter_poll_step < 19) {
                    meter_poll_resume = meter_poll_step;
                }
                if (meter_poll_step == meter_poll_inflight) {
                    g_20000486[0] = g_20000486[0] + 1;
                    if (g_20000486[0] >= 10) {
                        g_20000484 = 0;
                    }
                } else {
                    g_20000486[0] = 0;
                }
                meter_poll_inflight = meter_poll_step;
                switch (meter_poll_step) {
                case 0:
                    reg = regs[0];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 1:
                    reg = regs[1];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 2:
                    reg = regs[2];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 3:
                    reg = regs[3];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 4:
                    reg = regs[4];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 5:
                    reg = regs[5];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 6:
                    reg = regs[6];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 7:
                    reg = regs[7];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 8:
                    reg = regs[8];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 9:
                    reg = regs[10];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 10:
                    reg = regs[11];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 11:
                    reg = regs[12];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 1;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 12:
                    reg = regs[13];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 1;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 13:
                    reg = regs[14];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 1;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 14:
                    reg = regs[15];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 1;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 15:
                    reg = regs[16];
                    meter_tx_buf[0] = meter_poll_addr[0];
                    meter_tx_buf[1] = g_20000487[0];
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 1;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 1) != 0 || (meter_enable_mask & 0x10) == 0x10 ||
                        (meter_enable_mask & 0x20) == 0x20 || (meter_enable_mask & 0x40) == 0x40 ||
                        (meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 100;
                    } else {
                        meter_poll_step = meter_poll_step + 1;
                    }
                    break;
                case 16:
                    if (dev11_data.cmd_pending == 1) {
                        meter_tx_buf[0] = 0x11;
                        meter_tx_buf[1] = 6;
                        meter_tx_buf[2] = 0;
                        meter_tx_buf[3] = 0xe7;
                        meter_tx_buf[4] = (uint)dev11_data.cmd >> 8;
                        meter_tx_buf[5] = dev11_data.cmd;
                        crc = crc16_modbus((int)meter_tx_buf, 6);
                        meter_tx_buf[6] = crc >> 8;
                        meter_tx_buf[7] = crc;
                        rs485_meter_de_set(1);
                        os_task_delay((uchar)os_task_dly_idx, 3, 5);
                        uart_tx_start(1, (int)meter_tx_buf, 8);
                        dev11_data.cmd_pending = 0;
                    }
                    meter_poll_step = meter_poll_step + 1;
                    break;
                case 17:
                    meter_tx_buf[0] = 0x11;
                    meter_tx_buf[1] = 3;
                    meter_tx_buf[2] = 0;
                    meter_tx_buf[3] = 0xca;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 0x26;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc >> 8;
                    meter_tx_buf[7] = crc;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    meter_poll_step = meter_poll_step + 1;
                    break;
                case 18:
                    if ((meter_enable_mask & 0x2) == 0x2 && meter_poll_addr[0] == 2) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x4) == 0x4 && meter_poll_addr[0] == 3) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x8) == 0x8 && meter_poll_addr[0] == 4) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x10) == 0x10 && meter_poll_addr[0] == 5) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x20) == 0x20 && meter_poll_addr[0] == 6) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x40) == 0x40 && meter_poll_addr[0] == 7) {
                        meter_poll_step = 0;
                    } else if ((meter_enable_mask & 0x80) == 0x80 && meter_poll_addr[0] == 8) {
                        meter_poll_step = 0;
                    } else {
                        meter_poll_step = 0;
                        meter_cycle_done = 1;
                        g_20000484 = 0;
                    }
                    meter_poll_addr[0] = meter_poll_addr[0] + 1;
                    if (meter_poll_addr[0] > 8) {
                        meter_poll_addr[0] = 1;
                    }
                    break;
                case 100:
                    reg = regs[3];
                    meter_tx_buf[0] = 1;
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 0x10) == 0x10) {
                        meter_poll_step = 101;
                    } else if ((meter_enable_mask & 0x20) == 0x20) {
                        meter_poll_step = 102;
                    } else if ((meter_enable_mask & 0x40) == 0x40) {
                        meter_poll_step = 103;
                    } else if ((meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 104;
                    } else {
                        meter_poll_step = meter_poll_resume + 1;
                    }
                    break;
                case 101:
                    reg = regs[3];
                    meter_tx_buf[0] = 5;
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 0x20) == 0x20) {
                        meter_poll_step = 102;
                    } else if ((meter_enable_mask & 0x40) == 0x40) {
                        meter_poll_step = 103;
                    } else if ((meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 104;
                    } else {
                        meter_poll_step = meter_poll_resume + 1;
                    }
                    break;
                case 102:
                    reg = regs[3];
                    meter_tx_buf[0] = 6;
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 0x40) == 0x40) {
                        meter_poll_step = 103;
                    } else if ((meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 104;
                    } else {
                        meter_poll_step = meter_poll_resume + 1;
                    }
                    break;
                case 103:
                    reg = regs[3];
                    meter_tx_buf[0] = 7;
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    if ((meter_enable_mask & 0x80) == 0x80) {
                        meter_poll_step = 104;
                    } else {
                        meter_poll_step = meter_poll_resume + 1;
                    }
                    break;
                case 104:
                    reg = regs[3];
                    meter_tx_buf[0] = 8;
                    meter_tx_buf[1] = 4;
                    meter_tx_buf[2] = reg >> 8;
                    meter_tx_buf[3] = reg;
                    meter_tx_buf[4] = 0;
                    meter_tx_buf[5] = 2;
                    crc = crc16_modbus((int)meter_tx_buf, 6);
                    meter_tx_buf[6] = crc;
                    meter_tx_buf[7] = crc >> 8;
                    rs485_meter_de_set(1);
                    os_task_delay((uchar)os_task_dly_idx, 3, 5);
                    uart_tx_start(1, (int)meter_tx_buf, 8);
                    meter_poll_step = meter_poll_resume + 1;
                    break;
                default:
                    break;
                }
            }
        } else {
            meter_poll_addr[0] = meter_poll_addr[0] + 1;
            if (meter_poll_addr[0] > 8) {
                meter_poll_addr[0] = 1;
            }
        }
    }
}

// clang-format off
  /* at: __aeabi_fmul 0x080369A8, __aeabi_f2uiz 0x08036968, __aeabi_f2iz 0x08036930 */
// clang-format on
typedef __packed struct {
    ushort f0;
    ushort f2;
    ushort f4;
    int f6;
    uint fa;
    uint fe;
    short f12;
    int f14;
    int f18;
    int f1c;
    int f20;
    ushort f24;
    ushort f26;
    ushort f28;
    ushort f2a;
    ushort f2c;
} PACK_0802C0D6;
/* match: PK is meter_data with q_reactive unsigned (fa): the float converts with __aeabi_f2uiz;
   as meter_data[n].q_reactive (int) it would be __aeabi_f2iz */
#define PK ((PACK_0802C0D6 *)((uchar *)meter_data))

/* Takes a function 4 reply from meter 1-8 into meter_data[address - 1], by the step that asked
   for it (meter_poll_inflight). Floats are scaled to integers: voltage x10, current x100,
   frequency x100, power x10 (HR47 = 1) or x10000 (HR47 = 0), PF as 10000 - 10000 x PF, energies
   x10. A reversed meter (HR48/HR49 bit) gets its active power negated and its import and export
   energies swapped; its apparent power is multiplied by 4294967296.0, which saturates when it is
   converted to an integer (likely meant as a sign change; docs/findings.md). The first energy
   reading of the day is kept as the day's start value. */
void meter_parse_fc04(uchar *frame) {
    float v = 0;
    int raw;

    v = be_bytes_to_float(frame + 3, 4);
    meter_fc04_addr = *frame;
    if (meter_fc04_addr >= 1 && meter_fc04_addr <= 8) {
        meter_silence = 0;
        meter_fc04_addr = meter_fc04_addr - 1;
        switch (meter_poll_inflight) {
        case 0:
            meter_data[meter_fc04_addr].voltage = v * 10.0f;
            break;
        case 1:
            meter_data[meter_fc04_addr].current = v * 100.0f;
            break;
        case 2:
            meter_data[meter_fc04_addr].frequency = (int)(v * 100.0f);
            break;
        case 3:
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
            if (cfg_settings.meter_type == 1) {
                if ((cfg_settings.reverse_115_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].p_active = v * 10.0f * -1.0f;
                } else {
                    meter_data[meter_fc04_addr].p_active = v * 10.0f;
                }
            } else {
                if ((cfg_settings.reverse_418_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].p_active = v * 10000.0f * -1.0f;
                } else {
                    meter_data[meter_fc04_addr].p_active = v * 10000.0f;
                }
            }
            meter_power_fresh = 1;
            break;
        case 4:
            if (cfg_settings.meter_type == 1) {
                if ((cfg_settings.reverse_115_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    PK[meter_fc04_addr].fa = v * 10.0f;
                } else {
                    PK[meter_fc04_addr].fa = v * 10.0f;
                }
            } else {
                if ((cfg_settings.reverse_418_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    PK[meter_fc04_addr].fa = v * 10000.0f;
                } else {
                    PK[meter_fc04_addr].fa = v * 10000.0f;
                }
            }
            break;
        case 5:
            if (cfg_settings.meter_type == 1) {
                if ((cfg_settings.reverse_115_meter_dir & 1 << meter_fc04_addr) ==
                    (1 << meter_fc04_addr | 0)) {
                    meter_data[meter_fc04_addr].s_apparent = v * 10.0f * 4294967296.0f;
                } else {
                    meter_data[meter_fc04_addr].s_apparent = v * 10.0f;
                }
            } else {
                if ((cfg_settings.reverse_418_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].s_apparent = v * 10000.0f * 4294967296.0f;
                } else {
                    meter_data[meter_fc04_addr].s_apparent = v * 10000.0f;
                }
            }
            break;
        case 6:
            meter_data[meter_fc04_addr].pf = 10000 - (int)(v * 10000.0f);
            break;
        case 7:
            if (cfg_settings.meter_type == 1) {
                if ((cfg_settings.reverse_115_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].e_export = v * 10.0f;
                    if (energy_totals.meter_out_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_out_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_export;
                    }
                } else {
                    meter_data[meter_fc04_addr].e_import = v * 10.0f;
                    if (energy_totals.meter_in_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_in_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_import;
                    }
                }
            } else {
                if ((cfg_settings.reverse_418_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].e_export = v * 10.0f;
                    if (energy_totals.meter_out_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_out_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_export;
                    }
                } else {
                    meter_data[meter_fc04_addr].e_import = v * 10.0f;
                    if (energy_totals.meter_in_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_in_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_import;
                    }
                }
            }
            break;
        case 8:
            if (cfg_settings.meter_type == 1) {
                if ((cfg_settings.reverse_115_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].e_import = v * 10.0f;
                    if (energy_totals.meter_in_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_in_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_import;
                    }
                } else {
                    meter_data[meter_fc04_addr].e_export = v * 10.0f;
                    if (energy_totals.meter_out_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_out_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_export;
                    }
                }
            } else {
                if ((cfg_settings.reverse_418_meter_dir & 1 << meter_fc04_addr) ==
                    1 << meter_fc04_addr) {
                    meter_data[meter_fc04_addr].e_import = v * 10.0f;
                    if (energy_totals.meter_in_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_in_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_import;
                    }
                } else {
                    meter_data[meter_fc04_addr].e_export = v * 10.0f;
                    if (energy_totals.meter_out_day_start[meter_fc04_addr] == 0) {
                        energy_totals.meter_out_day_start[meter_fc04_addr] =
                            meter_data[meter_fc04_addr].e_export;
                    }
                }
            }
            break;
        case 9:
            raw = (frame[3] << 24) + (frame[4] << 16) + (frame[5] << 8) + frame[6];
            meter_data[meter_fc04_addr].f1C = raw;
            break;
        case 10:
            raw = (frame[3] << 24) + (frame[4] << 16) + (frame[5] << 8) + frame[6];
            meter_data[meter_fc04_addr].f20 = raw;
            break;
        case 11:
            raw = frame[4] + (frame[3] << 8);
            meter_data[meter_fc04_addr].f24 = raw;
            break;
        case 12:
            raw = frame[4] + (frame[3] << 8);
            meter_data[meter_fc04_addr].f26 = raw;
            break;
        case 13:
            raw = frame[4] + (frame[3] << 8);
            meter_data[meter_fc04_addr].f28 = raw;
            break;
        case 14:
            raw = frame[4] + (frame[3] << 8);
            meter_data[meter_fc04_addr].f2A = raw;
            break;
        case 15:
            raw = frame[4] + (frame[3] << 8);
            meter_data[meter_fc04_addr].f2C = raw;
            break;
        default:
            break;
        }
    }
}
#undef PK

/* Takes a function 3 reply (steps 9-15, HR47 = 1) into the raw words of meter_data. */
void meter_parse_fc03(uchar *frame) {
    int v = 0;

    meter_fc03_addr = frame[0];
    if (meter_fc03_addr >= 1 && meter_fc03_addr <= 8) {
        meter_silence = 0;
        meter_fc03_addr = meter_fc03_addr - 1;
        switch (meter_poll_inflight) {
        case 9:
            v = (frame[3] << 24) + (frame[4] << 16) + (frame[5] << 8) + frame[6];
            meter_data[meter_fc03_addr].f1C = v;
            break;
        case 10:
            v = (frame[3] << 24) + (frame[4] << 16) + (frame[5] << 8) + frame[6];
            meter_data[meter_fc03_addr].f20 = v;
            break;
        case 11:
            v = frame[4] + (frame[3] << 8);
            meter_data[meter_fc03_addr].f24 = v;
            break;
        case 12:
            v = frame[4] + (frame[3] << 8);
            meter_data[meter_fc03_addr].f26 = v;
            break;
        case 13:
            v = frame[4] + (frame[3] << 8);
            meter_data[meter_fc03_addr].f28 = v;
            break;
        case 14:
            v = frame[4] + (frame[3] << 8);
            meter_data[meter_fc03_addr].f2A = v;
            break;
        case 15:
            v = frame[4] + (frame[3] << 8);
            meter_data[meter_fc03_addr].f2C = v;
            break;
        default:
            break;
        }
    }
}

/* Chooses the power command for the 0x11 device: 36000 while HR230 = 1 (the block 6 default);
   else, when HR27 = 1, grid export above 50 W (p_grid_out > 500), no charge or discharge slot
   active and a fresh reply from the device, the export power itself; else 0. In every case it
   marks the command to be sent. */
void dev11_power_cmd_update() {

    if (cfg_block6.hr230 == 1) {
        dev11_data.cmd = 0x8ca0;
        dev11_data.cmd_pending = 1;
    } else {
        if (cfg_settings.battery_power_mode == 1 && p_grid_out > 0x1f4 &&
            dev11_data.cmd_pending != 1 && discharge_active == 0 && charge_active == 0 &&
            dev11_data.fresh == 1) {
            dev11_data.cmd = (ushort)p_grid_out;
            dev11_data.cmd_pending = 1;
            dev11_data.fresh = 0;
        } else {
            dev11_data.cmd = 0;
            dev11_data.cmd_pending = 1;
            dev11_data.fresh = 0;
        }
    }
}

/* Checks the CRC of a meter reply (either byte order is accepted) and hands it to the FC3 or FC4
   parser. A good reply moves board_init_done from 1 (meter fault) to 2 and restarts the
   40-pass poll timer. */
void meter_rx_frame(uchar *frame) {
    int frame_valid = 0;

    meter_rx_crc = crc16_modbus(frame, (ushort)(meter_rx_expect - 2));
    if ((uchar)meter_rx_crc == *(meter_rx_expect + frame - 2) &&
        *(meter_rx_expect + frame - 1) == meter_rx_crc >> 8) {
        frame_valid = 1;
    } else {
        if ((uchar)meter_rx_crc == *(meter_rx_expect + frame - 1) &&
            *(meter_rx_expect + frame - 2) == meter_rx_crc >> 8) {
            frame_valid = 1;
        } else {
            frame_valid = 0;
        }
    }
    if (frame_valid == 1) {
        if (frame[1] >= 3 && frame[1] <= 16) {
            switch (frame[1]) {
            case 3:
                meter_parse_fc03(frame);
                break;
            case 4:
                meter_parse_fc04(frame);
                break;
            case 6:
                break;
            case 16:
                break;
            default:
                break;
            }
        }
        if (board_init_done == 1) {
            board_init_done = 2;
        }
        meter_poll_timer = 0;
        g_20000474 = 0;
        g_20000475 = 0;
    }
}

/* Checks the CRC of the 0x11 device's FC3 reply (either order) and copies its registers from
   byte 16 on into dev11_data, then marks it fresh (+0x36). */
void dev11_parse_fc03(uchar *frame) {
    int frame_valid = 0;

    dev11_rx_crc = crc16_modbus(frame, (ushort)(meter_rx_expect - 2));
    if ((uchar)dev11_rx_crc == *(meter_rx_expect + frame - 2) &&
        *(meter_rx_expect + frame - 1) == dev11_rx_crc >> 8) {
        frame_valid = 1;
    } else {
        if ((uchar)dev11_rx_crc == *(meter_rx_expect + frame - 1) &&
            *(meter_rx_expect + frame - 2) == dev11_rx_crc >> 8) {
            frame_valid = 1;
        } else {
            frame_valid = 0;
        }
    }
    if (frame_valid == 1) {
        dev11_data.h00 = meter_rx_buf[0x11] & 0xff | meter_rx_buf[0x10] << 8;
        dev11_data.h02 = meter_rx_buf[0x13] & 0xff | meter_rx_buf[0x12] << 8;
        dev11_data.h04 = meter_rx_buf[0x15] & 0xff | meter_rx_buf[0x14] << 8;
        dev11_data.h06 = meter_rx_buf[0x17] & 0xff | meter_rx_buf[0x16] << 8;
        dev11_data.h08 = meter_rx_buf[0x19] & 0xff | meter_rx_buf[0x18] << 8;
        dev11_data.h0a = meter_rx_buf[0x1b] & 0xff | meter_rx_buf[0x1a] << 8;
        dev11_data.h0c = meter_rx_buf[0x1d] & 0xff | meter_rx_buf[0x1c] << 8;
        dev11_data.h0e = meter_rx_buf[0x1f] & 0xff | meter_rx_buf[0x1e] << 8;
        dev11_data.h10 = meter_rx_buf[0x21] & 0xff | meter_rx_buf[0x20] << 8;
        dev11_data.h12 = meter_rx_buf[0x23] & 0xff | meter_rx_buf[0x22] << 8;
        dev11_data.h14 = meter_rx_buf[0x25] & 0xff | meter_rx_buf[0x24] << 8;
        dev11_data.h16 = meter_rx_buf[0x27] & 0xff | meter_rx_buf[0x26] << 8;
        dev11_data.b18[0] = meter_rx_buf[0x28];
        dev11_data.b18[1] = meter_rx_buf[0x29];
        dev11_data.b18[2] = meter_rx_buf[0x2a];
        dev11_data.b18[3] = meter_rx_buf[0x2b];
        dev11_data.b18[4] = meter_rx_buf[0x2c];
        dev11_data.b18[5] = meter_rx_buf[0x2d];
        dev11_data.b18[6] = meter_rx_buf[0x2e];
        dev11_data.b18[7] = meter_rx_buf[0x2f];
        dev11_data.b18[8] = meter_rx_buf[0x30];
        dev11_data.b18[9] = meter_rx_buf[0x31];
        dev11_data.h22 = meter_rx_buf[0x33] & 0xff | meter_rx_buf[0x32] << 8;
        dev11_data.h24 = meter_rx_buf[0x35] & 0xff | meter_rx_buf[0x34] << 8;
        dev11_data.h26 = meter_rx_buf[0x37] & 0xff | meter_rx_buf[0x36] << 8;
        dev11_data.h28 = meter_rx_buf[0x39] & 0xff | meter_rx_buf[0x38] << 8;
        dev11_data.w30 = (meter_rx_buf[0x3a] << 24) + (meter_rx_buf[0x3b] << 16) +
                         (meter_rx_buf[0x3c] << 8) + (meter_rx_buf[0x3d] & 0xff);
        dev11_data.h34 = meter_rx_buf[0x4b] & 0xff | meter_rx_buf[0x4a] << 8;
        dev11_data.fresh = 1;
    }
}

/* Builds HR0-59 in hr_regs from the settings and live values. Fixed values: HR0 device type
   (0x2001, 0x2002 or 0x2003 for model bytes 0x32, 0x2E, 0x24), HR6 3600, HR21 0x13C (316, this
   ARM version), HR34 140. HR8-12 the DSP version text, HR13-17 the serial, HR18 the BMS firmware
   version, HR19 the DSP version, HR46 the BMS chip version, HR22 the USB device class, HR35-40
   the clock. HR55 shows the BMS's capacity when the DSP reports one, else the stored HR55. HR25
   is the
   peek: the DSP's answer, or the RAM halfword at 0x20000000 + HR24 for the ARM. */
void modbus_hr_update() {

    if ((uchar)cfg_settings.inverter_module_l == 0x32) {
        hr_regs[0] = 0x2001;
    } else {
        if ((uchar)cfg_settings.inverter_module_l == 0x2e) {
            hr_regs[0] = 0x2002;
        } else {
            if ((uchar)cfg_settings.inverter_module_l == 0x24) {
                hr_regs[0] = 0x2003;
            }
        }
    }
    hr_regs[HR_INVERTER_MODULE_H] = cfg_settings.inverter_module_h;
    hr_regs[HR_INVERTER_MODULE_L] = cfg_settings.inverter_module_l;
    hr_regs[3] = num_mppt_phases;
    hr_regs[4] = 0xffff & dsp_hr4_hr5 >> 0x10;
    hr_regs[5] = (ushort)dsp_hr4_hr5;
    hr_regs[6] = 0xe10;
    hr_regs[HR_ENABLE_AMMETER] = cfg_settings.enable_ammeter;
    hr_regs[8] = bms_serial_bytes[1] + (bms_serial_bytes[0] << 8);
    hr_regs[9] = bms_serial_bytes[3] + (bms_serial_bytes[2] << 8);
    hr_regs[10] = bms_serial_bytes[5] + (bms_serial_bytes[4] << 8);
    hr_regs[11] = bms_serial_bytes[7] + (bms_serial_bytes[6] << 8);
    hr_regs[12] = bms_serial_bytes[9] + (bms_serial_bytes[8] << 8);
    hr_regs[13] = cfg_settings.inverter_serial[0][1] + (cfg_settings.inverter_serial[0][0] << 8);
    hr_regs[14] = cfg_settings.inverter_serial[1][1] + cfg_settings.inverter_serial[1][0] * 0x100;
    hr_regs[15] = cfg_settings.inverter_serial[2][1] + cfg_settings.inverter_serial[2][0] * 0x100;
    hr_regs[16] = cfg_settings.inverter_serial[3][1] + cfg_settings.inverter_serial[3][0] * 0x100;
    hr_regs[17] = cfg_settings.inverter_serial[4][1] + cfg_settings.inverter_serial[4][0] * 0x100;
    hr_regs[18] = bms_fw_version;
    hr_regs[19] = dsp_version_hr19;
    hr_regs[HR_ENABLE_CHARGE_TARGET] = cfg_settings.enable_charge_target;
    hr_regs[21] = 0x13c;
    hr_regs[22] = usb_device_inserted;
    hr_regs[23] = peek_chip[0];
    hr_regs[24] = peek_addr;
    if (peek_chip[0] == 0) {
        hr_regs[25] = peek_value;
    } else {
        if ((peek_addr > 0 || peek_addr == 0) && peek_addr <= 0x10000) {
            hr_regs[25] = *(ushort *)(peek_addr + 0x20000000);
        } else {
            hr_regs[25] = 0;
        }
    }
    hr_regs[HR_GRID_PORT_MAX_POWER] = cfg_settings.grid_port_max_power;
    hr_regs[HR_BATTERY_POWER_MODE] = cfg_settings.battery_power_mode;
    hr_regs[HR_ENABLE_60HZ_FREQ_MODE] = cfg_settings.enable_60hz_freq_mode;
    hr_regs[HR_SOC_FORCE_ADJUST] = cfg_settings.soc_force_adjust;
    hr_regs[HR_MODBUS_ADDRESS] = cfg_settings.modbus_address;
    hr_regs[31] = hr31_value;
    hr_regs[32] = hr32_value;
    hr_regs[HR_USER_CODE] = cfg_settings.user_code;
    hr_regs[34] = 0x8c;
    hr_regs[35] = rtc_time.year;
    hr_regs[36] = rtc_time.month;
    hr_regs[37] = rtc_time.date;
    hr_regs[38] = rtc_time.hour;
    hr_regs[39] = rtc_time.min;
    hr_regs[40] = rtc_time.sec;
    hr_regs[HR_ENABLE_DRM_RJ45] = cfg_settings.enable_drm_rj45;
    hr_regs[HR_ENABLE_REVERSE_CT_CLAMP] = cfg_settings.enable_reverse_ct_clamp;
    hr_regs[HR_CHARGE_SOC] = cfg_settings.fA2;
    hr_regs[HR_DISCHARGE_SLOT_2_START] = cfg_settings.discharge_slot_2_start;
    hr_regs[HR_DISCHARGE_SLOT_2_END] = cfg_settings.discharge_slot_2_end;
    hr_regs[46] = bms_chip_version;
    hr_regs[HR_METER_TYPE] = cfg_settings.meter_type;
    hr_regs[HR_REVERSE_115_METER_DIR] = cfg_settings.reverse_115_meter_dir;
    hr_regs[HR_REVERSE_418_METER_DIR] = cfg_settings.reverse_418_meter_dir;
    hr_regs[HR_ACTIVE_POWER_RATE] = cfg_settings.active_power_rate;
    hr_regs[HR_REACTIVE_POWER_RATE] = cfg_settings.reactive_power_rate;
    hr_regs[HR_POWER_FACTOR] = cfg_settings.power_factor;
    hr_regs[HR_INVERTER_STATE] = cfg_settings.inverter_state | cfg_settings.f09 << 8;
    hr_regs[HR_BATTERY_TYPE] = cfg_settings.battery_type;
    if (batt_capacity_bms > 0) {
        hr_regs[HR_BATTERY_CAPACITY] = batt_capacity_bms;
    } else {
        hr_regs[HR_BATTERY_CAPACITY] = cfg_settings.battery_capacity;
    }
    hr_regs[HR_DISCHARGE_SLOT_1_START] = cfg_settings.discharge_slot_1_start;
    hr_regs[HR_DISCHARGE_SLOT_1_END] = cfg_settings.discharge_slot_1_end;
    hr_regs[HR_ENABLE_AUTO_JUDGE_BATTERY_TYPE] = cfg_settings.enable_auto_judge_battery_type;
    hr_regs[HR_ENABLE_DISCHARGE] = cfg_settings.enable_discharge;
}

/* Builds IR0-59 in ir_regs. IR0 is 2 while the meter fault is set (or charge status 6 with HR351
   = 0), else the run mode. Then PV, bus, grid and battery values, the energy counters, faults
   and temperatures, following the public GivEnergy input register list: IR50 battery voltage,
   IR51 current, IR52 power, IR56 battery temperature, IR57 the DSP warning word (1 while the
   meter fault is set), IR58 grid port current, IR59 SOC. IR6/7 add battery charge and discharge
   totals; IR47/48 is work_time in hours. */
void modbus_ir_update() {
    int pv_energy_total = 0;

    if (board_init_done >= 1) {
        ir_regs[0] = 2;
    } else {
        if (cfg_block6.hr351 == 0 && charge_status == 6) {
            ir_regs[0] = 2;
        } else {
            ir_regs[0] = inv_run_mode;
        }
    }
    ir_regs[1] = pv1_voltage;
    ir_regs[2] = pv2_voltage;
    ir_regs[3] = bus_p_voltage;
    ir_regs[4] = v_n_bus;
    ir_regs[5] = v_ac1;
    ir_regs[6] =
        0xffff & energy_totals.batt_discharge_total + energy_totals.batt_charge_total >> 0x10;
    ir_regs[7] =
        (ushort)energy_totals.batt_discharge_total + (ushort)energy_totals.batt_charge_total;
    ir_regs[8] = pv1_current / 0xa;
    ir_regs[9] = pv2_current / 0xa;
    ir_regs[10] = grid_current / 0xa;
    pv_energy_total = energy_totals.pv_total + e_pv_meter2_total + e_pv_meter3_total;
    ir_regs[11] = (uint)pv_energy_total >> 0x10;
    ir_regs[12] = pv_energy_total;
    ir_regs[13] = f_ac1;
    ir_regs[14] = charge_status;
    ir_regs[15] = bus_hv_voltage;
    ir_regs[16] = pf_inverter_output;
    ir_regs[17] = energy_totals.pv1_day + (ushort)e_pv_meter2_day;
    ir_regs[18] = (uint)pv1_power / 0xa;
    ir_regs[19] = energy_totals.pv2_day + (ushort)e_pv_meter3_day;
    ir_regs[20] = (uint)pv2_power / 0xa;
    ir_regs[21] = 0xffff & e_grid_out_total >> 0x10;
    ir_regs[22] = (ushort)e_grid_out_total;
    ir_regs[23] = 0;
    ir_regs[24] = p_inverter_out / 0xa;
    ir_regs[25] = (ushort)e_grid_out_day;
    ir_regs[26] = (ushort)e_grid_in_day;
    ir_regs[27] = 0xffff & e_inverter_in_total >> 0x10;
    ir_regs[28] = (ushort)e_inverter_in_total;
    ir_regs[29] = 0;
    ir_regs[30] = p_grid_out / 0xa;
    ir_regs[31] = p_eps_backup / 0xa;
    ir_regs[32] = 0xffff & e_grid_in_total >> 0x10;
    ir_regs[33] = (ushort)e_grid_in_total;
    ir_regs[34] = 0;
    ir_regs[35] = (ushort)e_inverter_in_day;
    ir_regs[36] = energy_totals.batt_charge_day;
    ir_regs[37] = energy_totals.batt_discharge_day;
    ir_regs[38] = inv_countdown;
    ir_regs[39] = (uint)fault_code_word >> 0x10;
    ir_regs[40] = (ushort)fault_code_word;
    ir_regs[41] = heatsink_temp;
    ir_regs[42] = p_load_demand / 0xa;
    ir_regs[43] = (uint)p_grid_apparent / 0xa;
    ir_regs[44] = (ushort)e_inverter_out_day;
    ir_regs[45] = 0xffff & e_inverter_out_total >> 0x10;
    ir_regs[46] = (ushort)e_inverter_out_total;
    ir_regs[47] = (uint)energy_totals.work_time / 0x1c20 >> 0x10;
    ir_regs[48] = (uint)energy_totals.work_time / 0x1c20;
    ir_regs[49] = sys_mode;
    ir_regs[50] = batt_voltage;
    ir_regs[51] = batt_current;
    ir_regs[52] = p_battery;
    ir_regs[53] = eps_voltage;
    ir_regs[54] = eps_frequency;
    ir_regs[55] = charger_temp;
    ir_regs[56] = batt_temp;
    ir_regs[57] = dsp_warning_word;
    if (board_init_done >= 1) {
        ir_regs[57] = 1;
    }
    ir_regs[58] = i_grid_port;
    ir_regs[59] = batt_soc;
}

/* Rebuilds both register images (HR0-59 and IR0-59). */
void modbus_regs_update() {

    modbus_hr_update();
    modbus_ir_update();
}

/* Task 4 (priority 4), every 5 ticks: refreshes the register images on every fifth pass, picks
   the 0x11 command, sends the next meter request and assembles replies from USART2. A reply
   starts with address 1-8 or 0x11 and function 3, 4, 6, 16 or 17; its length is byte 2 + 5 for
   meter reads, 8 for FC16, 94 for the 0x11 FC3 reply and 18 for its FC6 echo. 150 passes without
   a byte reset the receiver. A meter's byte count is not checked against the buffer (150 bytes):
   a count above 145 would run into meter_tx_buf. */
void task_meter() {

    rs485_meter_de_set(0);
    uart_rx_reset(1, 0x96);
    while (1) {
        meter_task_div = meter_task_div + 1;
        if (meter_task_div > 4) {
            meter_task_div = 0;
            modbus_regs_update();
        }
        dev11_power_cmd_update();
        meter_poll();
        while (uart_rx_getc(1, &meter_rx_byte) == 0) {
            switch (meter_rx_len) {
            case 0:
                if (meter_rx_byte >= 1 && meter_rx_byte <= 8 || meter_rx_byte == 0x11) {
                    meter_rx_buf[meter_rx_len] = meter_rx_byte;
                    meter_rx_len = meter_rx_len + 1;
                } else {
                    meter_rx_len = 0;
                }
                break;
            case 1:
                if (meter_rx_byte == 3 || meter_rx_byte == 4 || meter_rx_byte == 6 ||
                    meter_rx_byte == 0x10 || meter_rx_byte == 0x11) {
                    meter_rx_buf[meter_rx_len] = meter_rx_byte;
                    meter_rx_len = meter_rx_len + 1;
                } else {
                    meter_rx_len = 0;
                }
                break;
            default:
                if (meter_rx_buf[0] >= 1 && meter_rx_buf[0] <= 8) {
                    if (meter_rx_buf[1] == 0x10) {
                        meter_rx_expect = 8;
                    } else {
                        meter_rx_expect = meter_rx_buf[2] + 5;
                    }
                } else if (meter_rx_buf[0] == 0x11 && meter_rx_buf[1] == 3) {
                    meter_rx_expect = 0x5e;
                } else if (meter_rx_buf[0] == 0x11 && meter_rx_buf[1] == 6) {
                    meter_rx_expect = 0x12;
                } else {
                    meter_rx_expect = 8;
                }
                meter_rx_buf[meter_rx_len] = meter_rx_byte;
                meter_rx_len = meter_rx_len + 1;
                if (meter_rx_len >= meter_rx_expect) {
                    if (meter_rx_buf[0] >= 1 && meter_rx_buf[0] <= 8) {
                        meter_rx_frame(&meter_rx_buf[0]);
                    } else if (meter_rx_buf[0] == 0x11 && meter_rx_buf[1] == 3) {
                        dev11_parse_fc03(&meter_rx_buf[0]);
                    }
                    meter_rx_len = 0;
                }
                break;
            }
            meter_rx_idle = 0;
        }
        if (meter_rx_idle >= 0x96) {
            meter_rx_len = 0;
            meter_rx_idle = 0;
            uart_rx_reset(1, 0x96);
        } else {
            meter_rx_idle = meter_rx_idle + 1;
        }
        task_tbl[TASK_METER].count = 0;
        task_tbl[TASK_METER].period = 5;
        os_flags_pend(0);
    }
}
