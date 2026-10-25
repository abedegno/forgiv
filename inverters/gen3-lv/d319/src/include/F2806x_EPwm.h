/* F2806x_EPwm.h: TI's ePWM register layout (EPwm1Regs at 0x6800, then every 0x40 words to
   EPwm8Regs). Users: drivers/device_init.c (InitEPwm, the trip zones), drivers/pwm_force.c and
   the control code.
   name: original, TI's file name. */
#ifndef F2806X_EPWM_H
#define F2806X_EPWM_H

#include "F2806x_Device.h"

struct TBCTL_BITS {
    Uint16 CTRMODE : 2;
    Uint16 PHSEN : 1;
    Uint16 PRDLD : 1;
    Uint16 SYNCOSEL : 2;
    Uint16 SWFSYNC : 1;
    Uint16 HSPCLKDIV : 3;
    Uint16 CLKDIV : 3;
    Uint16 PHSDIR : 1;
    Uint16 FREE_SOFT : 2;
};
union TBCTL_REG {
    Uint16 all;
    struct TBCTL_BITS bit;
};

struct TBSTS_BITS {
    Uint16 CTRDIR : 1;
    Uint16 SYNCI : 1;
    Uint16 CTRMAX : 1;
    Uint16 rsvd1 : 13;
};
union TBSTS_REG {
    Uint16 all;
    struct TBSTS_BITS bit;
};

struct TBPHS_HRPWM_REG {
    Uint16 TBPHSHR;
    Uint16 TBPHS;
};
union TBPHS_HRPWM_GROUP {
    Uint32 all;
    struct TBPHS_HRPWM_REG half;
};

struct CMPCTL_BITS {
    Uint16 LOADAMODE : 2;
    Uint16 LOADBMODE : 2;
    Uint16 SHDWAMODE : 1;
    Uint16 rsvd1 : 1;
    Uint16 SHDWBMODE : 1;
    Uint16 rsvd2 : 1;
    Uint16 SHDWAFULL : 1;
    Uint16 SHDWBFULL : 1;
    Uint16 rsvd3 : 6;
};
union CMPCTL_REG {
    Uint16 all;
    struct CMPCTL_BITS bit;
};

struct CMPA_HRPWM_REG {
    Uint16 CMPAHR;
    Uint16 CMPA;
};
union CMPA_HRPWM_GROUP {
    Uint32 all;
    struct CMPA_HRPWM_REG half;
};

struct AQCTL_BITS {
    Uint16 ZRO : 2;
    Uint16 PRD : 2;
    Uint16 CAU : 2;
    Uint16 CAD : 2;
    Uint16 CBU : 2;
    Uint16 CBD : 2;
    Uint16 rsvd : 4;
};
union AQCTL_REG {
    Uint16 all;
    struct AQCTL_BITS bit;
};

struct AQSFRC_BITS {
    Uint16 ACTSFA : 2;
    Uint16 OTSFA : 1;
    Uint16 ACTSFB : 2;
    Uint16 OTSFB : 1;
    Uint16 RLDCSF : 2;
    Uint16 rsvd1 : 8;
};
union AQSFRC_REG {
    Uint16 all;
    struct AQSFRC_BITS bit;
};

struct AQCSFRC_BITS {
    Uint16 CSFA : 2;
    Uint16 CSFB : 2;
    Uint16 rsvd1 : 12;
};
union AQCSFRC_REG {
    Uint16 all;
    struct AQCSFRC_BITS bit;
};

struct DBCTL_BITS {
    Uint16 OUT_MODE : 2;
    Uint16 POLSEL : 2;
    Uint16 IN_MODE : 2;
    Uint16 rsvd1 : 9;
    Uint16 HALFCYCLE : 1;
};
union DBCTL_REG {
    Uint16 all;
    struct DBCTL_BITS bit;
};

struct TZSEL_BITS {
    Uint16 CBC1 : 1;
    Uint16 CBC2 : 1;
    Uint16 CBC3 : 1;
    Uint16 CBC4 : 1;
    Uint16 CBC5 : 1;
    Uint16 CBC6 : 1;
    Uint16 DCAEVT2 : 1;
    Uint16 DCBEVT2 : 1;
    Uint16 OSHT1 : 1;
    Uint16 OSHT2 : 1;
    Uint16 OSHT3 : 1;
    Uint16 OSHT4 : 1;
    Uint16 OSHT5 : 1;
    Uint16 OSHT6 : 1;
    Uint16 DCAEVT1 : 1;
    Uint16 DCBEVT1 : 1;
};
union TZSEL_REG {
    Uint16 all;
    struct TZSEL_BITS bit;
};

struct TZDCSEL_BITS {
    Uint16 DCAEVT1 : 3;
    Uint16 DCAEVT2 : 3;
    Uint16 DCBEVT1 : 3;
    Uint16 DCBEVT2 : 3;
    Uint16 rsvd : 4;
};
union TZDCSEL_REG {
    Uint16 all;
    struct TZDCSEL_BITS bit;
};

struct TZCTL_BITS {
    Uint16 TZA : 2;
    Uint16 TZB : 2;
    Uint16 DCAEVT1 : 2;
    Uint16 DCAEVT2 : 2;
    Uint16 DCBEVT1 : 2;
    Uint16 DCBEVT2 : 2;
    Uint16 rsvd : 4;
};
union TZCTL_REG {
    Uint16 all;
    struct TZCTL_BITS bit;
};

struct TZEINT_BITS {
    Uint16 rsvd1 : 1;
    Uint16 CBC : 1;
    Uint16 OST : 1;
    Uint16 DCAEVT1 : 1;
    Uint16 DCAEVT2 : 1;
    Uint16 DCBEVT1 : 1;
    Uint16 DCBEVT2 : 1;
    Uint16 rsvd2 : 9;
};
union TZEINT_REG {
    Uint16 all;
    struct TZEINT_BITS bit;
};

struct TZFLG_BITS {
    Uint16 INT : 1;
    Uint16 CBC : 1;
    Uint16 OST : 1;
    Uint16 DCAEVT1 : 1;
    Uint16 DCAEVT2 : 1;
    Uint16 DCBEVT1 : 1;
    Uint16 DCBEVT2 : 1;
    Uint16 rsvd2 : 9;
};
union TZFLG_REG {
    Uint16 all;
    struct TZFLG_BITS bit;
};
union TZCLR_REG {
    Uint16 all;
    struct TZFLG_BITS bit;
};

struct TZFRC_BITS {
    Uint16 rsvd1 : 1;
    Uint16 CBC : 1;
    Uint16 OST : 1;
    Uint16 DCAEVT1 : 1;
    Uint16 DCAEVT2 : 1;
    Uint16 DCBEVT1 : 1;
    Uint16 DCBEVT2 : 1;
    Uint16 rsvd2 : 9;
};
union TZFRC_REG {
    Uint16 all;
    struct TZFRC_BITS bit;
};

struct ETSEL_BITS {
    Uint16 INTSEL : 3;
    Uint16 INTEN : 1;
    Uint16 rsvd1 : 4;
    Uint16 SOCASEL : 3;
    Uint16 SOCAEN : 1;
    Uint16 SOCBSEL : 3;
    Uint16 SOCBEN : 1;
};
union ETSEL_REG {
    Uint16 all;
    struct ETSEL_BITS bit;
};

struct ETPS_BITS {
    Uint16 INTPRD : 2;
    Uint16 INTCNT : 2;
    Uint16 rsvd1 : 4;
    Uint16 SOCAPRD : 2;
    Uint16 SOCACNT : 2;
    Uint16 SOCBPRD : 2;
    Uint16 SOCBCNT : 2;
};
union ETPS_REG {
    Uint16 all;
    struct ETPS_BITS bit;
};

struct ETFLG_BITS {
    Uint16 INT : 1;
    Uint16 rsvd1 : 1;
    Uint16 SOCA : 1;
    Uint16 SOCB : 1;
    Uint16 rsvd2 : 12;
};
union ETFLG_REG {
    Uint16 all;
    struct ETFLG_BITS bit;
};
union ETCLR_REG {
    Uint16 all;
    struct ETFLG_BITS bit;
};
union ETFRC_REG {
    Uint16 all;
    struct ETFLG_BITS bit;
};

struct PCCTL_BITS {
    Uint16 CHPEN : 1;
    Uint16 OSHTWTH : 4;
    Uint16 CHPFREQ : 3;
    Uint16 CHPDUTY : 3;
    Uint16 rsvd1 : 5;
};
union PCCTL_REG {
    Uint16 all;
    struct PCCTL_BITS bit;
};

struct EPWM_REGS {
    union TBCTL_REG TBCTL;         /* 0x00 */
    union TBSTS_REG TBSTS;         /* 0x01 */
    union TBPHS_HRPWM_GROUP TBPHS; /* 0x02 */
    Uint16 TBCTR;                  /* 0x04 */
    Uint16 TBPRD;                  /* 0x05 */
    Uint16 TBPRDHR;                /* 0x06 */
    union CMPCTL_REG CMPCTL;       /* 0x07 */
    union CMPA_HRPWM_GROUP CMPA;   /* 0x08 */
    Uint16 CMPB;                   /* 0x0A */
    union AQCTL_REG AQCTLA;        /* 0x0B */
    union AQCTL_REG AQCTLB;        /* 0x0C */
    union AQSFRC_REG AQSFRC;       /* 0x0D */
    union AQCSFRC_REG AQCSFRC;     /* 0x0E */
    union DBCTL_REG DBCTL;         /* 0x0F */
    Uint16 DBRED;                  /* 0x10 */
    Uint16 DBFED;                  /* 0x11 */
    union TZSEL_REG TZSEL;         /* 0x12 */
    union TZDCSEL_REG TZDCSEL;     /* 0x13 */
    union TZCTL_REG TZCTL;         /* 0x14 */
    union TZEINT_REG TZEINT;       /* 0x15 */
    union TZFLG_REG TZFLG;         /* 0x16 */
    union TZCLR_REG TZCLR;         /* 0x17 */
    union TZFRC_REG TZFRC;         /* 0x18 */
    union ETSEL_REG ETSEL;         /* 0x19 */
    union ETPS_REG ETPS;           /* 0x1A */
    union ETFLG_REG ETFLG;         /* 0x1B */
    union ETCLR_REG ETCLR;         /* 0x1C */
    union ETFRC_REG ETFRC;         /* 0x1D */
    union PCCTL_REG PCCTL;         /* 0x1E */
    Uint16 rsvd1;                  /* 0x1F */
    Uint16 HRCNFG;                 /* 0x20 */
    Uint16 HRPWR;                  /* 0x21 */
    Uint16 rsvd2[4];               /* 0x22 */
    Uint16 HRMSTEP;                /* 0x26 */
    Uint16 rsvd3;                  /* 0x27 */
    Uint16 HRPCTL;                 /* 0x28 */
    Uint16 rsvd4;                  /* 0x29 */
    Uint32 TBPRDM;                 /* 0x2A */
    Uint32 CMPAM;                  /* 0x2C */
    Uint16 rsvd5[2];               /* 0x2E */
    Uint16 DCTRIPSEL;              /* 0x30 */
    Uint16 DCACTL;                 /* 0x31 */
    Uint16 DCBCTL;                 /* 0x32 */
    Uint16 DCFCTL;                 /* 0x33 */
    Uint16 DCCAPCTL;               /* 0x34 */
    Uint16 DCFOFFSET;              /* 0x35 */
    Uint16 DCFOFFSETCNT;           /* 0x36 */
    Uint16 DCFWINDOW;              /* 0x37 */
    Uint16 DCFWINDOWCNT;           /* 0x38 */
    Uint16 DCCAP;                  /* 0x39 */
    Uint16 rsvd6[6];               /* 0x3A */
};

extern volatile struct EPWM_REGS EPwm1Regs;
extern volatile struct EPWM_REGS EPwm2Regs;
extern volatile struct EPWM_REGS EPwm3Regs;
extern volatile struct EPWM_REGS EPwm4Regs;
extern volatile struct EPWM_REGS EPwm5Regs;
extern volatile struct EPWM_REGS EPwm6Regs;
extern volatile struct EPWM_REGS EPwm7Regs;
extern volatile struct EPWM_REGS EPwm8Regs;

#endif
