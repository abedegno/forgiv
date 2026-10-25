/* cm3_inline.h: CMSIS core_cm3.h V1.30 (StdPeriph V3.5.0): SysTick_Config and NVIC_SetPriority
   as the application inlines them (OS_CPU_SysTickInit), with only the register blocks they use.
   name: original (CMSIS). */
#ifndef CM3_INLINE_H
#define CM3_INLINE_H

typedef struct {
    volatile unsigned int CTRL;
    volatile unsigned int LOAD;
    volatile unsigned int VAL;
    volatile const unsigned int CALIB;
} SysTick_Type;
typedef struct {
    volatile const unsigned int CPUID;
    volatile unsigned int ICSR;
    volatile unsigned int VTOR;
    volatile unsigned int AIRCR;
    volatile unsigned int SCR;
    volatile unsigned int CCR;
    volatile unsigned char SHP[12];
} SCB_Type;
typedef struct {
    volatile unsigned int ISER[8];
    unsigned int RESERVED0[24];
    volatile unsigned int ICER[8];
    unsigned int RSERVED1[24];
    volatile unsigned int ISPR[8];
    unsigned int RESERVED2[24];
    volatile unsigned int ICPR[8];
    unsigned int RESERVED3[24];
    volatile unsigned int IABR[8];
    unsigned int RESERVED4[56];
    volatile unsigned char IP[240];
} NVIC_Type;
#define SysTick ((SysTick_Type *)(0xE000E000 + 0x0010))
#define SCB ((SCB_Type *)(0xE000E000 + 0x0D00))
#define NVIC ((NVIC_Type *)(0xE000E000 + 0x0100))

/* CMSIS: sets an exception (negative IRQn) or interrupt priority, 4 priority bits. */
static __inline void NVIC_SetPriority(int IRQn, unsigned int priority) {
    if (IRQn < 0) {
        SCB->SHP[((unsigned int)(IRQn) & 0xF) - 4] = ((priority << (8 - 4)) & 0xff);
    } else {
        NVIC->IP[(unsigned int)(IRQn)] = ((priority << (8 - 4)) & 0xff);
    }
}

/* CMSIS: SysTick reload = ticks - 1, lowest priority, counter cleared, processor clock,
   interrupt on. */
static __inline unsigned int SysTick_Config(unsigned int ticks) {
    if (ticks > (0xFFFFFFul << 0))
        return (1);

    SysTick->LOAD = (ticks & (0xFFFFFFul << 0)) - 1;
    NVIC_SetPriority(-1, (1 << 4) - 1);
    SysTick->VAL = 0;
    SysTick->CTRL = (1ul << 2) | (1ul << 1) | (1ul << 0);
    return (0);
}

#endif
