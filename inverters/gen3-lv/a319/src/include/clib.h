/* clib.h: the Keil C library functions the application calls, with the loose types the callers
   were compiled against (src/clib holds the two members built from C).
   name: descriptive (readability step 1). */
#ifndef CLIB_H
#define CLIB_H

#include "firmware_types.h"

/* functions */
void __use_two_region_memory();
void _fp_init();
int abs(int);
int strcmp(int, int);
void strcpy(int, int);
void keil_heap_escrow_two_region();
void keil_heap_expand_two_region();

#endif
