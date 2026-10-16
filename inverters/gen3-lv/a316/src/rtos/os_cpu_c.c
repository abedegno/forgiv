/* at: OSTaskStkInit 0x080303A4, OSTaskSwHook 0x0803043E */
/* at: OS_ENTER_CRITICAL 0x08030440 */
/* at: OS_EXIT_CRITICAL 0x08030448 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Port layer, C part: the initial stack frame of a task and the critical section.
   Library: Micrium's uC/OS-II Cortex-M3 port, os_cpu_c.c. OSTaskStkInit is that port's code word
   for word (the register fill values 0x04040404 to 0x12121212, xPSR 0x01000000, LR 0xFFFFFFFE).
   Job in this firmware: OSTaskCreate builds each task's first frame here, and the application
   brackets shared data with OS_ENTER_CRITICAL and OS_EXIT_CRITICAL (uart.c, monitor.c,
   usbh_usr.c).
   Differences from the port: OS_ENTER_CRITICAL and OS_EXIT_CRITICAL are functions, not macros
   around OS_CPU_SR_Save and OS_CPU_SR_Restore. They call sys_irq_disable and sys_irq_enable
   (main_p1.c), so they do not nest: an inner exit enables interrupts. Micrium's other hooks
   (OSInitHookBegin, OSTaskCreateHook, OSTimeTickHook ...) are not in the image.
   name: likely (map/filenames.tsv). Three old parts joined; the joined file compiles to the same
   bytes. os_cpu_a.s follows this object in the image, as in Micrium's port. */
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

/* match: an empty function after OSTaskStkInit (bx lr at 0x0803043E); nothing calls it */
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
