/* at: os_flags_pend 0x0802A578, os_flags_accept 0x0802A5CC, os_flags_post 0x0802A5E4, os_flags_post_isr 0x0802A618, OSRdyGrp 0x2000061A, os_task_dly_idx 0x2000061E, OSTCBCur 0x20000624, task_tbl 0x200074A4, OS_ENTER_CRITICAL 0x0802A4DC, OS_EXIT_CRITICAL 0x0802A4E0, OS_Sched 0x0802A3DC */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* Adapted from the matched A316 scheduler. Byte proof determines BMS changes. */
#include "bms_rtos_types.h"

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Wait for a flag outside the mask flags. If none is posted, clear the task's ready bit
   and switch away (OS_Sched). Returns the posted flags outside the mask and keeps only the
   masked ones. All callers pass 0: wait for anything, then clear everything. */
uint os_flags_pend(ushort flags)
{
  ushort rdy;

  OS_ENTER_CRITICAL();
  OSTCBCur->FlagsWait = flags;
  rdy = OSTCBCur->FlagsRdy;
  if ((rdy & ~flags) == 0)
  {
    OSRdyGrp &= ~(1 << OSPrioCur);
    OS_EXIT_CRITICAL();
    OS_Sched();
    OS_ENTER_CRITICAL();
    rdy = OSTCBCur->FlagsRdy;
    OSTCBCur->FlagsRdy = OSTCBCur->FlagsRdy & flags;
    OS_EXIT_CRITICAL();
    return rdy & ~flags;
  }
  OSTCBCur->FlagsRdy = OSTCBCur->FlagsRdy & flags;
  OS_EXIT_CRITICAL();
  return rdy & ~flags;
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Return and clear the current task's posted flags without waiting. */
ushort os_flags_accept()
{
  ushort rdy;

  OS_ENTER_CRITICAL();
  rdy = OSTCBCur->FlagsRdy;
  OSTCBCur->FlagsRdy = 0;
  OS_EXIT_CRITICAL();
  return rdy;
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Post flags to task prio and make it ready if any flag outside its wait mask is set.
   Task level: in a critical section. Does not switch tasks. */
void os_flags_post(ushort prio, ushort flags)
{
  OS_TCB *ptcb;

  OS_ENTER_CRITICAL();
  ptcb = &OSTCBTbl[prio];
  ptcb->FlagsRdy |= flags;
  if ((ptcb->FlagsRdy & ~ptcb->FlagsWait) != 0)
  {
    OSRdyGrp |= 1 << prio;
  }
  OS_EXIT_CRITICAL();
}
#undef OSPrioCur
#undef OSTCBTbl

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* The same as os_flags_post without the critical section, for the tick and interrupts. */
void os_flags_post_isr(ushort prio, ushort flags)
{
  OS_TCB *ptcb;

  ptcb = &OSTCBTbl[prio];
  ptcb->FlagsRdy |= flags;
  if ((ptcb->FlagsRdy & ~ptcb->FlagsWait) != 0)
  {
    OSRdyGrp |= 1 << prio;
  }
}
#undef OSPrioCur
#undef OSTCBTbl

