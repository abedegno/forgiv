/* F2806x_Dma.c: TI's DMA set-up functions (F2806x_Dma.c), in TI's order. The whole file is
   linked, but only channel 1 is used: InitDmaAdc (device_init.c) calls DMACH1AddrConfig,
   DMACH1BurstConfig, DMACH1TransferConfig, DMACH1WrapConfig, DMACH1ModeConfig and StartDMACH1 to
   copy the ADC results to RAM. DMAInitialize and the 30 functions of channels 2..6 have no
   callers.

   The one change from TI's text: DMACH1ModeConfig is this firmware's own. It takes no arguments
   and sets channel 1's mode itself: PERINTSEL 22, peripheral trigger on, continuous, 16-bit
   words, the channel interrupt at the end of a transfer but disabled (CHINTE 0). It still sets
   the PIE enable 7.1, as TI's does, but IER never enables INT7.

   Entry points: the functions above. Data: none (DmaRegs at 0x1000).
   Neighbours: drivers/device_init.c (InitDmaAdc), F2806x_Dma.h.
   name: original (map/filenames.tsv): TI's file and function names.
   at: DMAInitialize 0x3F0C2D .. StartDMACH6 0x3F0F61 */
#include "F2806x_Dma.h"
#include "F2806x_PieCtrl.h"

/* TI's: resets the DMA and lets it run while the debugger halts the CPU. No callers. */
void DMAInitialize(void)
{
    EALLOW;
    DmaRegs.DMACTRL.bit.HARDRESET = 1;
    asm(" NOP");
    DmaRegs.DEBUGCTRL.bit.FREE = 1;
    EDIS;
}

/* TI's: sets channel 1's source and destination start addresses (shadow registers). Used by
   InitDmaAdc. */
void DMACH1AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH1.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH1.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH1.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH1.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 1's burst size (words - 1) and the source and destination steps in a
   burst. Used by InitDmaAdc. */
void DMACH1BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH1.BURST_SIZE.all = bsize;
    DmaRegs.CH1.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH1.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 1's transfer size (bursts - 1) and the steps between bursts. Used by
   InitDmaAdc. */
void DMACH1TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH1.TRANSFER_SIZE = tsize;
    DmaRegs.CH1.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH1.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 1's source and destination wrap sizes (bursts - 1) and wrap steps. Used by
   InitDmaAdc. */
void DMACH1WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH1.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH1.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH1.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH1.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* This firmware's own, not TI's: channel 1's fixed mode for the ADC copy (see the file header). */
void DMACH1ModeConfig(void)
{
    EALLOW;
    DmaRegs.CH1.MODE.bit.PERINTSEL = 22;
    DmaRegs.CH1.MODE.bit.PERINTE = 1;
    DmaRegs.CH1.MODE.bit.ONESHOT = 0;
    DmaRegs.CH1.MODE.bit.CONTINUOUS = 1;
    DmaRegs.CH1.MODE.bit.SYNCE = 0;
    DmaRegs.CH1.MODE.bit.SYNCSEL = 0;
    DmaRegs.CH1.MODE.bit.OVRINTE = 0;
    DmaRegs.CH1.MODE.bit.DATASIZE = 0;
    DmaRegs.CH1.MODE.bit.CHINTMODE = 1;
    DmaRegs.CH1.MODE.bit.CHINTE = 0;
    DmaRegs.CH1.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH1.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx1 = 1;
    EDIS;
}

/* TI's: starts channel 1. Used by InitDmaAdc. */
void StartDMACH1(void)
{
    EALLOW;
    DmaRegs.CH1.CONTROL.bit.RUN = 1;
    EDIS;
}

/* TI's: sets channel 2's source and destination start addresses (shadow registers). No callers. */
void DMACH2AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH2.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH2.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH2.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH2.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 2's burst size (words - 1) and the source and destination steps in a
   burst. No callers. */
void DMACH2BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH2.BURST_SIZE.all = bsize;
    DmaRegs.CH2.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH2.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 2's transfer size (bursts - 1) and the steps between bursts. No callers. */
void DMACH2TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH2.TRANSFER_SIZE = tsize;
    DmaRegs.CH2.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH2.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 2's source and destination wrap sizes (bursts - 1) and wrap steps. No
   callers. */
void DMACH2WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH2.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH2.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH2.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH2.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* TI's: sets channel 2's mode from its arguments, clears its flags and sets its PIE enable
   (7.2). No callers. */
void DMACH2ModeConfig(Uint16 persel, Uint16 perinte, Uint16 oneshot, Uint16 cont, Uint16 synce,
                       Uint16 syncsel, Uint16 ovrinte, Uint16 datasize, Uint16 chintmode, Uint16 chinte)
{
    EALLOW;
    DmaRegs.CH2.MODE.bit.PERINTSEL = persel;
    DmaRegs.CH2.MODE.bit.PERINTE = perinte;
    DmaRegs.CH2.MODE.bit.ONESHOT = oneshot;
    DmaRegs.CH2.MODE.bit.CONTINUOUS = cont;
    DmaRegs.CH2.MODE.bit.SYNCE = synce;
    DmaRegs.CH2.MODE.bit.SYNCSEL = syncsel;
    DmaRegs.CH2.MODE.bit.OVRINTE = ovrinte;
    DmaRegs.CH2.MODE.bit.DATASIZE = datasize;
    DmaRegs.CH2.MODE.bit.CHINTMODE = chintmode;
    DmaRegs.CH2.MODE.bit.CHINTE = chinte;
    DmaRegs.CH2.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH2.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx2 = 1;
    EDIS;
}

/* TI's: starts channel 2. No callers. */
void StartDMACH2(void)
{
    EALLOW;
    DmaRegs.CH2.CONTROL.bit.RUN = 1;
    EDIS;
}

/* TI's: sets channel 3's source and destination start addresses (shadow registers). No callers. */
void DMACH3AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH3.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH3.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH3.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH3.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 3's burst size (words - 1) and the source and destination steps in a
   burst. No callers. */
void DMACH3BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH3.BURST_SIZE.all = bsize;
    DmaRegs.CH3.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH3.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 3's transfer size (bursts - 1) and the steps between bursts. No callers. */
void DMACH3TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH3.TRANSFER_SIZE = tsize;
    DmaRegs.CH3.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH3.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 3's source and destination wrap sizes (bursts - 1) and wrap steps. No
   callers. */
void DMACH3WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH3.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH3.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH3.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH3.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* TI's: sets channel 3's mode from its arguments, clears its flags and sets its PIE enable
   (7.3). No callers. */
void DMACH3ModeConfig(Uint16 persel, Uint16 perinte, Uint16 oneshot, Uint16 cont, Uint16 synce,
                       Uint16 syncsel, Uint16 ovrinte, Uint16 datasize, Uint16 chintmode, Uint16 chinte)
{
    EALLOW;
    DmaRegs.CH3.MODE.bit.PERINTSEL = persel;
    DmaRegs.CH3.MODE.bit.PERINTE = perinte;
    DmaRegs.CH3.MODE.bit.ONESHOT = oneshot;
    DmaRegs.CH3.MODE.bit.CONTINUOUS = cont;
    DmaRegs.CH3.MODE.bit.SYNCE = synce;
    DmaRegs.CH3.MODE.bit.SYNCSEL = syncsel;
    DmaRegs.CH3.MODE.bit.OVRINTE = ovrinte;
    DmaRegs.CH3.MODE.bit.DATASIZE = datasize;
    DmaRegs.CH3.MODE.bit.CHINTMODE = chintmode;
    DmaRegs.CH3.MODE.bit.CHINTE = chinte;
    DmaRegs.CH3.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH3.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx3 = 1;
    EDIS;
}

/* TI's: starts channel 3. No callers. */
void StartDMACH3(void)
{
    EALLOW;
    DmaRegs.CH3.CONTROL.bit.RUN = 1;
    EDIS;
}

/* TI's: sets channel 4's source and destination start addresses (shadow registers). No callers. */
void DMACH4AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH4.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH4.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH4.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH4.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 4's burst size (words - 1) and the source and destination steps in a
   burst. No callers. */
void DMACH4BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH4.BURST_SIZE.all = bsize;
    DmaRegs.CH4.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH4.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 4's transfer size (bursts - 1) and the steps between bursts. No callers. */
void DMACH4TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH4.TRANSFER_SIZE = tsize;
    DmaRegs.CH4.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH4.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 4's source and destination wrap sizes (bursts - 1) and wrap steps. No
   callers. */
void DMACH4WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH4.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH4.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH4.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH4.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* TI's: sets channel 4's mode from its arguments, clears its flags and sets its PIE enable
   (7.4). No callers. */
void DMACH4ModeConfig(Uint16 persel, Uint16 perinte, Uint16 oneshot, Uint16 cont, Uint16 synce,
                       Uint16 syncsel, Uint16 ovrinte, Uint16 datasize, Uint16 chintmode, Uint16 chinte)
{
    EALLOW;
    DmaRegs.CH4.MODE.bit.PERINTSEL = persel;
    DmaRegs.CH4.MODE.bit.PERINTE = perinte;
    DmaRegs.CH4.MODE.bit.ONESHOT = oneshot;
    DmaRegs.CH4.MODE.bit.CONTINUOUS = cont;
    DmaRegs.CH4.MODE.bit.SYNCE = synce;
    DmaRegs.CH4.MODE.bit.SYNCSEL = syncsel;
    DmaRegs.CH4.MODE.bit.OVRINTE = ovrinte;
    DmaRegs.CH4.MODE.bit.DATASIZE = datasize;
    DmaRegs.CH4.MODE.bit.CHINTMODE = chintmode;
    DmaRegs.CH4.MODE.bit.CHINTE = chinte;
    DmaRegs.CH4.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH4.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx4 = 1;
    EDIS;
}

/* TI's: starts channel 4. No callers. */
void StartDMACH4(void)
{
    EALLOW;
    DmaRegs.CH4.CONTROL.bit.RUN = 1;
    EDIS;
}

/* TI's: sets channel 5's source and destination start addresses (shadow registers). No callers. */
void DMACH5AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH5.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH5.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH5.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH5.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 5's burst size (words - 1) and the source and destination steps in a
   burst. No callers. */
void DMACH5BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH5.BURST_SIZE.all = bsize;
    DmaRegs.CH5.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH5.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 5's transfer size (bursts - 1) and the steps between bursts. No callers. */
void DMACH5TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH5.TRANSFER_SIZE = tsize;
    DmaRegs.CH5.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH5.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 5's source and destination wrap sizes (bursts - 1) and wrap steps. No
   callers. */
void DMACH5WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH5.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH5.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH5.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH5.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* TI's: sets channel 5's mode from its arguments, clears its flags and sets its PIE enable
   (7.5). No callers. */
void DMACH5ModeConfig(Uint16 persel, Uint16 perinte, Uint16 oneshot, Uint16 cont, Uint16 synce,
                       Uint16 syncsel, Uint16 ovrinte, Uint16 datasize, Uint16 chintmode, Uint16 chinte)
{
    EALLOW;
    DmaRegs.CH5.MODE.bit.PERINTSEL = persel;
    DmaRegs.CH5.MODE.bit.PERINTE = perinte;
    DmaRegs.CH5.MODE.bit.ONESHOT = oneshot;
    DmaRegs.CH5.MODE.bit.CONTINUOUS = cont;
    DmaRegs.CH5.MODE.bit.SYNCE = synce;
    DmaRegs.CH5.MODE.bit.SYNCSEL = syncsel;
    DmaRegs.CH5.MODE.bit.OVRINTE = ovrinte;
    DmaRegs.CH5.MODE.bit.DATASIZE = datasize;
    DmaRegs.CH5.MODE.bit.CHINTMODE = chintmode;
    DmaRegs.CH5.MODE.bit.CHINTE = chinte;
    DmaRegs.CH5.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH5.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx5 = 1;
    EDIS;
}

/* TI's: starts channel 5. No callers. */
void StartDMACH5(void)
{
    EALLOW;
    DmaRegs.CH5.CONTROL.bit.RUN = 1;
    EDIS;
}

/* TI's: sets channel 6's source and destination start addresses (shadow registers). No callers. */
void DMACH6AddrConfig(volatile Uint16 *DMA_Dest, volatile Uint16 *DMA_Source)
{
    EALLOW;
    DmaRegs.CH6.SRC_BEG_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH6.SRC_ADDR_SHADOW = (Uint32)DMA_Source;
    DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)DMA_Dest;
    DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)DMA_Dest;
    EDIS;
}

/* TI's: sets channel 6's burst size (words - 1) and the source and destination steps in a
   burst. No callers. */
void DMACH6BurstConfig(Uint16 bsize, int16 srcbstep, int16 desbstep)
{
    EALLOW;
    DmaRegs.CH6.BURST_SIZE.all = bsize;
    DmaRegs.CH6.SRC_BURST_STEP = srcbstep;
    DmaRegs.CH6.DST_BURST_STEP = desbstep;
    EDIS;
}

/* TI's: sets channel 6's transfer size (bursts - 1) and the steps between bursts. No callers. */
void DMACH6TransferConfig(Uint16 tsize, int16 srctstep, int16 deststep)
{
    EALLOW;
    DmaRegs.CH6.TRANSFER_SIZE = tsize;
    DmaRegs.CH6.SRC_TRANSFER_STEP = srctstep;
    DmaRegs.CH6.DST_TRANSFER_STEP = deststep;
    EDIS;
}

/* TI's: sets channel 6's source and destination wrap sizes (bursts - 1) and wrap steps. No
   callers. */
void DMACH6WrapConfig(Uint16 srcwsize, int16 srcwstep, Uint16 deswsize, int16 deswstep)
{
    EALLOW;
    DmaRegs.CH6.SRC_WRAP_SIZE = srcwsize;
    DmaRegs.CH6.SRC_WRAP_STEP = srcwstep;
    DmaRegs.CH6.DST_WRAP_SIZE = deswsize;
    DmaRegs.CH6.DST_WRAP_STEP = deswstep;
    EDIS;
}

/* TI's: sets channel 6's mode from its arguments, clears its flags and sets its PIE enable
   (7.6). No callers. */
void DMACH6ModeConfig(Uint16 persel, Uint16 perinte, Uint16 oneshot, Uint16 cont, Uint16 synce,
                       Uint16 syncsel, Uint16 ovrinte, Uint16 datasize, Uint16 chintmode, Uint16 chinte)
{
    EALLOW;
    DmaRegs.CH6.MODE.bit.PERINTSEL = persel;
    DmaRegs.CH6.MODE.bit.PERINTE = perinte;
    DmaRegs.CH6.MODE.bit.ONESHOT = oneshot;
    DmaRegs.CH6.MODE.bit.CONTINUOUS = cont;
    DmaRegs.CH6.MODE.bit.SYNCE = synce;
    DmaRegs.CH6.MODE.bit.SYNCSEL = syncsel;
    DmaRegs.CH6.MODE.bit.OVRINTE = ovrinte;
    DmaRegs.CH6.MODE.bit.DATASIZE = datasize;
    DmaRegs.CH6.MODE.bit.CHINTMODE = chintmode;
    DmaRegs.CH6.MODE.bit.CHINTE = chinte;
    DmaRegs.CH6.CONTROL.bit.PERINTCLR = 1;
    DmaRegs.CH6.CONTROL.bit.ERRCLR = 1;
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;
    EDIS;
}

/* TI's: starts channel 6. No callers. */
void StartDMACH6(void)
{
    EALLOW;
    DmaRegs.CH6.CONTROL.bit.RUN = 1;
    EDIS;
}
