/* sci.c: the SCI drivers, interrupt-driven, one byte per word.

     port 0  SCI-A, the ARM link: GPIO28 RX, GPIO29 TX, 9600 8N1 (sci_init).
     port 1  SCI-B, the BMS's RS-485 Modbus bus: GPIO23 RX, GPIO18 TX, 9600 8N1. GPIO57 is the
             bus driver's enable, I think: bms_link.c raises it to send and drops it after.

   Receive: sci_rx (from the receive interrupts, sci_isr.c) puts each byte into the port's ring
   (sci_rx_ring[port] over sci_rx_buf[port], 150 words; app_init sizes both to 150, bms_link
   cuts port 1 back to 60 on errors). Port 0 keeps only frames whose first byte is 0x43 and posts
   EV5_FRAME to task 5 when 45 bytes are in. Port 1 keeps everything; bms_link.c finds the frames.
   The tasks take bytes with ring_read and empty a ring with ring_reset. The read index never
   wraps: the ring starts again at 0 each time it runs empty, and a read at the end resets it.

   Send: port 0 through sci_send (the ARM frames: a copy in sci_tx_buf0, then one byte per
   transmit interrupt, sci_tx_done = 1 at the end). Port 1 sends bms_tx_buf, which bms_link.c
   fills and starts with sci_tx(1); bms_tx_state = 1 when it is out.

   The BMS pass-through (battery firmware relay). While bms_relay_active is 1 (set by
   arm_link_frame when the BMS's HR14, bms_relay_request, is 1) the two ports are joined:
     ARM to BMS  sci_rx(0) uses ring 0's write index as a byte counter. It waits for 'B' 'M' 'S',
                 raises GPIO57, drops byte 3, keeps bytes 4..6 in sci_end_tag (the frame's tag:
                 "HED" for a data frame, "END" for the last, from the A316's relay frames "BMS",
                 1, "HED", 1024 bytes, sum, "E"), and writes bytes 4..1032 to SCI-B as they come.
                 Byte 1033 drops GPIO57 and starts again. The 0x43 frames stop meanwhile.
     BMS to ARM  sci_rx(1) writes each byte to SCI-A and clears bms_relay_idle.
   Neither direction checks TXRDY; at the same baud rate each byte is out before the next comes
   in. bms_relay_active is cleared only by arm_link_frame, which does not run while the relay
   lasts (task 5 waits for it to end); power_limits.c raises GPIO27 after 15000 passes without a
   BMS byte (docs/findings-comm.md).

   Entry points: ring_reset, ring_read, sci_send (task code); sci_rx, sci_tx (sci_isr.c, and
   bms_link.c for sci_tx(1)).
   Data: arm_rx_len (arm_link.c's byte count; sci.c does not use it), sci_tx_done, sci_end_tag,
   sci_tx_state[2], sci_rx_ring[2] (from 0xCF40); the buffers sci_rx_buf (0xD000) and
   sci_tx_buf0 (0xCF80) are placed by the link.
   Neighbours: sci_isr.c, device_init.c (sci_init), app/comm/arm_link.c, app/battery/bms_link.c,
   app/power/power_limits.c (the relay timeout), app/state/app_init.c (ring_reset at start).
   name: descriptive (map/filenames.tsv), name kept.
   at: ring_reset 0x3F1322, sci_rx 0x3F134E, sci_tx 0x3F1417, ring_read 0x3F145F, sci_send 0x3F1487;
   arm_rx_len 0xCF40 .. sci_rx_ring 0xCF52 */
/* Compiled with 16.9.6 (docs/notes.md): 15.12.7 keeps ring_read's out pointer in
   XAR4, the image and 16.9.3 and later copy it to XAR7 at entry and keep the ring in XAR4.
   The image reads SCIRXBUF in the normal receive paths through the base register of the RXRDY
   test; a read through the register block as a word array gives that, the struct member does
   not. */
#include <string.h>
#include "F2806x_Device.h"
#include "F2806x_Sci.h"
#include "F2806x_Gpio.h"
#include "os.h"
#include "sci.h"
#include "bms.h"
#include "util.h"

/* ebss-begin */
unsigned int arm_rx_len;            /* name: arm_link.c counts the bytes of the incoming ARM frame here; sci.c never uses it */
unsigned int sci_tx_done = 1;
unsigned int sci_end_tag[3];
SCI_TX sci_tx_state[SCI_PORTS];
SCI_RX sci_rx_ring[SCI_PORTS];
/* ebss-end */

/* Empties ring port and gives it size words (at most SCI_RX_SIZE) of sci_rx_buf[port],
   cleared. */
void ring_reset(unsigned int port, unsigned int size)
{
    SCI_RX *r;
    unsigned int i;
    if (port >= SCI_PORTS) return;
    if (size > SCI_RX_SIZE) size = SCI_RX_SIZE;
    for (i = 0; i < size; i++)
        sci_rx_buf[port][i] = 0;
    r = &sci_rx_ring[port];
    r->empty = 1;
    r->count = 0;
    r->wr = 0;
    r->rd = 0;
    r->buf = sci_rx_buf[port];
    r->size = size;
}

/* Takes one received byte of port, from its receive interrupt. A receive error resets the
   port first (port 0 also abandons the frame being sent). Port 0: the relay state machine while
   bms_relay_active is 1 (see the file header), else the 0x43 frame filter and EV5_FRAME at 45
   bytes. Port 1: forwards to SCI-A during the relay, else stores in ring 1. */
void sci_rx(unsigned int port)
{
    SCI_RX *r;
    unsigned int c;
    if (port >= SCI_PORTS) return;
    r = &sci_rx_ring[port];
    if (port == 0) {
        if (SciaRegs.SCIRXST.bit.RXERROR) {
            SciaRegs.SCICTL1.all = 3;
            SciaRegs.SCICTL2.all = 3;
            SciaRegs.SCICTL1.bit.RXERRINTENA = 1;
            sci_tx_state[0].pos = 0;
            sci_tx_done = 1;
            SciaRegs.SCICTL1.all = 0x23;
            sci_tx_state[0].busy = 0;
        }
        if (!SciaRegs.SCIRXST.bit.RXRDY) return;
        if (bms_relay_active == 1) {
            c = SciaRegs.SCIRXBUF.all & 0xFF;
            switch (r->wr) {
            case 0:
                if (c == 'B') r->wr++; else r->wr = 0;
                break;
            case 1:
                if (c == 'M') r->wr++; else r->wr = 0;
                break;
            case 2:
                if (c == 'S') {
                    r->wr++;
                    GpioDataRegs.GPBSET.bit.GPIO57 = 1;  /* the BMS bus driver on */
                } else r->wr = 0;
                break;
            case 3:
                r->wr++;
                break;
            default:
                r->wr++;
                switch (r->wr) {
                case 5: sci_end_tag[0] = c; break;
                case 6: sci_end_tag[1] = c; break;
                case 7: sci_end_tag[2] = c; break;
                }
                if (r->wr < 1034)  /* bytes 4..1032 of the relay frame go to the BMS */
                    ScibRegs.SCITXBUF = c;
                else {
                    GpioDataRegs.GPBCLEAR.bit.GPIO57 = 1;
                    r->wr = 0;
                }
                break;
            }
            return;
        }
        c = ((volatile Uint16 *)&SciaRegs)[7] & 0xFF;     /* SCIRXBUF, through the base the RXRDY test used */
        arm_rx_block &= 8;  /* arm_rx_block is os_tcb[5].events (0xDAAC): this clears task 5's pending events */
        if (arm_rx_block != 0) return;
        r->buf[r->wr] = c;
        if (r->buf[0] == 'C') {
            r->wr++;
            if (r->wr >= r->size) r->wr = 0;
            if (r->count < r->size) r->count++;
            r->empty = 0;
            if (r->count == 45) os_post_isr(TASK_ARM_LINK, EV5_FRAME);  /* once per frame: the task's reads take count back down */
        } else {
            r->wr = 0;
            sci_rx_buf[0][0] = 0;
        }
    } else if (port == 1) {
        if (ScibRegs.SCIRXST.bit.RXERROR) {
            ScibRegs.SCICTL1.all = 3;
            ScibRegs.SCICTL2.all = 3;
            ScibRegs.SCICTL1.bit.RXERRINTENA = 1;
            ScibRegs.SCICTL1.all = 0x23;
        }
        if (!ScibRegs.SCIRXST.bit.RXRDY) return;
        if (bms_relay_active == 1) {
            bms_relay_idle = 0;
            SciaRegs.SCITXBUF = ScibRegs.SCIRXBUF.all & 0xFF;
        } else {
            r->buf[r->wr] = ((volatile Uint16 *)&ScibRegs)[7] & 0xFF;
            r->wr++;
            if (r->wr >= r->size) r->wr = 0;
            if (r->count < r->size) r->count++;
            r->empty = 0;
        }
    }
}

/* Sends the next byte of port, from its transmit interrupt. Port 0: the next byte of the
   sci_send buffer when SCI-A is ready; after the last one it sets sci_tx_done and frees the
   port. Port 1: the next byte of bms_tx_buf with the transmit interrupt on; after the last one
   it turns the interrupt off and sets bms_tx_state to 1. */
void sci_tx(unsigned int port)
{
    SCI_TX *t;
    if (port >= SCI_PORTS) return;
    t = &sci_tx_state[port];
    if (port == 0) {
        if (t->pos < t->len) {
            if (SciaRegs.SCICTL2.bit.TXRDY) {
                SciaRegs.SCITXBUF = t->buf[t->pos];
                t->pos++;
            }
        } else if (t->pos >= t->len && t->len != 0) {
            sci_tx_done = 1;
            t->busy = 0;
        }
    } else if (port == 1) {
        if (bms_tx_pos < bms_tx_len) {
            ScibRegs.SCICTL2.bit.TXINTENA = 1;
            ScibRegs.SCITXBUF = bms_tx_buf[bms_tx_pos++];
        } else {
            ScibRegs.SCICTL2.bit.TXINTENA = 0;
            bms_tx_pos = 0;
            bms_tx_state = 1;
        }
    }
}

/* Takes the next byte of ring port into *out. Returns 0 with a byte, 1 when the ring is empty.
   When the count reaches 0, the ring starts again at index 0. */
unsigned int ring_read(unsigned int port, unsigned int *out)
{
    SCI_RX *r;
    if (port >= SCI_PORTS) return 1;
    r = &sci_rx_ring[port];
    if (r->empty == 1) return 1;
    if (r->rd < r->size) {
        *out = r->buf[r->rd];
        r->rd++;
        if (r->count) r->count--;
        if (r->count == 0) {
            r->empty = 1;
            r->wr = 0;
            r->rd = 0;
        }
    } else {
        r->empty = 1;
        r->wr = 0;
        r->rd = 0;
        r->count = 0;
    }
    return 0;
}

/* Starts sending len words of src on port: copies them to buf (which must stay valid until
   the end) and sends the first byte; the transmit interrupt sends the rest. Returns 1 when the
   port is busy or len is 0, else 0. Only port 0 uses it. */
unsigned int sci_send(unsigned int port, unsigned int len, unsigned int *buf, unsigned int *src)
{
    SCI_TX *t;
    if (port >= SCI_PORTS || len == 0) return 1;
    t = &sci_tx_state[port];
    if (t->busy == 1) return 1;
    memcpy(buf, src, len);
    DINT;
    t->busy = 1;
    t->len = len;
    t->pos = 0;
    t->buf = buf;
    EINT;
    sci_tx(port);
    return 0;
}
