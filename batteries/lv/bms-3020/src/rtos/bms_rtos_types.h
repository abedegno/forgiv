#ifndef BMS_RTOS_TYPES_H
#define BMS_RTOS_TYPES_H
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef struct {
    uint *StkPtr;
    uchar Reserved_[6];
    ushort period, count, FlagsRdy, FlagsWait;
    uchar Reserved2_[2];
} OS_TCB;
extern ushort OSPrioHighRdy, os_task_dly_idx, OSUnMapTbl[], OSRdyGrp;
extern OS_TCB *OSTCBCur, *OSTCBHighRdy;
extern uint *os_stk_next;
extern uchar os_stk_pool[], os_stk_pool_top;
extern OS_TCB task_tbl[];
void OS_ENTER_CRITICAL(void);
void OS_EXIT_CRITICAL(void);
void OS_Sched(void);
ushort OS_SchedNew(void);
void OSCtxSw(void);
void OSStartHighRdy(void);
void OSIntCtxSw(void);
void OS_InitStkPool(void);
void OS_InitMisc(void);
void OS_InitTCBList(void);
uint *OSTaskStkInit(void (*task)(void *), void *arg, uint *ptos);
#endif
