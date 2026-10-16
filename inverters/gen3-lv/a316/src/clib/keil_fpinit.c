/* at: _fp_init 0x0803692C */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
/* Keil C library member fpinit.o from fz_ws.l: _fp_init, an empty function.
   Library: ARM/Keil MDK C library (armcc 5, the MDK 5.16 to 5.21 library). This file is a C
   stand-in for one library member, not ARM's source.
   Job in this firmware: the C library start-up calls _fp_init to set up the floating point
   library; with the software float library (fz_ws.l) there is nothing to do.
   match: the gate links the member from the library and also proves this C file at the same
   address; both must give the same bytes (docs/gate.md, section 4).
   name: original (map/filenames.tsv, map/names.tsv). The member also defines
   __fp_init_empty and __fplib_config_pureend_doubles at this address. The stem is keil_fpinit
   so it does not clash with the member's own name. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/clib.h"

/* Floating point library initialisation: nothing to do (bx lr). */
void _fp_init()
{

  return;
}

