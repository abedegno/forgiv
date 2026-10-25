/* at: OSStart 0x08030E7C, OSTaskCreate 0x08030EAE, OSIntExit 0x08030F06, OSTaskStkChk 0x08030F3C */
/* at: OSPrioHighRdy 0x200004D6, os_task_dly_idx 0x200004D8, OSTCBHighRdy 0x200004DC, OSTCBCur 0x200004E0, os_stk_next 0x200004E4, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Starting the scheduler, creating a task, interrupt exit and a stack check.
   Library: none. Vendor code with uC/OS-II names (OSStart, OSTaskCreate, OSIntExit,
   OSTaskStkChk); the bodies are much smaller than Micrium's.
   Job in this firmware: main creates eight tasks with OSTaskCreate(task, arg, stack words,
   priority) and then calls OSStart, which does not return. docs/subsystems/rtos.md lists them.
   A TCB keeps the task's stack top at offset 4 and its size in words at offset 8 (the
   Reserved_ bytes of OS_TCB); only OSTaskCreate and OSTaskStkChk use them.
   Unused here: OSIntExit and OSTaskStkChk have no callers. No interrupt handler calls OSIntExit,
   so the scheduler never pre-empts a task.
   name: likely (map/filenames.tsv). The callers in os_cpu_a.s fix which routine is which:
   OSStart calls OSStartHighRdy, OSIntExit calls OSIntCtxSw. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/tasks.h"


/* Make the highest ready task current and start it (OSStartHighRdy). Nothing is ready
   yet, so OS_SchedNew gives 7 and task_usb runs first. */
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

  v7 = (*(int *)(os_stk_next + 0));
  *(int *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20 + 4) = v7;
  *(ushort *)(&(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20 + 8) = param_3;
  (*(int *)(os_stk_next + 0)) = (*(int *)(os_stk_next + 0)) - (param_3 << 2);
  v10 = OSTaskStkInit(param_1, param_2, v7);
  v5 = &(*(uchar *)(((uchar *)task_tbl) + 0)) + param_4 * 20;
  *(int *)v5 = v10;
  *(ushort *)(v5 + 0xa) = 0;
  *(ushort *)(v5 + 0xc) = 0;
  *(ushort *)(v5 + 0xe) = 0;
}


/* Switch to a higher ready task at the end of an interrupt (OSIntCtxSw). No callers. */
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
   zero words (OS_InitStkPool zeroed the pool). 0 if it reaches the bottom first. No callers. */
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
