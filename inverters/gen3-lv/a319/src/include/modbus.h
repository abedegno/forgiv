/* modbus.h: the Modbus RTU slave (comm/modbus_slave.c, on USART3 and USB CDC): function codes,
   holding register numbers, the request and reply buffers and the HR0-59 and IR0-59 images
   (built by comm/meter.c). The register map is in the header of comm/modbus_slave.c.
   name: descriptive (readability step 1). */
#ifndef MODBUS_H
#define MODBUS_H

#include "firmware_types.h"

/* Modbus function codes modbus_slave_process answers */
#define MB_READ_HOLDING 3
#define MB_READ_INPUT 4
#define MB_WRITE_SINGLE 6
#define MB_WRITE_MULTIPLE 16
#define MB_READ_METER 22 /* A316's own: modbus_fc22_read_meter */

/* holding register numbers with a public name (the GivEnergy register list); the fields of
   settings_t say where each is kept */
#define HR_INVERTER_MODULE_H 1
#define HR_INVERTER_MODULE_L 2
#define HR_ENABLE_AMMETER 7
#define HR_ENABLE_CHARGE_TARGET 20
#define HR_GRID_PORT_MAX_POWER 26
#define HR_BATTERY_POWER_MODE 27
#define HR_ENABLE_60HZ_FREQ_MODE 28
#define HR_SOC_FORCE_ADJUST 29 /* the calibration stage bits */
#define HR_MODBUS_ADDRESS 30
#define HR_USER_CODE 33
#define HR_ENABLE_DRM_RJ45 41
#define HR_ENABLE_REVERSE_CT_CLAMP 42
#define HR_CHARGE_SOC 43 /* served from settings +0xA2 (3 after an HR48/49 change) */
#define HR_DISCHARGE_SLOT_2_START 44
#define HR_DISCHARGE_SLOT_2_END 45
#define HR_METER_TYPE 47
#define HR_REVERSE_115_METER_DIR 48
#define HR_REVERSE_418_METER_DIR 49
#define HR_ACTIVE_POWER_RATE 50
#define HR_REACTIVE_POWER_RATE 51
#define HR_POWER_FACTOR 52
#define HR_INVERTER_STATE 53
#define HR_BATTERY_TYPE 54
#define HR_BATTERY_CAPACITY 55
#define HR_DISCHARGE_SLOT_1_START 56
#define HR_DISCHARGE_SLOT_1_END 57
#define HR_ENABLE_AUTO_JUDGE_BATTERY_TYPE 58
#define HR_ENABLE_DISCHARGE 59
#define HR_V_PV_INPUT_START 60
#define HR_INVERTER_START_TIME 61
#define HR_INVERTER_RESTART_DELAY 62
#define HR_V_AC_LOW_OUT 63
#define HR_V_AC_HIGH_OUT 64
#define HR_F_AC_LOW_OUT 65
#define HR_F_AC_HIGH_OUT 66
#define HR_V_AC_LOW_OUT_TIME 67
#define HR_V_AC_HIGH_OUT_TIME 68
#define HR_F_AC_LOW_OUT_TIME 69
#define HR_F_AC_HIGH_OUT_TIME 70
#define HR_V_AC_LOW_IN 71
#define HR_V_AC_HIGH_IN 72
#define HR_F_AC_LOW_IN 73
#define HR_F_AC_HIGH_IN 74
#define HR_V_AC_LOW_IN_TIME 75
#define HR_V_AC_HIGH_IN_TIME 76
#define HR_F_AC_LOW_IN_TIME 77
#define HR_F_AC_HIGH_IN_TIME 78
#define HR_V_AC_LOW_C 79
#define HR_V_AC_HIGH_C 80
#define HR_F_AC_LOW_C 81
#define HR_F_AC_HIGH_C 82
#define HR_V_10_MIN_PROTECTION 83
#define HR_CHARGE_SLOT_1_START 94
#define HR_CHARGE_SLOT_1_END 95
#define HR_ENABLE_CHARGE 96
#define HR_BATTERY_LOW_VOLTAGE_LIMIT 97
#define HR_BATTERY_HIGH_VOLTAGE_LIMIT 98
#define HR_BATTERY_SOC_RESERVE 110
#define HR_BATTERY_CHARGE_LIMIT 111
#define HR_BATTERY_DISCHARGE_LIMIT 112
#define HR_BATTERY_DISCHARGE_MIN_POWER_RESERVE                                                     \
    114 /* also the lithium night-recovery and LED
                                                       reserve SOC */
#define HR_CHARGE_TARGET_SOC 116

/* data */
extern uchar dsp_reserved_state[];
extern int meter_swap_a;
extern int meter_swap_b;
extern ushort meter_dir_changed;
extern ushort adc_rms_ch0;
extern ushort modbus_ir243_value;
extern uchar hr31_value;
extern ushort i_rms_ch5;
extern ushort fault_ac_readback;
extern uchar rtc_set_pending;
extern uint p_pv_total_calc;
extern ushort eeprom_save_delay;
extern uchar modbus_unused_records[];
extern uchar meter_in_day_start;
extern ushort hr_regs[60]; /* HR0-59 as served (modbus_hr_update fills it) */
extern ushort ir_regs[60]; /* IR0-59 as served (modbus_ir_update fills it) */
extern ushort modbus_batt_index;
extern ushort modbus_crc;
extern uchar modbus_err_reply;
extern ushort modbus_fc22_index;
extern ushort modbus_fc22_slave;
extern ushort modbus_ir_slave;
extern ushort modbus_meter_index;
extern uchar modbus_no_reply;
extern uchar modbus_reg_buf[]; /* one 60-register window, big-endian */
extern uchar modbus_reply[];   /* 16-byte header (slave, fc, serial, start, count), data, CRC */
extern uchar modbus_reply_len;
extern ushort modbus_req_len;
extern uchar modbus_rx_buf[];
extern uchar modbus_rx_byte;
extern uchar modbus_rx_idle;
extern ushort modbus_rx_idx;
extern uchar modbus_src; /* 0 idle, 1 USB CDC, 2 RS485 */
extern uchar modbus_tx_buf[];
extern ushort modbus_usb_wait; /* SysTick clears it at the next tick */
extern ushort peek_addr;       /* HR24 */
extern uchar peek_chip[];      /* HR23: 0 DSP, 1 ARM */

/* functions */
ushort crc16_modbus(byte *data, ushort len);
void modbus_fc03_read_holding(uchar *req);
void modbus_fc04_read_input(uchar *req);
void modbus_fc06_write_reg(uchar *req);
void modbus_fc16_write_regs(int unused_start, int unused_count, uchar *reply, uchar *req);
void modbus_fc22_read_meter(uchar *req);
void modbus_fill_battery_regs(uchar n);
void modbus_fill_bcu_regs();
void modbus_fill_meter_ir(uchar idx);
void modbus_fill_meter_regs(uchar n);
void modbus_fill_slave30_regs();
void modbus_hr_update();
void modbus_ir_update();
void modbus_regs_update();
void modbus_slave_process(uchar *req);
void systick_disable();
void systick_int_disable();
void task_modbus_rs485();

#endif
