/* F2806x_PieVect.h: TI's PIE vector table, 128 interrupt vectors (0x0D00 .. 0x0DFF in the
   device), and PieVectTableInit, its first contents in flash (F2806x_PieVect.c).
   Users: drivers/device_init.c (InitPieVectTable, init_system's vectors), ti/F2806x_PieVect.c.
   name: original, TI's file name. */
#ifndef F2806x_PIE_VECT_H
#define F2806x_PIE_VECT_H

typedef interrupt void (*PINT)(void);

struct PIE_VECT_TABLE {
    /* Reset is never fetched from this table: it always comes from the boot ROM. */
    PINT PIE1_RESERVED;
    PINT PIE2_RESERVED;
    PINT PIE3_RESERVED;
    PINT PIE4_RESERVED;
    PINT PIE5_RESERVED;
    PINT PIE6_RESERVED;
    PINT PIE7_RESERVED;
    PINT PIE8_RESERVED;
    PINT PIE9_RESERVED;
    PINT PIE10_RESERVED;
    PINT PIE11_RESERVED;
    PINT PIE12_RESERVED;
    PINT PIE13_RESERVED;

    /* Non-peripheral interrupts */
    PINT TINT1;
    PINT TINT2;
    PINT DATALOG;
    PINT RTOSINT;
    PINT EMUINT;
    PINT NMI;
    PINT ILLEGAL;
    PINT USER1;
    PINT USER2;
    PINT USER3;
    PINT USER4;
    PINT USER5;
    PINT USER6;
    PINT USER7;
    PINT USER8;
    PINT USER9;
    PINT USER10;
    PINT USER11;
    PINT USER12;

    /* Group 1 PIE Peripheral Vectors */
    PINT ADCINT1;
    PINT ADCINT2;
    PINT rsvd1_3;
    PINT XINT1;
    PINT XINT2;
    PINT ADCINT9;
    PINT TINT0;
    PINT WAKEINT;

    /* Group 2 PIE Peripheral Vectors */
    PINT EPWM1_TZINT;
    PINT EPWM2_TZINT;
    PINT EPWM3_TZINT;
    PINT EPWM4_TZINT;
    PINT EPWM5_TZINT;
    PINT EPWM6_TZINT;
    PINT EPWM7_TZINT;
    PINT EPWM8_TZINT;

    /* Group 3 PIE Peripheral Vectors */
    PINT EPWM1_INT;
    PINT EPWM2_INT;
    PINT EPWM3_INT;
    PINT EPWM4_INT;
    PINT EPWM5_INT;
    PINT EPWM6_INT;
    PINT EPWM7_INT;
    PINT EPWM8_INT;

    /* Group 4 PIE Peripheral Vectors */
    PINT ECAP1_INT;
    PINT ECAP2_INT;
    PINT ECAP3_INT;
    PINT rsvd4_4;
    PINT rsvd4_5;
    PINT rsvd4_6;
    PINT HRCAP1_INT;
    PINT HRCAP2_INT;

    /* Group 5 PIE Peripheral Vectors */
    PINT EQEP1_INT;
    PINT EQEP2_INT;
    PINT rsvd5_3;
    PINT HRCAP3_INT;
    PINT HRCAP4_INT;
    PINT rsvd5_6;
    PINT rsvd5_7;
    PINT USB0_INT;

    /* Group 6 PIE Peripheral Vectors */
    PINT SPIRXINTA;
    PINT SPITXINTA;
    PINT SPIRXINTB;
    PINT SPITXINTB;
    PINT MRINTA;
    PINT MXINTA;
    PINT rsvd6_7;
    PINT rsvd6_8;

    /* Group 7 PIE Peripheral Vectors */
    PINT DINTCH1;
    PINT DINTCH2;
    PINT DINTCH3;
    PINT DINTCH4;
    PINT DINTCH5;
    PINT DINTCH6;
    PINT rsvd7_7;
    PINT rsvd7_8;

    /* Group 8 PIE Peripheral Vectors */
    PINT I2CINT1A;
    PINT I2CINT2A;
    PINT rsvd8_3;
    PINT rsvd8_4;
    PINT rsvd8_5;
    PINT rsvd8_6;
    PINT rsvd8_7;
    PINT rsvd8_8;

    /* Group 9 PIE Peripheral Vectors */
    PINT SCIRXINTA;
    PINT SCITXINTA;
    PINT SCIRXINTB;
    PINT SCITXINTB;
    PINT ECAN0INTA;
    PINT ECAN1INTA;
    PINT rsvd9_7;
    PINT rsvd9_8;

    /* Group 10 PIE Peripheral Vectors */
    PINT rsvd10_1;
    PINT rsvd10_2;
    PINT ADCINT3;
    PINT ADCINT4;
    PINT ADCINT5;
    PINT ADCINT6;
    PINT ADCINT7;
    PINT ADCINT8;

    /* Group 11 PIE Peripheral Vectors */
    PINT CLA1_INT1;
    PINT CLA1_INT2;
    PINT CLA1_INT3;
    PINT CLA1_INT4;
    PINT CLA1_INT5;
    PINT CLA1_INT6;
    PINT CLA1_INT7;
    PINT CLA1_INT8;

    /* Group 12 PIE Peripheral Vectors */
    PINT XINT3;
    PINT rsvd12_2;
    PINT rsvd12_3;
    PINT rsvd12_4;
    PINT rsvd12_5;
    PINT rsvd12_6;
    PINT LVF;
    PINT LUF;
};

extern volatile struct PIE_VECT_TABLE PieVectTable;
extern const struct PIE_VECT_TABLE PieVectTableInit;

#endif
