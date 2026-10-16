/* a316_types.h: small types shared by the application files, recovered from the code that uses
   them: the UART queue records, two overlays of the settings block that the Modbus write
   handlers use, and the NVIC register block.

   Used by: drivers/uart.c, stm32f10x_it.c, comm/modbus_slave.c, energy.h, uart.h.
   name: descriptive (readability step 1). */
#ifndef A316_TYPES_H
#define A316_TYPES_H

/* a UART transmit queue (0x20001A8A, one per port, 9 bytes) */
typedef __packed struct {
    uchar busy; /* 1 while sending */
    ushort len; /* bytes to send */
    ushort idx; /* next byte to send */
    uchar *buf;
} uart_tx;

/* a UART receive queue (0x20001A66, one per port, 9 bytes) */
typedef __packed struct {
    uchar busy; /* 1: nothing to read; the ISR clears it */
    uchar a;    /* bytes not yet read (ch 3: bytes read) */
    uchar b;    /* write index of the ISR */
    uchar idx;  /* read index of the task */
    uchar *buf; /* uart_rx_buf[ch] */
    uchar size; /* buffer size, at most 151 */
} uart_rx;

/* overlays of the settings block at 0x20001AAE (Modbus register handlers 0801C160, 08018816) */
struct aae {
    uchar p[0x13];
    uchar t[5][2];
}; /* t: the serial, HR13-17 (cfg_settings +0x13) */
typedef __packed struct {
    uchar p[0x194];
    int a[2][8];
} aaeb_t; /* a: the meters' import and
                                                                      export day-start counters
                                                                      (cfg_settings + 0x194 =
                                                                      meter_in_day_start) */

/* the NVIC registers the handlers write */
typedef struct {
    volatile uint ISER[8];
    uint r0[24];
    volatile uint ICER[8];
} nvic_t;

#endif
