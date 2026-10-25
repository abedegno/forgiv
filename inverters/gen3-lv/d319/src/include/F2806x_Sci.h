/* F2806x_Sci.h: TI's SCI register layout: SciaRegs at 0x7050 (the ARM link) and ScibRegs at
   0x7750 (the BMS bus). Users: drivers/sci.c, drivers/sci_isr.c, drivers/device_init.c
   (sci_init), app/comm/arm_link.c (SCIHBAUD), boot_flash_loader.c.
   name: original, TI's file name. */
#ifndef F2806X_SCI_H
#define F2806X_SCI_H

#include "F2806x_Device.h"

struct SCICCR_BITS {
    Uint16 SCICHAR : 3;
    Uint16 ADDRIDLE_MODE : 1;
    Uint16 LOOPBKENA : 1;
    Uint16 PARITYENA : 1;
    Uint16 PARITY : 1;
    Uint16 STOPBITS : 1;
    Uint16 rsvd1 : 8;
};
union SCICCR_REG {
    Uint16 all;
    struct SCICCR_BITS bit;
};

struct SCICTL1_BITS {
    Uint16 RXENA : 1;
    Uint16 TXENA : 1;
    Uint16 SLEEP : 1;
    Uint16 TXWAKE : 1;
    Uint16 rsvd : 1;
    Uint16 SWRESET : 1;
    Uint16 RXERRINTENA : 1;
    Uint16 rsvd1 : 9;
};
union SCICTL1_REG {
    Uint16 all;
    struct SCICTL1_BITS bit;
};

struct SCICTL2_BITS {
    Uint16 TXINTENA : 1;
    Uint16 RXBKINTENA : 1;
    Uint16 rsvd : 4;
    Uint16 TXEMPTY : 1;
    Uint16 TXRDY : 1;
    Uint16 rsvd1 : 8;
};
union SCICTL2_REG {
    Uint16 all;
    struct SCICTL2_BITS bit;
};

struct SCIRXST_BITS {
    Uint16 rsvd : 1;
    Uint16 RXWAKE : 1;
    Uint16 PE : 1;
    Uint16 OE : 1;
    Uint16 FE : 1;
    Uint16 BRKDT : 1;
    Uint16 RXRDY : 1;
    Uint16 RXERROR : 1;
    Uint16 rsvd1 : 8;
};
union SCIRXST_REG {
    Uint16 all;
    struct SCIRXST_BITS bit;
};

struct SCIRXBUF_BITS {
    Uint16 RXDT : 8;
    Uint16 rsvd : 6;
    Uint16 SCIFFFE : 1;
    Uint16 SCIFFPE : 1;
};
union SCIRXBUF_REG {
    Uint16 all;
    struct SCIRXBUF_BITS bit;
};

struct SCIFFTX_BITS {
    Uint16 TXFFIL : 5;
    Uint16 TXFFIENA : 1;
    Uint16 TXFFINTCLR : 1;
    Uint16 TXFFINT : 1;
    Uint16 TXFFST : 5;
    Uint16 TXFIFOXRESET : 1;
    Uint16 SCIFFENA : 1;
    Uint16 SCIRST : 1;
};
union SCIFFTX_REG {
    Uint16 all;
    struct SCIFFTX_BITS bit;
};

struct SCIFFRX_BITS {
    Uint16 RXFFIL : 5;
    Uint16 RXFFIENA : 1;
    Uint16 RXFFINTCLR : 1;
    Uint16 RXFFINT : 1;
    Uint16 RXFFST : 5;
    Uint16 RXFIFORESET : 1;
    Uint16 RXFFOVRCLR : 1;
    Uint16 RXFFOVF : 1;
};
union SCIFFRX_REG {
    Uint16 all;
    struct SCIFFRX_BITS bit;
};

struct SCIFFCT_BITS {
    Uint16 FFTXDLY : 8;
    Uint16 rsvd : 5;
    Uint16 CDC : 1;
    Uint16 ABDCLR : 1;
    Uint16 ABD : 1;
};
union SCIFFCT_REG {
    Uint16 all;
    struct SCIFFCT_BITS bit;
};

struct SCIPRI_BITS {
    Uint16 rsvd : 3;
    Uint16 FREE : 1;
    Uint16 SOFT : 1;
    Uint16 rsvd1 : 3;
    Uint16 rsvd2 : 8;
};
union SCIPRI_REG {
    Uint16 all;
    struct SCIPRI_BITS bit;
};

struct SCI_REGS {
    union SCICCR_REG SCICCR;     /* 0 */
    union SCICTL1_REG SCICTL1;   /* 1 */
    Uint16 SCIHBAUD;             /* 2 */
    Uint16 SCILBAUD;             /* 3 */
    union SCICTL2_REG SCICTL2;   /* 4 */
    union SCIRXST_REG SCIRXST;   /* 5 */
    Uint16 SCIRXEMU;             /* 6 */
    union SCIRXBUF_REG SCIRXBUF; /* 7 */
    Uint16 rsvd1;                /* 8 */
    Uint16 SCITXBUF;             /* 9 */
    union SCIFFTX_REG SCIFFTX;   /* 10 */
    union SCIFFRX_REG SCIFFRX;   /* 11 */
    union SCIFFCT_REG SCIFFCT;   /* 12 */
    Uint16 rsvd2;                /* 13 */
    Uint16 rsvd3;                /* 14 */
    union SCIPRI_REG SCIPRI;     /* 15 */
};

extern volatile struct SCI_REGS SciaRegs;
extern volatile struct SCI_REGS ScibRegs;

#endif
