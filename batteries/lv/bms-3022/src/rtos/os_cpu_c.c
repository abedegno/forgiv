/* at: OSTaskStkInit 0x0802A480 */
/* at: OSTaskSwHook 0x0802A4DA */
/* at: OS_ENTER_CRITICAL 0x0802A4DC */
/* at: OS_EXIT_CRITICAL 0x0802A4E0 */
/* at: sys_irq_disable 0x0801422C */
/* at: sys_irq_enable 0x08014230 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* Shared vendor unit, recompiled and linked at the battery addresses. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/tasks.h"
#include "../include/sys.h"

uint * OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, uint *ptos);

/* Build the first frame of a task below ptos: the exception frame (xPSR, PC = task,
   LR, R12, R3-R1, R0 = p_arg) and then R11-R4, which PendSV_Handler pops. Returns the new
   stack pointer for the TCB. */
uint *OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, uint *ptos)
{
  uint *stk;

  stk = ptos;
  *(stk) = (uint)0x01000000L;
  *(--stk) = (uint)task;
  *(--stk) = (uint)0xFFFFFFFEL;
  *(--stk) = (uint)0x12121212L;
  *(--stk) = (uint)0x03030303L;
  *(--stk) = (uint)0x02020202L;
  *(--stk) = (uint)0x01010101L;
  *(--stk) = (uint)p_arg;

  *(--stk) = (uint)0x11111111L;
  *(--stk) = (uint)0x10101010L;
  *(--stk) = (uint)0x09090909L;
  *(--stk) = (uint)0x08080808L;
  *(--stk) = (uint)0x07070707L;
  *(--stk) = (uint)0x06060606L;
  *(--stk) = (uint)0x05050505L;
  *(--stk) = (uint)0x04040404L;

  return (stk);
}

/* match: an empty function after OSTaskStkInit (bx lr at 0x08030882); nothing calls it */
void OSTaskSwHook(void)
{
}

/* Disable interrupts (cpsid i). Not nesting. */
void OS_ENTER_CRITICAL()
{

  sys_irq_disable();
  return;
}

/* Enable interrupts (cpsie i). */
void OS_EXIT_CRITICAL()
{

  sys_irq_enable();
  return;
}
