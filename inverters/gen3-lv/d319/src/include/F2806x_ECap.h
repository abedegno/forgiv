/* F2806x_ECap.h: TI's eCAP register layout: ECap1Regs at 0x6A00, the grid period capture.
   Users: drivers/device_init.c (InitECap) and the grid period interrupt.
   name: original, TI's file name; only eCAP1 is declared. */
#ifndef F2806X_ECAP_H
#define F2806X_ECAP_H

#include "F2806x_Device.h"

struct ECCTL1_BITS {
    Uint16 CAP1POL : 1;
    Uint16 CTRRST1 : 1;
    Uint16 CAP2POL : 1;
    Uint16 CTRRST2 : 1;
    Uint16 CAP3POL : 1;
    Uint16 CTRRST3 : 1;
    Uint16 CAP4POL : 1;
    Uint16 CTRRST4 : 1;
    Uint16 CAPLDEN : 1;
    Uint16 PRESCALE : 5;
    Uint16 FREE_SOFT : 2;
};
union ECCTL1_REG {
    Uint16 all;
    struct ECCTL1_BITS bit;
};

struct ECCTL2_BITS {
    Uint16 CONT_ONESHT : 1;
    Uint16 STOP_WRAP : 2;
    Uint16 REARM : 1;
    Uint16 TSCTRSTOP : 1;
    Uint16 SYNCI_EN : 1;
    Uint16 SYNCO_SEL : 2;
    Uint16 SWSYNC : 1;
    Uint16 CAP_APWM : 1;
    Uint16 APWMPOL : 1;
    Uint16 rsvd1 : 5;
};
union ECCTL2_REG {
    Uint16 all;
    struct ECCTL2_BITS bit;
};

struct ECEINT_BITS {
    Uint16 rsvd1 : 1;
    Uint16 CEVT1 : 1;
    Uint16 CEVT2 : 1;
    Uint16 CEVT3 : 1;
    Uint16 CEVT4 : 1;
    Uint16 CTROVF : 1;
    Uint16 CTR_EQ_PRD : 1;
    Uint16 CTR_EQ_CMP : 1;
    Uint16 rsvd2 : 8;
};
union ECEINT_REG {
    Uint16 all;
    struct ECEINT_BITS bit;
};

struct ECFLG_BITS {
    Uint16 INT : 1;
    Uint16 CEVT1 : 1;
    Uint16 CEVT2 : 1;
    Uint16 CEVT3 : 1;
    Uint16 CEVT4 : 1;
    Uint16 CTROVF : 1;
    Uint16 CTR_EQ_PRD : 1;
    Uint16 CTR_EQ_CMP : 1;
    Uint16 rsvd1 : 8;
};
union ECFLG_REG {
    Uint16 all;
    struct ECFLG_BITS bit;
};

struct ECCLR_BITS {
    Uint16 INT : 1;
    Uint16 CEVT1 : 1;
    Uint16 CEVT2 : 1;
    Uint16 CEVT3 : 1;
    Uint16 CEVT4 : 1;
    Uint16 CTROVF : 1;
    Uint16 CTR_EQ_PRD : 1;
    Uint16 CTR_EQ_CMP : 1;
    Uint16 rsvd1 : 8;
};
union ECCLR_REG {
    Uint16 all;
    struct ECCLR_BITS bit;
};

struct ECFRC_BITS {
    Uint16 rsvd1 : 1;
    Uint16 CEVT1 : 1;
    Uint16 CEVT2 : 1;
    Uint16 CEVT3 : 1;
    Uint16 CEVT4 : 1;
    Uint16 CTROVF : 1;
    Uint16 CTR_EQ_PRD : 1;
    Uint16 CTR_EQ_CMP : 1;
    Uint16 rsvd2 : 8;
};
union ECFRC_REG {
    Uint16 all;
    struct ECFRC_BITS bit;
};

struct ECAP_REGS {
    Uint32 TSCTR;            /* 0x00 */
    Uint32 CTRPHS;           /* 0x02 */
    Uint32 CAP1;             /* 0x04 */
    Uint32 CAP2;             /* 0x06 */
    Uint32 CAP3;             /* 0x08 */
    Uint32 CAP4;             /* 0x0A */
    Uint16 rsvd1[8];         /* 0x0C */
    union ECCTL1_REG ECCTL1; /* 0x14 */
    union ECCTL2_REG ECCTL2; /* 0x15 */
    union ECEINT_REG ECEINT; /* 0x16 */
    union ECFLG_REG ECFLG;   /* 0x17 */
    union ECCLR_REG ECCLR;   /* 0x18 */
    union ECFRC_REG ECFRC;   /* 0x19 */
    Uint16 rsvd2[6];         /* 0x1A */
};

extern volatile struct ECAP_REGS ECap1Regs;

#endif
