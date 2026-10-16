/* sci.h: the SCI drivers (drivers/sci.c, sci_isr.c): the receive ring and send state, the
   ports and their declarations. Port 0 is SCI-A, the ARM link; port 1 is SCI-B, the BMS bus;
   both run at 9600 8N1.
   name: descriptive. */
#ifndef SCI_H
#define SCI_H

#include "F2806x_Device.h"
#include "util.h"

typedef struct {
    unsigned int empty; /* 1 = nothing to read */
    unsigned int count; /* bytes held */
    unsigned int wr;    /* write index */
    unsigned int rd;    /* read index */
    unsigned int *buf;
    unsigned int size;
} SCI_RX; /* a receive ring (sci_rx_ring[port]); during the BMS relay, port 0 uses wr as its byte count */

typedef struct {
    unsigned int busy;
    unsigned int len;
    unsigned int pos;
    unsigned int *buf;
} SCI_TX; /* a port's send state (sci_tx_state[port]); port 1 sends from bms_tx_buf instead */

#define SCI_PORTS 2     /* 0: SCI-A, the ARM link; 1: SCI-B, the BMS bus */
#define SCI_RX_SIZE 150 /* bytes in each receive ring */

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* device_init.c */
void sci_init(void);

/* sci.c */
extern unsigned int arm_rx_len;
unsigned int ring_read(unsigned int port, unsigned int *out);
void ring_reset(unsigned int port, unsigned int size);
extern unsigned int sci_end_tag[3];
void sci_rx(unsigned int port);
extern SCI_RX sci_rx_ring[SCI_PORTS];
unsigned int sci_send(unsigned int port, unsigned int len, unsigned int *buf, unsigned int *src);
void sci_tx(unsigned int port);
extern unsigned int sci_tx_done;
extern SCI_TX sci_tx_state[SCI_PORTS];

/* sci_isr.c */
interrupt void scia_rx_isr(void);
interrupt void scia_tx_isr(void);
interrupt void scib_rx_isr(void);
interrupt void scib_tx_isr(void);

/* defined by no source: the linker places it at its map address */
extern volatile unsigned long arm_rx_block; /* really os_tcb[5].events (0xDAAC): sci_rx clears it */
extern unsigned int sci_rx_buf[SCI_PORTS][SCI_RX_SIZE]; /* 0xD000 */
/* ---- end of declarations ---- */

#endif
