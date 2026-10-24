/* at: OS_SchedNew 0x08029F58, OS_Sched 0x08029F98, OS_InitStkPool 0x08029FC4, OS_InitMisc 0x08029FE0, OS_InitTCBList 0x08029FF8, OSUnMapTbl 0x200005E2, OSRdyGrp 0x20000602, OSPrioHighRdy 0x20000604, os_task_dly_idx 0x20000606, OSTCBHighRdy 0x20000608, OSTCBCur 0x2000060C, os_stk_next 0x20000610, os_stk_pool 0x200045AC, os_stk_pool_top 0x20007488, task_tbl 0x2000748C, OS_ENTER_CRITICAL 0x0802A098, OS_EXIT_CRITICAL 0x0802A09C, OSCtxSw 0x0802A0B8 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* Adapted from the matched A316 scheduler. Byte proof determines BMS changes. */
#include "bms_rtos_types.h"

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Highest ready priority: the lowest set bit of OSRdyGrp, one nibble at a time through
   OSUnMapTbl. Returns priority 4 when no bit is set. */
ushort OS_SchedNew()
{
  if ((OSRdyGrp & 0x000F) != 0)
  {
    return OSUnMapTbl[OSRdyGrp & 0x000F];
  }
  if ((OSRdyGrp & 0x00F0) != 0)
  {
    return OSUnMapTbl[(OSRdyGrp & 0x00F0) >> 4] + 4;
  }
  if ((OSRdyGrp & 0x0F00) != 0)
  {
    return OSUnMapTbl[(OSRdyGrp & 0x0F00) >> 8] + 8;
  }
  return 4;
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Make the highest ready task current and pend PendSV (OSCtxSw). Called from
   os_flags_pend. */
void OS_Sched()
{

  OS_ENTER_CRITICAL();
  OSPrioHighRdy = OS_SchedNew();
  OSPrioCur = OSPrioHighRdy;
  OSTCBHighRdy = &OSTCBTbl[OSPrioHighRdy];
  OSCtxSw();
  OS_EXIT_CRITICAL();
}
#undef OSPrioCur
#undef OSTCBTbl


/* Zero the 3000-word task stack pool and point os_stk_next at its last word. */
void OS_InitStkPool()
{
  int iVar1;
  
  for (iVar1 = 0; iVar1 < 3000; iVar1 = iVar1 + 1) {
    (&(*(uint *)(os_stk_pool + 0)))[iVar1] = 0;
  }
  os_stk_next = (uint *)&os_stk_pool_top;
  return;
}

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Clear the ready mask and the current and highest-ready task. */
void OS_InitMisc()
{
  OSRdyGrp = 0;
  OSPrioCur = 0;
  OSPrioHighRdy = 0;
  OSTCBHighRdy = 0;
  OSTCBCur = 0;
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Clear the stack pointer, period, count and both flag words of all eleven TCBs. */
void OS_InitTCBList()
{
  OS_TCB *ptcb;
  ushort i;

  ptcb = &OSTCBTbl[0];
  for (i = 0; i < 11; i++)
  {
    ptcb->StkPtr = 0;
    ptcb->period = 0;
    ptcb->count = 0;
    ptcb->FlagsRdy = 0;
    ptcb->FlagsWait = 0;
    ptcb++;
  }
}
#undef OSPrioCur
#undef OSTCBTbl

