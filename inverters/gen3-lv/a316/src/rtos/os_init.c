/* at: OSInit 0x080305EC */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* OSInit: the scheduler's start-up initialisation, alone in its object.
   Library: none. Vendor code with the uC/OS-II name (uC/OS-II's OSInit is in os_core.c).
   Job in this firmware: main calls it after board_init and before the eight OSTaskCreate calls.
   name: descriptive file name (map/filenames.tsv). The object sits between os_flag and os_task;
   it could be one file with os_task.c, which I have not tried. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/tasks.h"

/* Clear the stack pool, the scheduler variables and the TCBs. */
void OSInit()
{

  OS_InitStkPool();
  OS_InitMisc();
  OS_InitTCBList();
  return;
}

