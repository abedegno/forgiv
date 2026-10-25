/* init.h: the start-up set-up of the peripherals (drivers/device_init.c, init_system) and
   the ramfuncs copy symbols it uses (RamfuncsLoadStart, RamfuncsLoadSize, RamfuncsRunStart:
   the code that runs from RAM at 0x8000, loaded from 0x3F1BB7).
   name: descriptive. */
#ifndef INIT_H
#define INIT_H

#include "F2806x_Device.h"
#include "util.h"

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */

/* device_init.c */
void InitAdc(void);
void InitDmaAdc(void);
void InitECap(void);
void InitEPwm(void);
void InitGpio(void);
void adc_offset_measure(void);
void epwm_trip_clear(void);
void epwm_trip_force(void);
void init_system(void);
void timer2_init(void);

/* defined by no source: the linker places it at its map address */
extern unsigned int RamfuncsLoadSize;
extern unsigned int RamfuncsLoadStart;
extern unsigned int RamfuncsRunStart;
/* ---- end of declarations ---- */

#endif
