/* I keep the six BCD date bytes in their observed order; the logger passes them by value. */
#ifndef BMS_DATES_H
#define BMS_DATES_H
#include "ghidra_types.h"
typedef struct {
    uchar d[6];
} date_t;
extern date_t date_now, date_start, date_end;
#endif
