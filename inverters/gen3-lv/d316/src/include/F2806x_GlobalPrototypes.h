/* F2806x_GlobalPrototypes.h: TI's F2806x_GlobalPrototypes.h, cut down to the TI support
   functions this firmware has: the DMA channel 1 set-up, MemCopy, InitFlash, the system
   control, PIE, timer and ADC calibration functions of device_init.c, and DSP28x_usDelay.
   name: original, TI's file name; the list below is generated (docs/readability.md). */
#ifndef F2806X_GLOBALPROTOTYPES_H
#define F2806X_GLOBALPROTOTYPES_H

#include "F2806x_Device.h"

/* ---- declarations: one per name (generated, see docs/readability.md) ---- */
#include "F2806x_CpuTimers.h"
#include "F2806x_Device.h"

/* F2806x_Dma.c */
void DMACH1AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source);
void DMACH1BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep);
void DMACH1ModeConfig(void);
void DMACH1TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep);
void DMACH1WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep);
void StartDMACH1(void);

/* F2806x_MemCopy.c */
void MemCopy(Uint16 *SourceAddr, Uint16 *SourceEndAddr, Uint16 *DestAddr);

/* F2806x_SysCtrl.c */
void InitFlash(void);

/* device_init.c */
void AdcChanSelect(unsigned int ch_no);
unsigned int AdcConversion(void);
void AdcOffsetSelfCal(void);
void ConfigCpuTimer(struct CPUTIMER_VARS *Timer, float Freq, float Period);
void InitPeripheralClocks(void);
void InitPieCtrl(void);
void InitPieVectTable(void);
void InitPll(unsigned int val, unsigned int divsel);
void InitSysCtrl(void);
void XtalOscSel(void);

/* defined by no source: the linker places it at its map address */
void DSP28x_usDelay(unsigned long count);
/* ---- end of declarations ---- */

#endif
