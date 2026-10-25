/* I use the same eight-byte software timer view in the runtime and link tasks. */
#ifndef BMS_TIMERS_H
#define BMS_TIMERS_H
#include "ghidra_types.h"
typedef struct {
    uchar enabled, elapsed;
    ushort reserved;
    uint left;
} RuntimeTimer;
extern volatile RuntimeTimer g_20000F9C[31];
#endif
