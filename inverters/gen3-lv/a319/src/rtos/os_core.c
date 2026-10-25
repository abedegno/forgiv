/* at: OS_SchedNew 0x08030B18, OS_Sched 0x08030B72, OS_InitStkPool 0x08030BA8, OS_InitMisc 0x08030BC4, OS_InitTCBList 0x08030BDC */
/* at: OSUnMapTbl 0x200004B2, OSRdyGrp 0x200004D4, OSPrioHighRdy 0x200004D6, os_task_dly_idx 0x200004D8, OSTCBHighRdy 0x200004DC, OSTCBCur 0x200004E0, os_stk_next 0x200004E4, os_stk_pool 0x20004030, os_stk_pool_top 0x20007EAC, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Scheduler core: pick the highest ready task, switch to it, and the start-up initialisers.
   Library: none. This is the vendor's own small cooperative scheduler, written in the shape of
   uC/OS-II (Micrium) and using its names, not Micrium's code. Eleven task control blocks
   (task_tbl, OS_TCB in src/include/rtos_types.h), one per priority 0..10; a task's priority is its
   index, and 0 is the most urgent. OSRdyGrp has one ready bit per task.
   Job in this firmware: runs the eight tasks main creates (docs/subsystems/rtos.md has the table).
   A task runs until it calls os_flags_pend; no interrupt switches tasks (OSIntExit has no caller),
   so the tick only marks tasks ready.
   Differences from uC/OS-II: no OSRdyTbl, no event control blocks, no OSRunning, no idle task:
   when nothing is ready OS_SchedNew returns 7, so task 7 (task_usb) is the fall-back.
   OSPrioCur is the map's os_task_dly_idx (0x200004D8): the tasks pass it to os_task_delay as their
   own index, which is how the map named it. The #define lines give it the uC/OS-II name.
   name: likely (map/filenames.tsv). OS_SchedNew, OS_Sched, OS_InitMisc and OS_InitTCBList are
   os_core.c names in uC/OS-II; OS_InitStkPool is descriptive. The object comes before os_cpu_c
   and os_cpu_a.s in link order, as os_core.c does in a uC/OS-II project. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/rtos_types.h"
#include "../include/tasks.h"

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Highest ready priority: the lowest set bit of OSRdyGrp, one nibble at a time through
   OSUnMapTbl. Returns 7 (task_usb) when no bit is set. */
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
  return 7;
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Make the highest ready task current and pend PendSV (OSCtxSw). Called only from
   os_flags_pend. match: cpu_sr is a dead volatile store kept for the bytes (docs/matching.md). */
void OS_Sched()
{
  volatile uint cpu_sr = 0;

  OS_ENTER_CRITICAL();
  OSPrioHighRdy = OS_SchedNew();
  OSPrioCur = OSPrioHighRdy;
  OSTCBHighRdy = &OSTCBTbl[OSPrioHighRdy];
  OSCtxSw();
  OS_EXIT_CRITICAL();
}
#undef OSPrioCur
#undef OSTCBTbl


/* Zero the 4000-word task stack pool and point os_stk_next at its last word. */
void OS_InitStkPool()
{
  int iVar1;

  for (iVar1 = 0; iVar1 < 4000; iVar1 = iVar1 + 1) {
    (&(*(uint *)(os_stk_pool + 0)))[iVar1] = 0;
  }
  (*(uint *)(os_stk_next + 0)) = &os_stk_pool_top;
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
