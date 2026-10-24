/* I share the protocol entry points without moving their implementations.
   match: the CRC implementation stays in register_protocol.c; same-unit visibility is
   required by its byte-matched callers. Caller argument widths stay unchanged. */
#ifndef BMS_PROTOCOL_H
#define BMS_PROTOCOL_H
#include "ghidra_types.h"
/* I name the register function codes handled by these frame dispatchers. */
enum BmsModbusFunction {
    BMS_MODBUS_READ_HOLDING_REGISTERS = 3,
    BMS_MODBUS_READ_INPUT_REGISTERS = 4,
    BMS_MODBUS_WRITE_SINGLE_REGISTER = 6
};
ushort protocol_crc16(byte *data, ushort length);
void protocol_initialize(void);
void pack_send_request(int peer_address, int function_code, int register_address,
                       int value_or_count);
void pack_handle_register_write(uchar *frame);
void pack_reply_register_read(int request_address);
void pack_dispatch_frame(byte *frame, uchar length);
void pack_send_pending_settings(void);
void pack_store_register_reply(byte *frame, ushort data_length);
void pack_protocol_task(void);
void inverter_reply_register_write(uchar *frame);
void inverter_reply_input_registers(uchar *frame);
void inverter_reply_register_read(uchar *frame);
void inverter_dispatch_frame(byte *frame);
void inverter_reply_read_error(uchar *frame);
void pack_update_summary(void);
void pack_balance_and_control(void);
#endif
