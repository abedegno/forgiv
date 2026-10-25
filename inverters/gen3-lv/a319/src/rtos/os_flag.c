/* at: os_flags_pend 0x08030D64, os_flags_accept 0x08030DD6, os_flags_post 0x08030DF6, os_flags_post_isr 0x08030E30 */
/* at: OSRdyGrp 0x200004D4, OSPrioHighRdy 0x200004D6, os_task_dly_idx 0x200004D8, OSTCBHighRdy 0x200004DC, OSTCBCur 0x200004E0, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Per-task event flags: the only way a task blocks and the only way another task or an
   interrupt wakes it.
   Library: none. Vendor code in the uC/OS-II manner. uC/OS-II's event flag groups (os_flag.c)
   are separate objects that tasks wait on; here each TCB holds its own two 16-bit words,
   FlagsRdy (posted) and FlagsWait (the mask the task ignores).
   Job in this firmware: every task loop ends in os_flags_pend(0). Flag 1 comes from OSTimeTick
   when the task's period runs out; ADC1_2_IRQHandler posts flag 2 to task 0 (task_measure) at
   the end of each mains cycle of samples.
   match: the volatile cpu_sr locals are dead stores the image has (docs/matching.md, round 4).
   name: descriptive for the functions; likely for the file (map/filenames.tsv). */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/rtos_types.h"
#include "../include/tasks.h"

#define OSPrioCur os_task_dly_idx
#define OSTCBTbl task_tbl

/* Wait for a flag outside the mask flags. If none is posted, clear the task's ready bit
   and switch away (OS_Sched). Returns the posted flags outside the mask and keeps only the
   masked ones. All callers pass 0: wait for anything, then clear everything. */
uint os_flags_pend(ushort flags)
{
  volatile uint cpu_sr = 0;
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

/* Return and clear the current task's posted flags without waiting. No callers. */
ushort os_flags_accept()
{
  volatile uint cpu_sr = 0;
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
  volatile uint cpu_sr = 0;
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
