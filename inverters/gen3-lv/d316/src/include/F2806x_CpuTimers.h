/* F2806x_CpuTimers.h: TI's CPU timer registers (CpuTimer2Regs at 0x0C10) and TI's
   CPUTIMER_VARS, which ConfigCpuTimer fills for timer 2 (CpuTimer2), the RTOS's 1 ms tick.
   Users: drivers/device_init.c (timer2_init, ConfigCpuTimer).
   name: original, TI's file name; only timer 2 is declared. */
#ifndef F2806X_CPUTIMERS_H
#define F2806X_CPUTIMERS_H

#include "F2806x_Device.h"

struct TIM_REG {
    Uint16 LSW;
    Uint16 MSW;
};
union TIM_GROUP {
    Uint32 all;
    struct TIM_REG half;
};

struct PRD_REG {
    Uint16 LSW;
    Uint16 MSW;
};
union PRD_GROUP {
    Uint32 all;
    struct PRD_REG half;
};

struct TCR_BITS {
    Uint16 rsvd1 : 4;
    Uint16 TSS : 1;
    Uint16 TRB : 1;
    Uint16 rsvd2 : 4;
    Uint16 SOFT : 1;
    Uint16 FREE : 1;
    Uint16 rsvd3 : 2;
    Uint16 TIE : 1;
    Uint16 TIF : 1;
};
union TCR_REG {
    Uint16 all;
    struct TCR_BITS bit;
};

struct TPR_BITS {
    Uint16 TDDR : 8;
    Uint16 PSC : 8;
};
union TPR_REG {
    Uint16 all;
    struct TPR_BITS bit;
};

struct TPRH_BITS {
    Uint16 TDDRH : 8;
    Uint16 PSCH : 8;
};
union TPRH_REG {
    Uint16 all;
    struct TPRH_BITS bit;
};

struct CPUTIMER_REGS {
    union TIM_GROUP TIM; /* 0 */
    union PRD_GROUP PRD; /* 2 */
    union TCR_REG TCR;   /* 4 */
    Uint16 rsvd1;        /* 5 */
    union TPR_REG TPR;   /* 6 */
    union TPRH_REG TPRH; /* 7 */
};

struct CPUTIMER_VARS {
    volatile struct CPUTIMER_REGS *RegsAddr;
    Uint32 InterruptCount;
    float CPUFreqInMHz;
    float PeriodInUSec;
};

extern volatile struct CPUTIMER_REGS CpuTimer2Regs;
extern struct CPUTIMER_VARS CpuTimer2;

#endif
