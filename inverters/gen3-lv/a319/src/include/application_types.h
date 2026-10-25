/* application_types.h: small types shared by the application files, recovered from the code that uses
   them: the UART queue records, two overlays of the settings block that the Modbus write
   handlers use, and the NVIC register block.

   Used by: drivers/uart.c, stm32f10x_it.c, comm/modbus_slave.c, energy.h, uart.h.
   name: descriptive (readability step 1). */
#ifndef APPLICATION_TYPES_H
#define APPLICATION_TYPES_H

/* a UART transmit queue (0x20001A96, one per port, 9 bytes) */
typedef __packed struct {
    uchar busy; /* 1 while sending */
    ushort len; /* bytes to send */
    ushort idx; /* next byte to send */
    uchar *buf;
} uart_tx;

/* a UART receive queue (0x20001A72, one per port, 9 bytes) */
typedef __packed struct {
    uchar busy; /* 1: nothing to read; the ISR clears it */
    uchar pending_count;    /* bytes not yet read (ch 3: bytes read) */
    uchar write_index;    /* write index of the ISR */
    uchar idx;  /* read index of the task */
    uchar *buf; /* uart_rx_buf[ch] */
    uchar size; /* buffer size, at most 151 */
} uart_rx;

/* overlays of the settings block at 0x20001ABA (Modbus FC6/FC16 handlers) */
struct settings_serial_view {
    uchar reserved_00[0x13];
    uchar serial_bytes[5][2];
}; /* serial_bytes: the serial, HR13-17 (cfg_settings +0x13) */
typedef __packed struct {
    uchar reserved_00[0x194];
    int day_start[2][8];
} settings_energy_view_t; /* Import/export day-start counters, settings + 0x194. */

/* NVIC set-enable and clear-enable registers; the gap is reserved hardware space. */
typedef struct {
    volatile uint ISER[8];
    uint reserved_20[24];
    volatile uint ICER[8];
} nvic_t;

#endif
