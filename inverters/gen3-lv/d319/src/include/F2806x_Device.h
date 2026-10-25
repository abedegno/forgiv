/* F2806x_Device.h: the part of TI's F2806x_Device.h this firmware uses: the CPU registers
   IER and IFR, the interrupt and EALLOW macros (EINT, DINT, ERTM, DRTM, EALLOW, EDIS, ESTOP0)
   and TI's integer and float types. Users: nearly every source.
   name: original, TI's file name; the peripheral headers it includes in TI's tree are separate
   includes here. */
#ifndef F2806X_DEVICE_H
#define F2806X_DEVICE_H

extern cregister volatile unsigned int IFR;
extern cregister volatile unsigned int IER;

#define EINT asm(" clrc INTM")
#define DINT asm(" setc INTM")
#define ERTM asm(" clrc DBGM")
#define DRTM asm(" setc DBGM")
#define EALLOW asm(" EALLOW")
#define EDIS asm(" EDIS")
#define ESTOP0 asm(" ESTOP0")

typedef int int16;
typedef long int32;
typedef unsigned int Uint16;
typedef unsigned long Uint32;
typedef float float32;
typedef long double float64;

#endif
