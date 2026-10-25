/* F2806x_Dma.h: the part of TI's F2806x_Dma.h this firmware uses: DmaRegs at 0x1000, six
   channels. MODE bits 6:5 and 13:12 are written by DMACHxModeConfig from its synce and syncsel
   arguments; the F2806x DMA has no sync, so they are reserved bits here.
   Users: ti/F2806x_Dma.c, drivers/device_init.c (InitDmaAdc).
   name: original, TI's file name. */
#ifndef F2806X_DMA_H
#define F2806X_DMA_H

#include "F2806x_Device.h"

struct MODE_BITS {
    Uint16 PERINTSEL : 5;  /* 4:0 */
    Uint16 SYNCE : 2;      /* 6:5 */
    Uint16 OVRINTE : 1;    /* 7 */
    Uint16 PERINTE : 1;    /* 8 */
    Uint16 CHINTMODE : 1;  /* 9 */
    Uint16 ONESHOT : 1;    /* 10 */
    Uint16 CONTINUOUS : 1; /* 11 */
    Uint16 SYNCSEL : 2;    /* 13:12 */
    Uint16 DATASIZE : 1;   /* 14 */
    Uint16 CHINTE : 1;     /* 15 */
};
union MODE_REG {
    Uint16 all;
    struct MODE_BITS bit;
};

struct CONTROL_BITS {
    Uint16 RUN : 1;         /* 0 */
    Uint16 HALT : 1;        /* 1 */
    Uint16 SOFTRESET : 1;   /* 2 */
    Uint16 PERINTFRC : 1;   /* 3 */
    Uint16 PERINTCLR : 1;   /* 4 */
    Uint16 rsvd2 : 2;       /* 6:5 */
    Uint16 ERRCLR : 1;      /* 7 */
    Uint16 PERINTFLG : 1;   /* 8 */
    Uint16 rsvd4 : 1;       /* 9 */
    Uint16 TRANSFERSTS : 1; /* 10 */
    Uint16 BURSTSTS : 1;    /* 11 */
    Uint16 RUNSTS : 1;      /* 12 */
    Uint16 OVRFLG : 1;      /* 13 */
    Uint16 rsvd5 : 2;       /* 15:14 */
};
union CONTROL_REG {
    Uint16 all;
    struct CONTROL_BITS bit;
};

struct DMACTRL_BITS {
    Uint16 HARDRESET : 1;
    Uint16 PRIORITYRESET : 1;
    Uint16 rsvd1 : 14;
};
union DMACTRL_REG {
    Uint16 all;
    struct DMACTRL_BITS bit;
};

struct DEBUGCTRL_BITS {
    Uint16 rsvd1 : 15;
    Uint16 FREE : 1;
};
union DEBUGCTRL_REG {
    Uint16 all;
    struct DEBUGCTRL_BITS bit;
};

struct BURST_SIZE_BITS {
    Uint16 BURSTSIZE : 5;
    Uint16 rsvd1 : 11;
};
union BURST_SIZE_REG {
    Uint16 all;
    struct BURST_SIZE_BITS bit;
};

struct CH_REGS {
    union MODE_REG MODE;             /* 0 */
    union CONTROL_REG CONTROL;       /* 1 */
    union BURST_SIZE_REG BURST_SIZE; /* 2 */
    Uint16 BURST_COUNT;              /* 3 */
    int16 SRC_BURST_STEP;            /* 4 */
    int16 DST_BURST_STEP;            /* 5 */
    Uint16 TRANSFER_SIZE;            /* 6 */
    Uint16 TRANSFER_COUNT;           /* 7 */
    int16 SRC_TRANSFER_STEP;         /* 8 */
    int16 DST_TRANSFER_STEP;         /* 9 */
    Uint16 SRC_WRAP_SIZE;            /* 10 */
    Uint16 SRC_WRAP_COUNT;           /* 11 */
    int16 SRC_WRAP_STEP;             /* 12 */
    Uint16 DST_WRAP_SIZE;            /* 13 */
    Uint16 DST_WRAP_COUNT;           /* 14 */
    int16 DST_WRAP_STEP;             /* 15 */
    Uint32 SRC_BEG_ADDR_SHADOW;      /* 16 */
    Uint32 SRC_ADDR_SHADOW;          /* 18 */
    Uint32 SRC_BEG_ADDR_ACTIVE;      /* 20 */
    Uint32 SRC_ADDR_ACTIVE;          /* 22 */
    Uint32 DST_BEG_ADDR_SHADOW;      /* 24 */
    Uint32 DST_ADDR_SHADOW;          /* 26 */
    Uint32 DST_BEG_ADDR_ACTIVE;      /* 28 */
    Uint32 DST_ADDR_ACTIVE;          /* 30 */
};

struct DMA_REGS {
    union DMACTRL_REG DMACTRL;     /* 0 */
    union DEBUGCTRL_REG DEBUGCTRL; /* 1 */
    Uint16 REVISION;               /* 2 */
    Uint16 rsvd1;                  /* 3 */
    Uint16 PRIORITYCTRL1;          /* 4 */
    Uint16 rsvd2;                  /* 5 */
    Uint16 PRIORITYSTAT;           /* 6 */
    Uint16 rsvd3[25];              /* 7 */
    struct CH_REGS CH1;            /* 0x20 */
    struct CH_REGS CH2;
    struct CH_REGS CH3;
    struct CH_REGS CH4;
    struct CH_REGS CH5;
    struct CH_REGS CH6;
};

extern volatile struct DMA_REGS DmaRegs;

#endif
