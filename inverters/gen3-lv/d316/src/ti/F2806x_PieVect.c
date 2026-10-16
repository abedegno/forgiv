/* F2806x_PieVect.c: TI's PieVectTableInit (F2806x_PieVect.c), unchanged: the PIE vector
   table's first contents in flash, every entry one of TI's default handlers
   (F2806x_DefaultIsr.c). InitPieVectTable (device_init.c) copies it to the PIE vector table at
   0x0D00; init_system then puts this firmware's nine handlers in their places (TINT2, USER12,
   EPWM3_TZINT, EPWM4_INT, ECAP1_INT and the four SCI vectors).
   Entry points: none (data). Neighbours: F2806x_DefaultIsr.c, drivers/device_init.c.
   name: original (map/filenames.tsv): TI's file and table name.
   at: PieVectTableInit 0x3F3B46 */
#include "F2806x_PieVect.h"
#include "F2806x_DefaultIsr.h"

const struct PIE_VECT_TABLE PieVectTableInit = {
    /* Reserved space */
    PIE_RESERVED,    /*   0 PIE1_RESERVED */
    PIE_RESERVED,    /*   1 PIE2_RESERVED */
    PIE_RESERVED,    /*   2 PIE3_RESERVED */
    PIE_RESERVED,    /*   3 PIE4_RESERVED */
    PIE_RESERVED,    /*   4 PIE5_RESERVED */
    PIE_RESERVED,    /*   5 PIE6_RESERVED */
    PIE_RESERVED,    /*   6 PIE7_RESERVED */
    PIE_RESERVED,    /*   7 PIE8_RESERVED */
    PIE_RESERVED,    /*   8 PIE9_RESERVED */
    PIE_RESERVED,    /*   9 PIE10_RESERVED */
    PIE_RESERVED,    /*  10 PIE11_RESERVED */
    PIE_RESERVED,    /*  11 PIE12_RESERVED */
    PIE_RESERVED,    /*  12 PIE13_RESERVED */

    /* Non-peripheral interrupts */
    INT13_ISR,       /*  13 TINT1 */
    INT14_ISR,       /*  14 TINT2 */
    DATALOG_ISR,     /*  15 DATALOG */
    RTOSINT_ISR,     /*  16 RTOSINT */
    EMUINT_ISR,      /*  17 EMUINT */
    NMI_ISR,         /*  18 NMI */
    ILLEGAL_ISR,     /*  19 ILLEGAL */
    USER1_ISR,       /*  20 USER1 */
    USER2_ISR,       /*  21 USER2 */
    USER3_ISR,       /*  22 USER3 */
    USER4_ISR,       /*  23 USER4 */
    USER5_ISR,       /*  24 USER5 */
    USER6_ISR,       /*  25 USER6 */
    USER7_ISR,       /*  26 USER7 */
    USER8_ISR,       /*  27 USER8 */
    USER9_ISR,       /*  28 USER9 */
    USER10_ISR,      /*  29 USER10 */
    USER11_ISR,      /*  30 USER11 */
    USER12_ISR,      /*  31 USER12 */

    /* Group 1 PIE Peripheral Vectors */
    ADCINT1_ISR,     /*  32 ADCINT1 */
    ADCINT2_ISR,     /*  33 ADCINT2 */
    rsvd_ISR,        /*  34 rsvd1_3 */
    XINT1_ISR,       /*  35 XINT1 */
    XINT2_ISR,       /*  36 XINT2 */
    ADCINT9_ISR,     /*  37 ADCINT9 */
    TINT0_ISR,       /*  38 TINT0 */
    WAKEINT_ISR,     /*  39 WAKEINT */

    /* Group 2 PIE Peripheral Vectors */
    EPWM1_TZINT_ISR, /*  40 EPWM1_TZINT */
    EPWM2_TZINT_ISR, /*  41 EPWM2_TZINT */
    EPWM3_TZINT_ISR, /*  42 EPWM3_TZINT */
    EPWM4_TZINT_ISR, /*  43 EPWM4_TZINT */
    EPWM5_TZINT_ISR, /*  44 EPWM5_TZINT */
    EPWM6_TZINT_ISR, /*  45 EPWM6_TZINT */
    EPWM7_TZINT_ISR, /*  46 EPWM7_TZINT */
    EPWM8_TZINT_ISR, /*  47 EPWM8_TZINT */

    /* Group 3 PIE Peripheral Vectors */
    EPWM1_INT_ISR,   /*  48 EPWM1_INT */
    EPWM2_INT_ISR,   /*  49 EPWM2_INT */
    EPWM3_INT_ISR,   /*  50 EPWM3_INT */
    EPWM4_INT_ISR,   /*  51 EPWM4_INT */
    EPWM5_INT_ISR,   /*  52 EPWM5_INT */
    EPWM6_INT_ISR,   /*  53 EPWM6_INT */
    EPWM7_INT_ISR,   /*  54 EPWM7_INT */
    EPWM8_INT_ISR,   /*  55 EPWM8_INT */

    /* Group 4 PIE Peripheral Vectors */
    ECAP1_INT_ISR,   /*  56 ECAP1_INT */
    ECAP2_INT_ISR,   /*  57 ECAP2_INT */
    ECAP3_INT_ISR,   /*  58 ECAP3_INT */
    rsvd_ISR,        /*  59 rsvd4_4 */
    rsvd_ISR,        /*  60 rsvd4_5 */
    rsvd_ISR,        /*  61 rsvd4_6 */
    HRCAP1_INT_ISR,  /*  62 HRCAP1_INT */
    HRCAP2_INT_ISR,  /*  63 HRCAP2_INT */

    /* Group 5 PIE Peripheral Vectors */
    EQEP1_INT_ISR,   /*  64 EQEP1_INT */
    EQEP2_INT_ISR,   /*  65 EQEP2_INT */
    rsvd_ISR,        /*  66 rsvd5_3 */
    HRCAP3_INT_ISR,  /*  67 HRCAP3_INT */
    HRCAP4_INT_ISR,  /*  68 HRCAP4_INT */
    rsvd_ISR,        /*  69 rsvd5_6 */
    rsvd_ISR,        /*  70 rsvd5_7 */
    USB0_INT_ISR,    /*  71 USB0_INT */

    /* Group 6 PIE Peripheral Vectors */
    SPIRXINTA_ISR,   /*  72 SPIRXINTA */
    SPITXINTA_ISR,   /*  73 SPITXINTA */
    SPIRXINTB_ISR,   /*  74 SPIRXINTB */
    SPITXINTB_ISR,   /*  75 SPITXINTB */
    MRINTA_ISR,      /*  76 MRINTA */
    MXINTA_ISR,      /*  77 MXINTA */
    rsvd_ISR,        /*  78 rsvd6_7 */
    rsvd_ISR,        /*  79 rsvd6_8 */

    /* Group 7 PIE Peripheral Vectors */
    DINTCH1_ISR,     /*  80 DINTCH1 */
    DINTCH2_ISR,     /*  81 DINTCH2 */
    DINTCH3_ISR,     /*  82 DINTCH3 */
    DINTCH4_ISR,     /*  83 DINTCH4 */
    DINTCH5_ISR,     /*  84 DINTCH5 */
    DINTCH6_ISR,     /*  85 DINTCH6 */
    rsvd_ISR,        /*  86 rsvd7_7 */
    rsvd_ISR,        /*  87 rsvd7_8 */

    /* Group 8 PIE Peripheral Vectors */
    I2CINT1A_ISR,    /*  88 I2CINT1A */
    I2CINT2A_ISR,    /*  89 I2CINT2A */
    rsvd_ISR,        /*  90 rsvd8_3 */
    rsvd_ISR,        /*  91 rsvd8_4 */
    rsvd_ISR,        /*  92 rsvd8_5 */
    rsvd_ISR,        /*  93 rsvd8_6 */
    rsvd_ISR,        /*  94 rsvd8_7 */
    rsvd_ISR,        /*  95 rsvd8_8 */

    /* Group 9 PIE Peripheral Vectors */
    SCIRXINTA_ISR,   /*  96 SCIRXINTA */
    SCITXINTA_ISR,   /*  97 SCITXINTA */
    SCIRXINTB_ISR,   /*  98 SCIRXINTB */
    SCITXINTB_ISR,   /*  99 SCITXINTB */
    ECAN0INTA_ISR,   /* 100 ECAN0INTA */
    ECAN1INTA_ISR,   /* 101 ECAN1INTA */
    rsvd_ISR,        /* 102 rsvd9_7 */
    rsvd_ISR,        /* 103 rsvd9_8 */

    /* Group 10 PIE Peripheral Vectors */
    rsvd_ISR,        /* 104 rsvd10_1 */
    rsvd_ISR,        /* 105 rsvd10_2 */
    ADCINT3_ISR,     /* 106 ADCINT3 */
    ADCINT4_ISR,     /* 107 ADCINT4 */
    ADCINT5_ISR,     /* 108 ADCINT5 */
    ADCINT6_ISR,     /* 109 ADCINT6 */
    ADCINT7_ISR,     /* 110 ADCINT7 */
    ADCINT8_ISR,     /* 111 ADCINT8 */

    /* Group 11 PIE Peripheral Vectors */
    CLA1_INT1_ISR,   /* 112 CLA1_INT1 */
    CLA1_INT2_ISR,   /* 113 CLA1_INT2 */
    CLA1_INT3_ISR,   /* 114 CLA1_INT3 */
    CLA1_INT4_ISR,   /* 115 CLA1_INT4 */
    CLA1_INT5_ISR,   /* 116 CLA1_INT5 */
    CLA1_INT6_ISR,   /* 117 CLA1_INT6 */
    CLA1_INT7_ISR,   /* 118 CLA1_INT7 */
    CLA1_INT8_ISR,   /* 119 CLA1_INT8 */

    /* Group 12 PIE Peripheral Vectors */
    XINT3_ISR,       /* 120 XINT3 */
    rsvd_ISR,        /* 121 rsvd12_2 */
    rsvd_ISR,        /* 122 rsvd12_3 */
    rsvd_ISR,        /* 123 rsvd12_4 */
    rsvd_ISR,        /* 124 rsvd12_5 */
    rsvd_ISR,        /* 125 rsvd12_6 */
    LVF_ISR,         /* 126 LVF */
    LUF_ISR          /* 127 LUF */
};
