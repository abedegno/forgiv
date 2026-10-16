/* F2806x_SysCtrl.h: TI's system control (SysCtrlRegs at 0x7010: clocks, PLL, watchdog,
   peripheral clocks), the flash registers FlashRegs (as TI has them in this file) and the code
   security module (CsmRegs at 0x0AE0, the password locations CsmPwl at 0x3F7FF8). Registers this
   firmware does not use bit by bit are plain words; the offsets are TI's.
   Users: drivers/device_init.c, ti/F2806x_SysCtrl.c (InitFlash), boot/ (CsmUnlock, the update
   routine's clock and flash set-up), rtos/os_main.c (start).
   name: original, TI's file name. */
#ifndef F2806X_SYSCTRL_H
#define F2806X_SYSCTRL_H

#include "F2806x_Device.h"

struct XCLK_BITS {
    Uint16 XCLKOUTDIV : 2;
    Uint16 rsvd1 : 4;
    Uint16 XCLKINSEL : 1;
    Uint16 rsvd2 : 9;
};
union XCLK_REG {
    Uint16 all;
    struct XCLK_BITS bit;
};

struct PLLSTS_BITS {
    Uint16 PLLLOCKS : 1;
    Uint16 rsvd1 : 1;
    Uint16 PLLOFF : 1;
    Uint16 MCLKSTS : 1;
    Uint16 MCLKCLR : 1;
    Uint16 OSCOFF : 1;
    Uint16 MCLKOFF : 1;
    Uint16 DIVSEL : 2;
    Uint16 rsvd2 : 6;
    Uint16 NORMRDYE : 1;
};
union PLLSTS_REG {
    Uint16 all;
    struct PLLSTS_BITS bit;
};

struct CLKCTL_BITS {
    Uint16 OSCCLKSRCSEL : 1;
    Uint16 OSCCLKSRC2SEL : 1;
    Uint16 WDCLKSRCSEL : 1;
    Uint16 TMR2CLKSRCSEL : 2;
    Uint16 TMR2CLKPRESCALE : 3;
    Uint16 INTOSC1OFF : 1;
    Uint16 INTOSC1HALTI : 1;
    Uint16 INTOSC2OFF : 1;
    Uint16 INTOSC2HALTI : 1;
    Uint16 WDHALTI : 1;
    Uint16 XCLKINOFF : 1;
    Uint16 XTALOSCOFF : 1;
    Uint16 NMIRESETSEL : 1;
};
union CLKCTL_REG {
    Uint16 all;
    struct CLKCTL_BITS bit;
};

struct LOSPCP_BITS {
    Uint16 LSPCLK : 3;
    Uint16 rsvd1 : 13;
};
union LOSPCP_REG {
    Uint16 all;
    struct LOSPCP_BITS bit;
};

struct PCLKCR0_BITS {
    Uint16 HRPWMENCLK : 1;
    Uint16 rsvd1 : 1;
    Uint16 TBCLKSYNC : 1;
    Uint16 ADCENCLK : 1;
    Uint16 I2CAENCLK : 1;
    Uint16 rsvd2 : 3;
    Uint16 SPIAENCLK : 1;
    Uint16 SPIBENCLK : 1;
    Uint16 SCIAENCLK : 1;
    Uint16 SCIBENCLK : 1;
    Uint16 MCBSPAENCLK : 1;
    Uint16 rsvd3 : 1;
    Uint16 ECANAENCLK : 1;
    Uint16 rsvd4 : 1;
};
union PCLKCR0_REG {
    Uint16 all;
    struct PCLKCR0_BITS bit;
};

struct PCLKCR1_BITS {
    Uint16 EPWM1ENCLK : 1;
    Uint16 EPWM2ENCLK : 1;
    Uint16 EPWM3ENCLK : 1;
    Uint16 EPWM4ENCLK : 1;
    Uint16 EPWM5ENCLK : 1;
    Uint16 EPWM6ENCLK : 1;
    Uint16 EPWM7ENCLK : 1;
    Uint16 EPWM8ENCLK : 1;
    Uint16 ECAP1ENCLK : 1;
    Uint16 ECAP2ENCLK : 1;
    Uint16 ECAP3ENCLK : 1;
    Uint16 rsvd1 : 3;
    Uint16 EQEP1ENCLK : 1;
    Uint16 EQEP2ENCLK : 1;
};
union PCLKCR1_REG {
    Uint16 all;
    struct PCLKCR1_BITS bit;
};

struct PCLKCR3_BITS {
    Uint16 COMP1ENCLK : 1;
    Uint16 COMP2ENCLK : 1;
    Uint16 COMP3ENCLK : 1;
    Uint16 rsvd1 : 5;
    Uint16 CPUTIMER0ENCLK : 1;
    Uint16 CPUTIMER1ENCLK : 1;
    Uint16 CPUTIMER2ENCLK : 1;
    Uint16 DMAENCLK : 1;
    Uint16 rsvd2 : 2;
    Uint16 CLA1ENCLK : 1;
    Uint16 USB0ENCLK : 1;
};
union PCLKCR3_REG {
    Uint16 all;
    struct PCLKCR3_BITS bit;
};

struct PLLCR_BITS {
    Uint16 DIV : 5;
    Uint16 rsvd1 : 11;
};
union PLLCR_REG {
    Uint16 all;
    struct PLLCR_BITS bit;
};

struct SYS_CTRL_REGS {
    union XCLK_REG XCLK;       /* 0x00 */
    union PLLSTS_REG PLLSTS;   /* 0x01 */
    union CLKCTL_REG CLKCTL;   /* 0x02 */
    Uint16 PLLLOCKPRD;         /* 0x03 */
    Uint16 INTOSC1TRIM;        /* 0x04 */
    Uint16 rsvd1;              /* 0x05 */
    Uint16 INTOSC2TRIM;        /* 0x06 */
    Uint16 rsvd2[2];           /* 0x07 */
    Uint16 PCLKCR2;            /* 0x09 */
    Uint16 rsvd3;              /* 0x0A */
    union LOSPCP_REG LOSPCP;   /* 0x0B */
    union PCLKCR0_REG PCLKCR0; /* 0x0C */
    union PCLKCR1_REG PCLKCR1; /* 0x0D */
    Uint16 LPMCR0;             /* 0x0E */
    Uint16 rsvd4;              /* 0x0F */
    union PCLKCR3_REG PCLKCR3; /* 0x10 */
    union PLLCR_REG PLLCR;     /* 0x11 */
    Uint16 SCSR;               /* 0x12 */
    Uint16 WDCNTR;             /* 0x13 */
    Uint16 rsvd5;              /* 0x14 */
    Uint16 WDKEY;              /* 0x15 */
    Uint16 rsvd6[3];           /* 0x16 */
    Uint16 WDCR;               /* 0x19 */
};

extern volatile struct SYS_CTRL_REGS SysCtrlRegs;

/* Code security module */
struct CSMSCR_BITS {
    Uint16 SECURE : 1;
    Uint16 rsvd1 : 14;
    Uint16 FORCESEC : 1;
};
union CSMSCR_REG {
    Uint16 all;
    struct CSMSCR_BITS bit;
};

struct CSM_REGS {
    Uint16 KEY0; /* 0 */
    Uint16 KEY1;
    Uint16 KEY2;
    Uint16 KEY3;
    Uint16 KEY4;
    Uint16 KEY5;
    Uint16 KEY6;
    Uint16 KEY7;
    Uint16 rsvd1; /* 8 */
    Uint16 rsvd2;
    Uint16 rsvd3;
    Uint16 rsvd4;
    Uint16 rsvd5;
    Uint16 rsvd6;
    Uint16 rsvd7;
    union CSMSCR_REG CSMSCR; /* 15 */
};

struct CSM_PWL {
    Uint16 PSWD0;
    Uint16 PSWD1;
    Uint16 PSWD2;
    Uint16 PSWD3;
    Uint16 PSWD4;
    Uint16 PSWD5;
    Uint16 PSWD6;
    Uint16 PSWD7;
};

extern volatile struct CSM_REGS CsmRegs;
extern volatile struct CSM_PWL CsmPwl;

/* Flash wait states and pipeline (FlashRegs at 0x0A80) */
struct FOPT_BITS {
    Uint16 ENPIPE : 1;
    Uint16 rsvd : 15;
};
union FOPT_REG {
    Uint16 all;
    struct FOPT_BITS bit;
};

struct FSTDBYWAIT_BITS {
    Uint16 STDBYWAIT : 9;
    Uint16 rsvd : 7;
};
union FSTDBYWAIT_REG {
    Uint16 all;
    struct FSTDBYWAIT_BITS bit;
};

struct FACTIVEWAIT_BITS {
    Uint16 ACTIVEWAIT : 9;
    Uint16 rsvd : 7;
};
union FACTIVEWAIT_REG {
    Uint16 all;
    struct FACTIVEWAIT_BITS bit;
};

struct FBANKWAIT_BITS {
    Uint16 RANDWAIT : 4;
    Uint16 rsvd1 : 4;
    Uint16 PAGEWAIT : 4;
    Uint16 rsvd2 : 4;
};
union FBANKWAIT_REG {
    Uint16 all;
    struct FBANKWAIT_BITS bit;
};

struct FOTPWAIT_BITS {
    Uint16 OTPWAIT : 5;
    Uint16 rsvd : 11;
};
union FOTPWAIT_REG {
    Uint16 all;
    struct FOTPWAIT_BITS bit;
};

struct FLASH_REGS {
    union FOPT_REG FOPT;               /* 0 */
    Uint16 rsvd1;                      /* 1 */
    Uint16 FPWR;                       /* 2 */
    Uint16 FSTATUS;                    /* 3 */
    union FSTDBYWAIT_REG FSTDBYWAIT;   /* 4 */
    union FACTIVEWAIT_REG FACTIVEWAIT; /* 5 */
    union FBANKWAIT_REG FBANKWAIT;     /* 6 */
    union FOTPWAIT_REG FOTPWAIT;       /* 7 */
};

extern volatile struct FLASH_REGS FlashRegs;

#endif
