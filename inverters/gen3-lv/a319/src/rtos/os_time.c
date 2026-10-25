/* at: OSTimeTick 0x08030FB8, OS_CPU_SysTickInit 0x08030FEE, os_task_delay 0x0803104A */
/* at: SystemCoreClock 0x20000034, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* The tick: per-task periods, the SysTick set-up and a task delay.
   Library: none. Vendor code; OSTimeTick and OS_CPU_SysTickInit have uC/OS-II names, but
   OSTimeTick does not keep OSTCBDly counters. Each TCB has a period and a count instead:
   when count reaches period the tick posts flag 1 to the task. So a task with a period wakes
   every period ms, as long as it is waiting by then.
   Job in this firmware: SysTick_Handler (stm32f10x_it.c) calls OSTimeTick every 1 ms. main
   sets task 5's period to 20; task_monitor sets the others once the EEPROM settings are loaded,
   and several tasks change their own or another's period (docs/subsystems/rtos.md).
   name: likely (map/filenames.tsv); os_task_delay is descriptive. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/rtos_types.h"
#include "../include/cm3_inline.h"
#include "../include/sys.h"
#include "../include/tasks.h"

#define OSTCBTbl task_tbl

/* For each of the eleven TCBs with a period: count up, and at the period post flag 1
   (os_flags_post_isr) and start again. Called from SysTick_Handler every 1 ms. */
void OSTimeTick()
{
  OS_TCB *p;
  ushort i = 0;
  OS_TCB *end = &OSTCBTbl[10];

  p = &OSTCBTbl[0];
  while (p <= end)
  {
    if (p->period != 0)
    {
      p->count++;
      if (p->count >= p->period)
      {
        p->count = 0;
        os_flags_post_isr(i, 1);
      }
    }
    p++;
    i++;
  }
}
#undef OSTCBTbl


/* SysTick at SystemCoreClock / 1000: a 1 ms tick. Spins for ever if
   SysTick_Config refuses the value. */
void OS_CPU_SysTickInit()
{
  if (SysTick_Config(SystemCoreClock / 1000))
  {
    while (1);
  }
}

#define OSTCBTbl task_tbl

/* Sleep task idx for first ticks, then give it period as its tick period again. It clears
   flag 1, sets the period to first, waits, then sets the period. Callers pass their own index
   (os_task_dly_idx, which is OSPrioCur). */
void os_task_delay(int idx, ushort first, ushort period)
{
  OS_TCB *t = &OSTCBTbl[idx];

  t->FlagsRdy &= 0xfffe;
  OSTCBTbl[idx].count = 0;
  OSTCBTbl[idx].period = first;
  os_flags_pend(0);
  OSTCBTbl[idx].count = 0;
  OSTCBTbl[idx].period = period;
}
#undef OSTCBTbl
