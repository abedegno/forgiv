/* meter.h: the RS485 Modbus master on USART2 (comm/meter.c): energy meters 1-8 and the device at
   address 0x11, the poll state, and the meter records the Modbus slave and power_flow_update
   use.
   name: descriptive (readability step 1). */
#ifndef METER_H
#define METER_H

#include "ghidra_types.h"

/* one meter's readings (meter_data[8], 0x2E bytes each): filled by meter_parse_fc04 from the
   meter's function 4 replies (big-endian floats scaled to integers) and meter_parse_fc03.
   Units as the scaling suggests (inference): voltage x10, current x100, frequency x100,
   powers in W x10 (EM115) or x10000 kW (EM418), energies x10 */
typedef __packed struct {
    ushort voltage;   /* +0x00 */
    ushort current;   /* +0x02 */
    ushort frequency; /* +0x04 */
    int p_active;     /* +0x06 sign follows HR48/HR49 */
    int q_reactive;   /* +0x0A */
    uint s_apparent;  /* +0x0E */
    short pf;         /* +0x12 10000 - pf x 10000 */
    int e_import;     /* +0x14 */
    int e_export;     /* +0x18 */
    int f1C;          /* +0x1C function 3 reply */
    int f20;          /* +0x20 function 3 reply */
    ushort f24;       /* +0x24 */
    ushort f26;       /* +0x26 */
    ushort f28;       /* +0x28 */
    ushort f2A;       /* +0x2A */
    ushort f2C;       /* +0x2C */
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
extern uchar g_20000474;
extern uchar g_20000475;
extern ushort g_20000484;
extern uchar g_20000486[];
extern uchar g_20000487[];
extern uchar meter_fc04_addr;
extern uchar meter_fc03_addr;
extern ushort meter_rx_crc;
extern ushort dev11_rx_crc;
extern uchar meter_task_div;
extern uchar meter_cycle_done; /* set after a full round of the meters (meter_poll step 18) */
extern meter_t meter_data[8];
extern uchar meter_data_7_8[];
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
