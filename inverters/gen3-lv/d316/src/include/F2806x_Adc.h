/* F2806x_Adc.h: TI's ADC register layout (F2806x_Adc.h): AdcRegs at 0x7100 and the result
   registers AdcResult at 0x0B00. Users: drivers/device_init.c (InitAdc, the TI calibration
   functions, adc_offset_measure) and the measurement code that reads AdcResult.
   name: original, TI's file name; only the registers this firmware uses have bit fields. */
#ifndef F2806X_ADC_H
#define F2806X_ADC_H

#include "F2806x_Device.h"

struct ADCCTL1_BITS {
    Uint16 TEMPCONV : 1;
    Uint16 VREFLOCONV : 1;
    Uint16 INTPULSEPOS : 1;
    Uint16 ADCREFSEL : 1;
    Uint16 rsvd1 : 1;
    Uint16 ADCREFPWD : 1;
    Uint16 ADCBGPWD : 1;
    Uint16 ADCPWDN : 1;
    Uint16 ADCBSYCHN : 5;
    Uint16 ADCBSY : 1;
    Uint16 ADCENABLE : 1;
    Uint16 RESET : 1;
};
union ADCCTL1_REG {
    Uint16 all;
    struct ADCCTL1_BITS bit;
};

struct ADCCTL2_BITS {
    Uint16 CLKDIV2EN : 1;
    Uint16 ADCNONOVERLAP : 1;
    Uint16 CLKDIV4EN : 1;
    Uint16 rsvd1 : 13;
};
union ADCCTL2_REG {
    Uint16 all;
    struct ADCCTL2_BITS bit;
};

struct ADCINT_BITS {
    Uint16 ADCINT1 : 1;
    Uint16 ADCINT2 : 1;
    Uint16 ADCINT3 : 1;
    Uint16 ADCINT4 : 1;
    Uint16 ADCINT5 : 1;
    Uint16 ADCINT6 : 1;
    Uint16 ADCINT7 : 1;
    Uint16 ADCINT8 : 1;
    Uint16 ADCINT9 : 1;
    Uint16 rsvd1 : 7;
};
union ADCINT_REG {
    Uint16 all;
    struct ADCINT_BITS bit;
};

struct INTSEL1N2_BITS {
    Uint16 INT1SEL : 5;
    Uint16 INT1E : 1;
    Uint16 INT1CONT : 1;
    Uint16 rsvd1 : 1;
    Uint16 INT2SEL : 5;
    Uint16 INT2E : 1;
    Uint16 INT2CONT : 1;
    Uint16 rsvd2 : 1;
};
union INTSEL1N2_REG {
    Uint16 all;
    struct INTSEL1N2_BITS bit;
};

struct SOCPRICTL_BITS {
    Uint16 SOCPRIORITY : 5;
    Uint16 RRPOINTER : 6;
    Uint16 rsvd1 : 4;
    Uint16 ONESHOT : 1;
};
union SOCPRICTL_REG {
    Uint16 all;
    struct SOCPRICTL_BITS bit;
};

struct ADCINTSOCSEL1_BITS {
    Uint16 SOC0 : 2;
    Uint16 SOC1 : 2;
    Uint16 SOC2 : 2;
    Uint16 SOC3 : 2;
    Uint16 SOC4 : 2;
    Uint16 SOC5 : 2;
    Uint16 SOC6 : 2;
    Uint16 SOC7 : 2;
};
union ADCINTSOCSEL1_REG {
    Uint16 all;
    struct ADCINTSOCSEL1_BITS bit;
};

struct ADCINTSOCSEL2_BITS {
    Uint16 SOC8 : 2;
    Uint16 SOC9 : 2;
    Uint16 SOC10 : 2;
    Uint16 SOC11 : 2;
    Uint16 SOC12 : 2;
    Uint16 SOC13 : 2;
    Uint16 SOC14 : 2;
    Uint16 SOC15 : 2;
};
union ADCINTSOCSEL2_REG {
    Uint16 all;
    struct ADCINTSOCSEL2_BITS bit;
};

struct ADCSOC_BITS {
    Uint16 SOC0 : 1;
    Uint16 SOC1 : 1;
    Uint16 SOC2 : 1;
    Uint16 SOC3 : 1;
    Uint16 SOC4 : 1;
    Uint16 SOC5 : 1;
    Uint16 SOC6 : 1;
    Uint16 SOC7 : 1;
    Uint16 SOC8 : 1;
    Uint16 SOC9 : 1;
    Uint16 SOC10 : 1;
    Uint16 SOC11 : 1;
    Uint16 SOC12 : 1;
    Uint16 SOC13 : 1;
    Uint16 SOC14 : 1;
    Uint16 SOC15 : 1;
};
union ADCSOC_REG {
    Uint16 all;
    struct ADCSOC_BITS bit;
};

struct ADCSOCxCTL_BITS {
    Uint16 ACQPS : 6;
    Uint16 CHSEL : 4;
    Uint16 rsvd1 : 1;
    Uint16 TRIGSEL : 5;
};
union ADCSOCxCTL_REG {
    Uint16 all;
    struct ADCSOCxCTL_BITS bit;
};

struct ADCOFFTRIM_BITS {
    int16 OFFTRIM : 9;
    Uint16 rsvd1 : 7;
};
union ADCOFFTRIM_REG {
    Uint16 all;
    struct ADCOFFTRIM_BITS bit;
};

struct ADC_REGS {
    union ADCCTL1_REG ADCCTL1;             /* 0x00 */
    union ADCCTL2_REG ADCCTL2;             /* 0x01 */
    Uint16 rsvd1[2];                       /* 0x02 */
    union ADCINT_REG ADCINTFLG;            /* 0x04 */
    union ADCINT_REG ADCINTFLGCLR;         /* 0x05 */
    union ADCINT_REG ADCINTOVF;            /* 0x06 */
    union ADCINT_REG ADCINTOVFCLR;         /* 0x07 */
    union INTSEL1N2_REG INTSEL1N2;         /* 0x08 */
    Uint16 INTSEL3N4;                      /* 0x09 */
    Uint16 INTSEL5N6;                      /* 0x0A */
    Uint16 INTSEL7N8;                      /* 0x0B */
    Uint16 INTSEL9N10;                     /* 0x0C */
    Uint16 rsvd2[3];                       /* 0x0D */
    union SOCPRICTL_REG SOCPRICTL;         /* 0x10 */
    Uint16 rsvd3;                          /* 0x11 */
    Uint16 ADCSAMPLEMODE;                  /* 0x12 */
    Uint16 rsvd4;                          /* 0x13 */
    union ADCINTSOCSEL1_REG ADCINTSOCSEL1; /* 0x14 */
    union ADCINTSOCSEL2_REG ADCINTSOCSEL2; /* 0x15 */
    Uint16 rsvd5[2];                       /* 0x16 */
    union ADCSOC_REG ADCSOCFLG1;           /* 0x18 */
    Uint16 rsvd6;                          /* 0x19 */
    union ADCSOC_REG ADCSOCFRC1;           /* 0x1A */
    Uint16 rsvd7;                          /* 0x1B */
    union ADCSOC_REG ADCSOCOVF1;           /* 0x1C */
    Uint16 rsvd8;                          /* 0x1D */
    union ADCSOC_REG ADCSOCOVFCLR1;        /* 0x1E */
    Uint16 rsvd9;                          /* 0x1F */
    union ADCSOCxCTL_REG ADCSOC0CTL;       /* 0x20 */
    union ADCSOCxCTL_REG ADCSOC1CTL;
    union ADCSOCxCTL_REG ADCSOC2CTL;
    union ADCSOCxCTL_REG ADCSOC3CTL;
    union ADCSOCxCTL_REG ADCSOC4CTL;
    union ADCSOCxCTL_REG ADCSOC5CTL;
    union ADCSOCxCTL_REG ADCSOC6CTL;
    union ADCSOCxCTL_REG ADCSOC7CTL;
    union ADCSOCxCTL_REG ADCSOC8CTL;
    union ADCSOCxCTL_REG ADCSOC9CTL;
    union ADCSOCxCTL_REG ADCSOC10CTL;
    union ADCSOCxCTL_REG ADCSOC11CTL;
    union ADCSOCxCTL_REG ADCSOC12CTL;
    union ADCSOCxCTL_REG ADCSOC13CTL;
    union ADCSOCxCTL_REG ADCSOC14CTL;
    union ADCSOCxCTL_REG ADCSOC15CTL;
    Uint16 rsvd10[16];               /* 0x30 */
    Uint16 ADCREFTRIM;               /* 0x40 */
    union ADCOFFTRIM_REG ADCOFFTRIM; /* 0x41 */
    Uint16 rsvd11[10];               /* 0x42 */
    Uint16 COMPHYSTCTL;              /* 0x4C */
    Uint16 rsvd12[2];                /* 0x4D */
    Uint16 ADCREV;                   /* 0x4F */
};

struct ADC_RESULT_REGS {
    Uint16 ADCRESULT0;
    Uint16 ADCRESULT1;
    Uint16 ADCRESULT2;
    Uint16 ADCRESULT3;
    Uint16 ADCRESULT4;
    Uint16 ADCRESULT5;
    Uint16 ADCRESULT6;
    Uint16 ADCRESULT7;
    Uint16 ADCRESULT8;
    Uint16 ADCRESULT9;
    Uint16 ADCRESULT10;
    Uint16 ADCRESULT11;
    Uint16 ADCRESULT12;
    Uint16 ADCRESULT13;
    Uint16 ADCRESULT14;
    Uint16 ADCRESULT15;
};

extern volatile struct ADC_REGS AdcRegs;
extern volatile struct ADC_RESULT_REGS AdcResult;

#endif
