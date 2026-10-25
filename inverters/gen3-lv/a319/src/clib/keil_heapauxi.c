/* at: __use_two_region_memory 0x080370E8, keil_heap_escrow_two_region 0x080370EA */
/* at: keil_heap_expand_two_region 0x080370EC */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Keil C library member heapauxi.o from c_w.l: three empty functions (bx lr each).
   Library: ARM/Keil MDK C library (armcc 5). This file is a C stand-in for one library member,
   not ARM's source.
   Job in this firmware: the startup file IMPORTs __use_two_region_memory, which tells the
   library that the heap and the stack are two separate regions (startup_stm32f10x_cl.s
   defines HEAP and STACK and __user_initial_stackheap). The licensed link symbol table confirms
   __rt_heap_escrow$2region at 0x080370EB and __rt_heap_expand$2region at 0x080370ED
   (Thumb addresses). Their descriptive C stand-in names avoid the dollar-sign suffix.
   match: the gate links the member from the library and also proves this C file at the same
   addresses; both must give the same bytes (docs/gate.md, section 4).
   name: original (map/filenames.tsv). Two old parts joined; the joined file compiles to the
   same bytes. */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/clib.h"

/* Marker: the heap and stack are two regions. Empty. */
void __use_two_region_memory()
{

  return;
}

/* __rt_heap_escrow$2region: the empty two-region heap escrow hook. */
void keil_heap_escrow_two_region()
{

  return;
}

/* __rt_heap_expand$2region: the empty two-region heap expansion hook. */
void keil_heap_expand_two_region()
{

  return;
}
