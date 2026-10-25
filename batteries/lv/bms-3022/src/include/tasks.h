/* tasks.h: the task scheduler (src/rtos; uC/OS-II shaped, not Micrium's code): task control
   blocks, ready mask, event flags. A task sets its own period and count in task_tbl[prio] and
   then waits in os_flags_pend; OSTimeTick posts flag 1 when the period runs out, and interrupts
   post other flags (2 ADC window to task 0, 8 DSP frame to task 1, 0x10 DSP frame sent to task
   2).
   name: descriptive (readability step 1). */
#ifndef TASKS_H
#define TASKS_H

#include "ghidra_types.h"
#include "rtos_types.h"

/* data */
extern ushort OSPrioHighRdy;
extern OS_TCB *OSTCBCur;
extern OS_TCB *OSTCBHighRdy;
extern ushort OSUnMapTbl[];
extern ushort OSRdyGrp;
extern uchar os_stk_next[];
extern uchar os_stk_pool[];
extern uchar os_stk_pool_top;
extern ushort os_task_dly_idx;
/* task priorities: each task's index in task_tbl (main's OSTaskCreate calls) */
#define TASK_MEASURE 0
#define TASK_DSP_RX 1
#define TASK_DSP_TX 2
#define TASK_MODBUS 3
#define TASK_METER 4
#define TASK_MONITOR 5
#define TASK_SCHEDULE 6
#define TASK_USB 7

extern OS_TCB task_tbl[]; /* by priority: 0 measure, 1 dsp_rx, 2 dsp_tx, 3 modbus, 4 meter, 5
                              monitor, 6 schedule, 7 usb */

/* functions */
void OSInit();
void OSIntExit();
void OSStart();
void OSTaskCreate(int param_1, int param_2, ushort param_3, int param_4);
int OSTaskStkChk(int param_1);
uint *OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, uint *ptos);
void OSTaskSwHook(void);
void OSTimeTick();
void OS_CPU_SysTickInit();
void OS_ENTER_CRITICAL();
void OS_EXIT_CRITICAL();
void OS_InitMisc();
void OS_InitStkPool();
void OS_InitTCBList();
void OS_Sched();
ushort OS_SchedNew();
ushort os_flags_accept();
uint os_flags_pend(ushort flags);
void os_flags_post(ushort prio, ushort flags);
void os_flags_post_isr(ushort prio, ushort flags);
void os_task_delay(int idx, ushort first, ushort period);
void sub_08030450(void);
void sub_08030468(void);
void sub_08030476(void);

#endif
