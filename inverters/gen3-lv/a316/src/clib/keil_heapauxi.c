/* at: __use_two_region_memory 0x08036868, sub_0803686A 0x0803686A */
/* at: sub_0803686C 0x0803686C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Keil C library member heapauxi.o from c_w.l: three empty functions (bx lr each).
   Library: ARM/Keil MDK C library (armcc 5). This file is a C stand-in for one library member,
   not ARM's source.
   Job in this firmware: the startup file IMPORTs __use_two_region_memory, which tells the
   library that the heap and the stack are two separate regions (startup_stm32f10x_cl.s
   defines HEAP and STACK and __user_initial_stackheap). The other two entry points are, I think,
   the member's two-region heap hooks (__rt_heap_escrow and __rt_heap_expand); I have not read
   the member's symbol table to confirm, so they keep sub_ names.
   match: the gate links the member from the library and also proves this C file at the same
   addresses; both must give the same bytes (docs/gate.md, section 4).
   name: original (map/filenames.tsv). Two old parts joined; the joined file compiles to the
   same bytes. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/clib.h"

/* Marker: the heap and stack are two regions. Empty. */
void __use_two_region_memory()
{

  return;
}

/* Second entry point of heapauxi.o, probably a two-region heap hook. Empty. */
void sub_0803686A()
{

  return;
}

/* Third entry point of heapauxi.o, probably a two-region heap hook. Empty. */
void sub_0803686C()
{

  return;
}
