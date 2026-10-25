/* meter.h: the RS485 Modbus master on USART2 (comm/meter.c): energy meters 1-8 and the device at
   address 0x11, the poll state, and the meter records the Modbus slave and power_flow_update
   use.
   name: descriptive (readability step 1). */
#ifndef METER_H
#define METER_H

#include "firmware_types.h"

/* Packed 144-byte meter record. Single-phase replies populate the first phase;
   the extended protocol supplies all three phases and totals. The phase-angle, average and reactive-energy names are inferred from the matching
   SDM630-compatible wire offsets (see docs/semantic-recovery.md); device identity is unproven.
   Reserved/undocumented register meanings retain their offsets. */
typedef __packed struct {
    ushort frequency;  /* +0x00 */
    int p_active;      /* +0x02 */
    ushort voltage;    /* +0x06 */
    ushort voltage_l2; /* +0x08 */
    ushort voltage_l3; /* +0x0A */
    ushort current;    /* +0x0C */
    ushort current_l2; /* +0x0E */
    ushort current_l3; /* +0x10 */
    int p_l1;          /* +0x12 */
    int p_l2;          /* +0x16 */
    int p_l3;          /* +0x1A */
    int p_total;       /* +0x1E */
    int q_reactive;    /* +0x22 */
    int q_l2;          /* +0x26 */
    int q_l3;          /* +0x2A */
    int q_total;       /* +0x2E */
    int s_apparent;    /* +0x32 */
    uint s_l2;         /* +0x36 */
    uint s_l3;         /* +0x3A */
    uint s_total;      /* +0x3E */
    ushort pf;         /* +0x42 */
    ushort pf_l2;      /* +0x44 */
    ushort pf_l3;      /* +0x46 */
    ushort pf_total;   /* +0x48 */
    int phase_angle_l1_x10;   /* +0x4A */
    int phase_angle_l2_x10;   /* +0x4E */
    int phase_angle_l3_x10;   /* +0x52 */
    int extended_56;   /* +0x56 */
    int average_voltage_x10;   /* +0x5A */
    int extended_5e;   /* +0x5E */
    int average_current_x10;   /* +0x62 */
    int sum_current_x100;   /* +0x66 */
    int extended_6a;   /* +0x6A */
    int e_import;      /* +0x6E */
    int e_export;      /* +0x72 */
    int reactive_import_energy_x10;   /* +0x76 */
    int reactive_export_energy_x10;   /* +0x7A */
    int f1C;           /* +0x7E */
    int f20;           /* +0x82 */
    ushort f24;        /* +0x86 */
    ushort f26;        /* +0x88 */
    ushort f28;        /* +0x8A */
    ushort f2A;        /* +0x8C */
    ushort f2C;        /* +0x8E */
} meter_t;

/* the device at Modbus address 0x11 (dev11_data): dev11_parse_fc03 unpacks its FC3 reply (the
   registers from 0xCA, big-endian) into h00-h34 and sets fresh; meter_poll sends cmd when
   cmd_pending is 1 (dev11_power_cmd_update sets it); modbus_fc03_read_holding serves the
   registers. */
typedef struct {
    ushort h00;
    ushort h02;
    ushort h04;
    short h06;
    short h08;
    short h0a;
    ushort h0c;
    ushort h0e;
    ushort h10;
    ushort h12;
    ushort h14;
    ushort h16;
    uchar b18[10];
    ushort h22;
    ushort h24;
    ushort h26;
    ushort h28;
    ushort cmd;        /* +0x2A the value to write */
    uchar cmd_pending; /* +0x2C 1: send cmd */
    uchar f2D[3];
    uint w30;    /* +0x30 */
    ushort h34;  /* +0x34 */
    uchar fresh; /* +0x36 1 after a good reply */
} dev11_t;

/* data */
extern dev11_t dev11_data;
extern ushort num_mppt_phases;
extern uchar dev11_reply_pending;
extern uchar dev11_command_pending;
extern ushort meter_reset_guard;
extern uchar meter_retries[];
extern uchar meter_read_function[];
extern uchar meter_fc04_addr;
extern uchar meter_fc03_addr;
extern ushort meter_rx_crc;
extern ushort dev11_rx_crc;
extern uchar meter_task_div;
extern uchar meter_cycle_done; /* set after a full round of the meters (meter_poll step 18) */
extern meter_t meter_data[8];
extern uchar meter_record_3[];
extern meter_t meter_records_5_6[2];
extern meter_t meter_records_7_8[2];
extern uchar meter_reply_address;
extern ushort meter_reply_wait;
extern uint meter_config_timer;
extern ushort meter_bad_frequency_count[8];
extern uchar meter_bad_frequency_mask;
extern uchar meter_protocol_changed;
extern uchar meter_previous_protocol;
extern uchar meter_protocol_ready;
extern uchar meter_config_pending;
extern ushort meter_enable_mask;
extern uchar meter_poll_addr[];
extern ushort meter_poll_inflight;
extern ushort meter_poll_resume;
extern ushort meter_poll_step; /* meter_poll's step: 0-18, 100-104 */
extern ushort meter_poll_timer;
extern uchar meter_power_fresh;
extern uchar meter_rx_buf[];
extern uchar meter_rx_byte;
extern ushort meter_rx_expect;
extern uchar meter_rx_idle;
extern ushort meter_rx_len;
extern uint meter_silence;
extern uchar meter_tx_buf[];
extern ushort v_n_bus;

/* functions */
float be_bytes_to_float(uchar *data, int len);
void dev11_parse_fc03(uchar *frame);
void dev11_power_cmd_update();
void meter_parse_fc03(uchar *frame);
void meter_parse_fc04(uchar *frame);
void meter_poll();
void meter_rx_frame(uchar *frame);
void task_meter();

#endif
