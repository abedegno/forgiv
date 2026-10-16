/* clib.h: the Keil C library functions the application calls, with the loose types the callers
   were compiled against (src/clib holds the two members built from C).
   name: descriptive (readability step 1). */
#ifndef CLIB_H
#define CLIB_H

#include "ghidra_types.h"

/* functions */
void __use_two_region_memory();
void _fp_init();
int abs(int);
int strcmp(int, int);
void strcpy(int, int);
void sub_0803686A();
void sub_0803686C();

#endif
