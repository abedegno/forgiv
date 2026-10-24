/* at: OSInit 0x0802A208, OSStart 0x0802A21A, OSTaskCreate 0x0802A23C, OSIntExit 0x0802A268, OSTaskStkChk 0x0802A290, OSPrioHighRdy 0x20000604, os_task_dly_idx 0x20000606, OSTCBHighRdy 0x20000608, OSTCBCur 0x2000060C, os_stk_next 0x20000610, task_tbl 0x2000748C, OS_SchedNew 0x08029F58, OSStartHighRdy 0x0802A0A0, OSIntCtxSw 0x0802A0C6, OSTaskStkInit 0x0802A03C, OS_InitStkPool 0x08029FC4, OS_InitMisc 0x08029FE0, OS_InitTCBList 0x08029FF8 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* Adapted from the matched A316 scheduler. Byte proof determines BMS changes. */
#include "bms_rtos_types.h"

ushort OSPrioHighRdy;
ushort os_task_dly_idx;
OS_TCB *OSTCBHighRdy;
OS_TCB *OSTCBCur;
uint *os_stk_next;

/* Clear the stack pool, the scheduler variables and the TCBs. */
void OSInit()
{

  OS_InitStkPool();
  OS_InitMisc();
  OS_InitTCBList();
  return;
}


/* Make the highest ready task current and start it (OSStartHighRdy). Nothing is ready
   yet, so OS_SchedNew gives the fallback priority 4. */
void OSStart()
{

  
  OSPrioHighRdy = OS_SchedNew();
  os_task_dly_idx = OSPrioHighRdy;
  (*(int *)(((uchar *)&OSTCBHighRdy) + 0)) = &(*(uchar *)(((uchar *)task_tbl) + 0)) + OSPrioHighRdy * 20;
  (*(int *)(((uchar *)&OSTCBCur) + 0)) = (*(int *)(((uchar *)&OSTCBHighRdy) + 0));
  OSStartHighRdy();
}


/* Create a task: param_1 the entry, param_2 its argument, param_3 the stack size in
   words, param_4 the priority (TCB index). The stack is taken from the top of the pool
   downwards (os_stk_next), then OSTaskStkInit builds the first frame. The task is not made
   ready: it first runs when its period posts flag 1 or another task posts to it. */
void OSTaskCreate(int param_1, int param_2, ushort param_3, int param_4)
{
  int v5;
  int v7;
  int v10;
  
  v7 = (int)os_stk_next;
  *(int *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20 + 4) = v7;
  *(ushort *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20 + 8) = param_3;
  os_stk_next = (uint *)((int)os_stk_next - (param_3 << 2));
  v10 = OSTaskStkInit(param_1, param_2, v7);
  v5 = &(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20;
  *(int *)v5 = v10;
  *(ushort *)(v5 + 0xa) = 0;
  *(ushort *)(v5 + 0xc) = 0;
  *(ushort *)(v5 + 0xe) = 0;
}


/* Switch to a higher ready task at the end of an interrupt (OSIntCtxSw). */
void OSIntExit()
{

  
  OSPrioHighRdy = OS_SchedNew();
  if (OSPrioHighRdy != os_task_dly_idx) {
    os_task_dly_idx = OSPrioHighRdy;
    (*(int *)(((uchar *)&OSTCBHighRdy) + 0)) = &(*(uchar *)(((uchar *)task_tbl) + 0)) + OSPrioHighRdy * 20;
    OSIntCtxSw();
  }
}


/* Stack used by task param_1, in words: scans down from its top to the first run of ten
   zero words (OS_InitStkPool zeroed the pool). 0 if it reaches the bottom first. */
int OSTaskStkChk(int param_1)
{
  int v2;
  ushort v3;
  
  v3 = 0;
  v2 = *(int *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_1 * 20 + 4);
  while (1) {
    if (*(int *)v2 == 0) {
      v3++;
      if (v3 == 0xa) {
        return (ushort)((*(int *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_1 * 20 + 4) - v2 >> 2) - 0xa);
      }
    }
    else {
      v3 = 0;
    }
    v2 = v2 - 4;
    if (*(int *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_1 * 20 + 4) - (*(ushort *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_1 * 20 + 8) << 2) == v2) {
      return 0;
    }
  }
}

