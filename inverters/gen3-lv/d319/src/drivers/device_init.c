/* device_init.c: the start-up set-up of the device. main calls init_system first; it runs TI's
   F2806x set-up code and this firmware's own peripheral set-up, and leaves interrupts off (main
   turns them on after the tasks exist).

   What it sets up:
     clocks   the crystal oscillator and the PLL x 9 / 2. Every timing in the image assumes
              SYSCLK = 90 MHz (ConfigCpuTimer's 90, DELAY_US's 11.111 ns), so the crystal is
              20 MHz. LSPCLK = SYSCLK / 4 = 22.5 MHz for the SCIs. The watchdog is off.
     flash    TI's InitFlash, run from RAM after init_system copies the ramfuncs to 0x8000
     GPIO     ePWM1A..5B on GPIO0..9, ePWM7 and ePWM8 on GPIO40, 42, 43 and 44, the trip-zone
              inputs on GPIO16, 17 and 50 (their third function),
              eCAP1 on GPIO24 (the grid period), SCI-A on GPIO28/29, SCI-B on GPIO18 (TX) and
              GPIO23 (RX), and the plain outputs (GPIO27, GPIO57 and others)
     ADC      external reference, all 16 SOCs on channels 0..15 started by ePWM4's SOCA,
              ADCINT1 at the end of SOC15, DMA channel 1 copying the results to adc_dma_buf
     ePWM     1, 3, 4, 5, 7 and 8 at 19.2 kHz (up-down count, TBPRD 2343); 3, 4 and 5 with a
              115-count dead band; ePWM4's interrupt (control_isr) and SOCA at counter zero (SOCA
              also at the period); all outputs start forced and tripped
     eCAP1    falling edges, an interrupt on each (grid_period_isr)
     SCI      both ports 9600 8N1, interrupt-driven (sci.c)
     timer 2  1 ms, the RTOS tick (timer2_isr)
     PIE      TI's default handlers first, then this firmware's nine handlers; IER 0x210E is
              INT2, INT3, INT4, INT9 and INT14

   TI's functions here, with what differs from TI's text as far as I know it: InitSysCtrl (the
   crystal and InitPll(9, 2) chosen), XtalOscSel (no 1 ms wait after the crystal is turned on,
   where the boot loader's inline copy has one), InitPll, InitPeripheralClocks (this firmware's
   clocks), InitPieCtrl (ends by turning the PIE on and acknowledging every group, which TI's
   EnableInterrupts does), InitPieVectTable (TI's own text), ConfigCpuTimer, AdcOffsetSelfCal,
   AdcChanSelect and AdcConversion (TI's F2806x_Adc.c). The rest is this firmware's own: sci_init,
   timer2_init, InitAdc, InitGpio, InitEPwm, epwm_trip_force, epwm_trip_clear, InitECap,
   adc_offset_measure and InitDmaAdc.

   Entry point: init_system (from main); epwm_trip_force and epwm_trip_clear are also used by the
   control and state code.
   Data: none of its own; it fills adc_offsets and clears adc_ch and meas_fast.
   Neighbours: rtos/os_main.c (main), ti/ (F2806x_SysCtrl.c InitFlash, F2806x_PieVect.c,
   F2806x_Dma.c, F2806x_usDelay.asm), sci.c, rtos/os_tick_isr.c, app/control/ (control_isr,
   tz_trip_isr), app/meas/ (grid_period_isr, the ADC users), app/util (iq_consts_init).
   name: descriptive (map/filenames.tsv: was init_b.c). It is one object, so TI's file names
   (F2806x_SysCtrl.c, F2806x_PieCtrl.c, F2806x_CpuTimers.c, F2806x_Adc.c, F2806x_Gpio.c,
   F2806x_EPwm.c) cannot each have their part.
   at: init_system 0x3ED38A; InitSysCtrl 0x3ED3FD; XtalOscSel 0x3ED419; InitPll 0x3ED42D;
       InitPeripheralClocks 0x3ED47F; InitPieCtrl 0x3ED4B5; InitPieVectTable 0x3ED4D6;
       sci_init 0x3ED919; timer2_init 0x3ED946; ConfigCpuTimer 0x3ED967; AdcOffsetSelfCal 0x3ED4E8;
       AdcChanSelect 0x3ED50F; AdcConversion 0x3ED567; InitAdc 0x3ED67D; InitGpio 0x3ED7B8;
       InitEPwm 0x3ED9BF; epwm_trip_force 0x3EDBBC, epwm_trip_clear 0x3EDC48; InitECap 0x3ED98A;
       adc_offset_measure 0x3EDCDC; InitDmaAdc 0x3EDCC1 */
#define OWN_meas_fast   /* match: a block of 44 words here (InitAdc clears it by index); with the header's struct meas_fast_t InitAdc compiles differently */
#include <string.h>
#include "F2806x_Device.h"
#include "F2806x_SysCtrl.h"
#include "F2806x_PieCtrl.h"
#include "F2806x_PieVect.h"
#include "F2806x_CpuTimers.h"
#include "F2806x_Sci.h"
#include "F2806x_Adc.h"
#include "F2806x_Gpio.h"
#include "F2806x_EPwm.h"
#include "F2806x_ECap.h"
#include "util.h"
#include "os.h"
#include "sci.h"
#include "settings.h"
#include "faults.h"
#include "control.h"
#include "state.h"
#include "init.h"
#include "F2806x_GlobalPrototypes.h"
#include "meas.h"

#define DELAY_US(A) DSP28x_usDelay(((((long double) A * 1000.0L) / (long double)11.111L) - 9.0L) / 5.0L)
#define Device_cal (void (*)(void))0x3D7C80

/* TI's DMA channel 1 set-up (F2806x_Dma.c): addresses, burst, transfer, wrap, mode, start */

extern struct adc_buf_c0c0_t { unsigned int w[44]; } meas_fast;

/* the linker's RamfuncsRunStart, RamfuncsLoadStart and RamfuncsLoadSize */

/* Sets up the device in order (see the file header): system control, the PIE, the ramfuncs
   copy and InitFlash, GPIO, ADC, DMA, SCI, timer 2, eCAP1, the forced trips, ePWM, the ADC
   offsets (after a 500 ms wait). Then the vectors: TINT2 timer2_isr, USER12 os_ctx_sw_isr (TRAP
   #31), SCI-A and SCI-B receive and transmit, EPWM4_INT control_isr, EPWM3_TZINT tz_trip_isr,
   ECAP1_INT grid_period_isr. PIE enables 2.3, 3.4, 4.1, 4.2 and 9.1..9.4. PIE 4.2 is ECAP2,
   which keeps TI's default handler; its clock is off, so it never fires. */
void init_system(void)
{
    DINT;
    IER = 0x0000;
    IFR = 0x0000;
    InitSysCtrl();
    InitPieCtrl();
    InitPieVectTable();
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (unsigned long)&RamfuncsLoadSize);
    InitFlash();
    InitGpio();
    InitAdc();
    InitDmaAdc();
    sci_init();
    timer2_init();
    InitECap();
    epwm_trip_force();
    InitEPwm();
    adc_offset_measure();
    EALLOW;
    PieVectTable.TINT2 = &timer2_isr;
    PieVectTable.USER12 = &os_ctx_sw_isr;
    PieVectTable.SCIRXINTA = &scia_rx_isr;
    PieVectTable.SCITXINTA = &scia_tx_isr;
    PieVectTable.EPWM4_INT = &control_isr;
    PieVectTable.EPWM3_TZINT = &tz_trip_isr;
    PieVectTable.ECAP1_INT = &grid_period_isr;
    PieVectTable.SCIRXINTB = &scib_rx_isr;
    PieVectTable.SCITXINTB = &scib_tx_isr;
    EDIS;
    DINT;
    IER = 0x0000;
    IFR = 0x0000;
    IER = 0x210E;
    PieCtrlRegs.PIEIER2.bit.INTx3 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx4 = 1;
    PieCtrlRegs.PIEIER4.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER4.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER9.bit.INTx2 = 1;
    PieCtrlRegs.PIEIER9.bit.INTx3 = 1;
    PieCtrlRegs.PIEIER9.bit.INTx4 = 1;
}

/* TI's: watchdog off, the ADC calibration from OTP (Device_cal, 0x3D7C80), the crystal, the
   PLL (x 9 / 2) and the peripheral clocks. */
void InitSysCtrl(void)
{
    EALLOW;
    SysCtrlRegs.WDCR = 0x0068;
    EDIS;
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
    (*Device_cal)();
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 0;
    EDIS;
    XtalOscSel();
    InitPll(9, 2);
    InitPeripheralClocks();
}

/* TI's: switches the clock source from the internal oscillator to the crystal and turns the
   internal oscillators off. There is no wait for the crystal to start here. */
void XtalOscSel(void)
{
    EALLOW;
    SysCtrlRegs.CLKCTL.bit.XTALOSCOFF = 0;
    SysCtrlRegs.CLKCTL.bit.XCLKINOFF = 1;
    SysCtrlRegs.CLKCTL.bit.OSCCLKSRC2SEL = 0;
    SysCtrlRegs.CLKCTL.bit.OSCCLKSRCSEL = 1;
    SysCtrlRegs.CLKCTL.bit.WDCLKSRCSEL = 1;
    SysCtrlRegs.CLKCTL.bit.INTOSC2OFF = 1;
    SysCtrlRegs.CLKCTL.bit.INTOSC1OFF = 1;
    EDIS;
}

/* TI's: sets the PLL multiplier val and the divider divsel, waiting for the PLL to lock. */
void InitPll(unsigned int val, unsigned int divsel)
{
    if (SysCtrlRegs.PLLSTS.bit.MCLKSTS != 0) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKCLR = 1;
        EDIS;
    }
    if (SysCtrlRegs.PLLSTS.bit.DIVSEL != 0) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.DIVSEL = 0;
        EDIS;
    }
    if (SysCtrlRegs.PLLCR.bit.DIV != val) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKOFF = 1;
        SysCtrlRegs.PLLCR.bit.DIV = val;
        EDIS;
        EALLOW;
        SysCtrlRegs.WDCR = 0x0068;
        EDIS;
        while (SysCtrlRegs.PLLSTS.bit.PLLLOCKS != 1) {
        }
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.MCLKOFF = 0;
        EDIS;
    }
    if ((divsel == 1) || (divsel == 2)) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.DIVSEL = divsel;
        EDIS;
    }
    if (divsel == 3) {
        EALLOW;
        SysCtrlRegs.PLLSTS.bit.DIVSEL = 2;
        DELAY_US(50L);
        SysCtrlRegs.PLLSTS.bit.DIVSEL = 3;
        EDIS;
    }
}

/* TI's, with this firmware's set: LSPCLK = SYSCLK / 4, XCLKOUT on GPIO18 (until InitGpio
   gives GPIO18 to SCI-B), and clocks for the ADC, eCAP1, ePWM1, 3, 4, 5, 7, 8, the CPU timers,
   DMA and both SCIs. ePWM2 and ePWM6 stay without a clock. */
void InitPeripheralClocks(void)
{
    EALLOW;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 3;
    SysCtrlRegs.LOSPCP.all = 0x0002;
    SysCtrlRegs.XCLK.bit.XCLKOUTDIV = 2;
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.ECAP1ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM3ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM4ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM5ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM7ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM8ENCLK = 1;
    SysCtrlRegs.PCLKCR3.bit.CPUTIMER0ENCLK = 1;
    SysCtrlRegs.PCLKCR3.bit.CPUTIMER1ENCLK = 1;
    SysCtrlRegs.PCLKCR3.bit.CPUTIMER2ENCLK = 1;
    SysCtrlRegs.PCLKCR3.bit.DMAENCLK = 1;
    SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 1;
    SysCtrlRegs.PCLKCR0.bit.SCIBENCLK = 1;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
}

/* TI's: turns the PIE off and clears every enable and flag, then turns it back on and
   acknowledges all groups. */
void InitPieCtrl(void)
{
    PieCtrlRegs.PIECTRL.bit.ENPIE = 0;
    PieCtrlRegs.PIEIER1.all = 0;
    PieCtrlRegs.PIEIER2.all = 0;
    PieCtrlRegs.PIEIER3.all = 0;
    PieCtrlRegs.PIEIER4.all = 0;
    PieCtrlRegs.PIEIER5.all = 0;
    PieCtrlRegs.PIEIER6.all = 0;
    PieCtrlRegs.PIEIER7.all = 0;
    PieCtrlRegs.PIEIER8.all = 0;
    PieCtrlRegs.PIEIER9.all = 0;
    PieCtrlRegs.PIEIER10.all = 0;
    PieCtrlRegs.PIEIER11.all = 0;
    PieCtrlRegs.PIEIER12.all = 0;
    PieCtrlRegs.PIEIFR1.all = 0;
    PieCtrlRegs.PIEIFR2.all = 0;
    PieCtrlRegs.PIEIFR3.all = 0;
    PieCtrlRegs.PIEIFR4.all = 0;
    PieCtrlRegs.PIEIFR5.all = 0;
    PieCtrlRegs.PIEIFR6.all = 0;
    PieCtrlRegs.PIEIFR7.all = 0;
    PieCtrlRegs.PIEIFR8.all = 0;
    PieCtrlRegs.PIEIFR9.all = 0;
    PieCtrlRegs.PIEIFR10.all = 0;
    PieCtrlRegs.PIEIFR11.all = 0;
    PieCtrlRegs.PIEIFR12.all = 0;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
    PieCtrlRegs.PIEACK.all = 0xFFFF;
}

/* TI's: copies PieVectTableInit (TI's default handlers) to the PIE vector table at 0x0D00,
   skipping the 3 reserved entries, and turns the PIE on. */
void InitPieVectTable(void)
{
    int i;
    unsigned long *Source = (void *)&PieVectTableInit;    /* TI's order (F2806x_PieVect.c) */
    unsigned long *Dest = (void *)&PieVectTable;
    Source = Source + 3;
    Dest = Dest + 3;
    EALLOW;
    for (i = 0; i < 125; i++) *Dest++ = *Source++;
    EDIS;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
}

/* SCI-A and SCI-B: 8 data bits, 1 stop bit, no parity, BRR 0x0124 (9600 baud at 22.5 MHz),
   receive, transmit and receive error interrupts on. */
void sci_init(void)
{
    SciaRegs.SCICCR.all = 0x0007;
    SciaRegs.SCICTL1.all = 0x0003;
    SciaRegs.SCIHBAUD = 0x0001;
    SciaRegs.SCILBAUD = 0x0024;
    SciaRegs.SCICTL2.bit.TXINTENA = 1;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;
    SciaRegs.SCICTL1.bit.RXERRINTENA = 1;
    SciaRegs.SCICTL1.all = 0x0023;
    ScibRegs.SCICCR.all = 0x0007;
    ScibRegs.SCICTL1.all = 0x0003;
    ScibRegs.SCIHBAUD = 0x0001;
    ScibRegs.SCILBAUD = 0x0024;
    ScibRegs.SCICTL2.bit.TXINTENA = 1;
    ScibRegs.SCICTL2.bit.RXBKINTENA = 1;
    ScibRegs.SCICTL1.bit.RXERRINTENA = 1;
    ScibRegs.SCICTL1.all = 0x0023;
}

/* CPU timer 2 as the RTOS tick: 1 ms at 90 MHz, interrupt on, started. */
void timer2_init(void)
{
    CpuTimer2.RegsAddr = &CpuTimer2Regs;
    CpuTimer2Regs.PRD.all = 0xFFFFFFFF;
    CpuTimer2Regs.TPR.all = 0;
    CpuTimer2Regs.TPRH.all = 0;
    CpuTimer2Regs.TCR.bit.TSS = 1;
    CpuTimer2Regs.TCR.bit.TRB = 1;
    CpuTimer2.InterruptCount = 0;
    ConfigCpuTimer(&CpuTimer2, 90, 1000);
    CpuTimer2Regs.TCR.bit.TSS = 0;
}

/* TI's: sets Timer's period to Freq (MHz) x Period (us) clocks, stopped, with its interrupt
   on. */
void ConfigCpuTimer(struct CPUTIMER_VARS *Timer, float Freq, float Period)
{
    unsigned long PeriodInClocks;
    Timer->CPUFreqInMHz = Freq;
    Timer->PeriodInUSec = Period;
    PeriodInClocks = (long)(Freq * Period);
    Timer->RegsAddr->PRD.all = PeriodInClocks - 1;
    Timer->RegsAddr->TPR.all = 0;
    Timer->RegsAddr->TPRH.all = 0;
    Timer->RegsAddr->TCR.bit.TSS = 1;
    Timer->RegsAddr->TCR.bit.TRB = 1;
    Timer->RegsAddr->TCR.bit.SOFT = 0;
    Timer->RegsAddr->TCR.bit.FREE = 0;
    Timer->RegsAddr->TCR.bit.TIE = 1;
    Timer->InterruptCount = 0;
}

/* TI's: trims the ADC offset (ADCOFFTRIM) by converting VREFLO with the internal reference. */
void AdcOffsetSelfCal(void)
{
    unsigned int AdcConvMean;
    EALLOW;
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 0;
    AdcRegs.ADCCTL1.bit.VREFLOCONV = 1;
    AdcChanSelect(13);
    AdcRegs.ADCOFFTRIM.bit.OFFTRIM = 80;
    AdcConvMean = AdcConversion();
    AdcRegs.ADCOFFTRIM.bit.OFFTRIM = 80 - AdcConvMean;
    AdcRegs.ADCCTL1.bit.VREFLOCONV = 0;
    EDIS;
}

/* TI's: points all 16 SOCs at channel ch_no. */
void AdcChanSelect(unsigned int ch_no)
{
    AdcRegs.ADCSOC0CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC1CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC2CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC3CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC4CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC5CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC6CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC7CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC8CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC9CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC10CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC11CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC12CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC13CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC14CTL.bit.CHSEL = ch_no;
    AdcRegs.ADCSOC15CTL.bit.CHSEL = ch_no;
}

/* TI's: converts 512 samples on all SOCs and returns their mean. */
unsigned int AdcConversion(void)
{
    unsigned int index, SampleSize, Mean, ACQPS_Value;
    unsigned long Sum;
    index = 0;
    SampleSize = 512;
    Sum = 0;
    Mean = 999;
    ACQPS_Value = 40;
    AdcRegs.ADCSOC0CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC1CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC2CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC3CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC4CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC5CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC6CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC7CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC8CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC9CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC10CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC11CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC12CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC13CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC14CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.ADCSOC15CTL.bit.ACQPS = ACQPS_Value;
    AdcRegs.INTSEL1N2.bit.INT1E = 1;
    AdcRegs.INTSEL1N2.bit.INT2E = 1;
    AdcRegs.INTSEL1N2.bit.INT1CONT = 0;
    AdcRegs.INTSEL1N2.bit.INT2CONT = 0;
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    AdcRegs.INTSEL1N2.bit.INT1SEL = 6;
    AdcRegs.INTSEL1N2.bit.INT2SEL = 14;
    AdcRegs.ADCINTSOCSEL1.bit.SOC0 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC1 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC2 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC3 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC4 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC5 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC6 = 2;
    AdcRegs.ADCINTSOCSEL1.bit.SOC7 = 2;
    AdcRegs.ADCINTSOCSEL2.bit.SOC8 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC9 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC10 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC11 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC12 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC13 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC14 = 1;
    AdcRegs.ADCINTSOCSEL2.bit.SOC15 = 1;
    DELAY_US(1000L);
    AdcRegs.ADCSOCFRC1.all = 0x00FF;
    while (index < SampleSize) {
        while (AdcRegs.ADCINTFLG.bit.ADCINT1 == 0) {}
        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
        Sum += AdcResult.ADCRESULT0;
        Sum += AdcResult.ADCRESULT1;
        Sum += AdcResult.ADCRESULT2;
        Sum += AdcResult.ADCRESULT3;
        Sum += AdcResult.ADCRESULT4;
        Sum += AdcResult.ADCRESULT5;
        Sum += AdcResult.ADCRESULT6;
        while (AdcRegs.ADCSOCFLG1.bit.SOC9 == 1) {}
        Sum += AdcResult.ADCRESULT7;
        while (AdcRegs.ADCINTFLG.bit.ADCINT2 == 0) {}
        AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
        Sum += AdcResult.ADCRESULT8;
        Sum += AdcResult.ADCRESULT9;
        Sum += AdcResult.ADCRESULT10;
        Sum += AdcResult.ADCRESULT11;
        Sum += AdcResult.ADCRESULT12;
        Sum += AdcResult.ADCRESULT13;
        Sum += AdcResult.ADCRESULT14;
        while (AdcRegs.ADCSOCFLG1.bit.SOC1 == 1) {}
        Sum += AdcResult.ADCRESULT15;
        index += 16;
    }
    AdcRegs.INTSEL1N2.bit.INT1E = 0;
    AdcRegs.INTSEL1N2.bit.INT2E = 0;
    while (AdcRegs.ADCSOCFLG1.all != 0) {}
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
    AdcRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;
    AdcRegs.ADCINTOVFCLR.bit.ADCINT2 = 1;
    AdcRegs.SOCPRICTL.bit.SOCPRIORITY = 1;
    while (AdcRegs.SOCPRICTL.bit.SOCPRIORITY != 1);
    AdcRegs.SOCPRICTL.bit.SOCPRIORITY = 0;
    while (AdcRegs.SOCPRICTL.bit.SOCPRIORITY != 0);
    Mean = Sum / SampleSize;
    return Mean;
}

/* Powers the ADC up (with the OTP calibration), runs the offset self-calibration, selects the
   external reference, calls iq_consts_init, then sets the 16 SOCs: SOCn converts channel n,
   started by ePWM4's SOCA, acquisition window 7 cycles, ADCINT1 at the end of SOC15. Clears
   adc_ch and meas_fast. */
void InitAdc(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
    (*Device_cal)();
    EDIS;
    EALLOW;
    AdcRegs.ADCCTL1.bit.ADCBGPWD = 1;
    AdcRegs.ADCCTL1.bit.ADCREFPWD = 1;
    AdcRegs.ADCCTL1.bit.ADCPWDN = 1;
    AdcRegs.ADCCTL1.bit.ADCENABLE = 1;
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 1;
    EDIS;
    DELAY_US(1000L);
    EALLOW;
    AdcRegs.ADCCTL2.bit.CLKDIV2EN = 1;
    AdcRegs.ADCCTL2.bit.CLKDIV4EN = 0;
    EDIS;
    DELAY_US(1000L);
    AdcOffsetSelfCal();
    EALLOW;
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 1;
    EDIS;
    iq_consts_init();
    EALLOW;
    AdcRegs.ADCCTL2.bit.ADCNONOVERLAP = 1;
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    AdcRegs.INTSEL1N2.bit.INT1E = 1;
    AdcRegs.INTSEL1N2.bit.INT1CONT = 0;
    AdcRegs.INTSEL1N2.bit.INT1SEL = 15;
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 1;
    AdcRegs.ADCSOC2CTL.bit.CHSEL = 2;
    AdcRegs.ADCSOC3CTL.bit.CHSEL = 3;
    AdcRegs.ADCSOC4CTL.bit.CHSEL = 4;
    AdcRegs.ADCSOC5CTL.bit.CHSEL = 5;
    AdcRegs.ADCSOC6CTL.bit.CHSEL = 6;
    AdcRegs.ADCSOC7CTL.bit.CHSEL = 7;
    AdcRegs.ADCSOC8CTL.bit.CHSEL = 8;
    AdcRegs.ADCSOC9CTL.bit.CHSEL = 9;
    AdcRegs.ADCSOC10CTL.bit.CHSEL = 10;
    AdcRegs.ADCSOC11CTL.bit.CHSEL = 11;
    AdcRegs.ADCSOC12CTL.bit.CHSEL = 12;
    AdcRegs.ADCSOC13CTL.bit.CHSEL = 13;
    AdcRegs.ADCSOC14CTL.bit.CHSEL = 14;
    AdcRegs.ADCSOC15CTL.bit.CHSEL = 15;
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC3CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC5CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC7CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC8CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC9CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC10CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC11CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC12CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC13CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC14CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC15CTL.bit.TRIGSEL = 11;
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC1CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC2CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC3CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC4CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC5CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC6CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC7CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC8CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC9CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC10CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC11CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC12CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC13CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC14CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC15CTL.bit.ACQPS = 6;
    EDIS;
    memset(&adc_ch, 0, sizeof(adc_ch));
    memset(&meas_fast, 0, sizeof(meas_fast));
}

/* The pin set-up (see the file header). The writes of 0 to GPASET, GPACLEAR and GPBSET bits
   (GPIO10, 11, 12, 21, 27, 52, 53) do nothing; the outputs start low because the output latches
   reset to 0. */
void InitGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.all = 0;
    GpioCtrlRegs.GPAPUD.all = 0xFFFFFFFF;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO5 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO6 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO7 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO9 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;
    GpioDataRegs.GPASET.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO11 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;
    GpioDataRegs.GPASET.bit.GPIO12 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO15 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 3;
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 3;
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 2;
    GpioCtrlRegs.GPAPUD.bit.GPIO23 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO23 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 3;
    GpioCtrlRegs.GPAPUD.bit.GPIO20 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO20 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO21 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO21 = 1;
    GpioDataRegs.GPASET.bit.GPIO21 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO22 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO25 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO26 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO27 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO27 = 1;
    GpioDataRegs.GPASET.bit.GPIO27 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;
    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO32 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO33 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO33 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO39 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO39 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO40 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO44 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO44 = 3;
    GpioCtrlRegs.GPBPUD.bit.GPIO41 = 0;
    GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO41 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO42 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO43 = 1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO50 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 3;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO50 = 3;
    GpioCtrlRegs.GPBPUD.bit.GPIO52 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO52 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO52 = 1;
    GpioDataRegs.GPBSET.bit.GPIO52 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO53 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO53 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO53 = 1;
    GpioDataRegs.GPBSET.bit.GPIO53 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO54 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO54 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO54 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO55 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO55 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO55 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO56 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO56 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO56 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO57 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO57 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO57 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO57 = 1;
    GpioCtrlRegs.GPBPUD.bit.GPIO58 = 1;
    GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO58 = 0;
    EDIS;
}

/* ePWM1, 3, 4, 5, 7 and 8 with their time bases stopped (TBCLKSYNC 0): up-down count, period
   2343 (19.2 kHz), shadowed compare loads at counter zero, the action qualifiers, the dead band
   on 3, 4 and 5 (115 counts each edge, active high complementary), every output forced by
   AQCSFRC, ePWM4's interrupt at counter zero and its SOCA at zero and period. Then the time
   bases start together. */
void InitEPwm(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;
    EPwm1Regs.TBCTR = 0;
    EPwm1Regs.TBPHS.half.TBPHS = 0;
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;
    EPwm1Regs.TBCTL.bit.PHSEN = 0;
    EPwm1Regs.TBCTL.bit.PRDLD = 0;
    EPwm1Regs.TBCTL.bit.SYNCOSEL = 1;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm1Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm1Regs.AQCSFRC.bit.CSFA = 1;
    EPwm1Regs.AQCTLA.bit.CAU = 2;
    EPwm1Regs.AQCTLA.bit.CAD = 1;
    EPwm1Regs.AQCSFRC.bit.CSFB = 1;
    EPwm1Regs.AQCTLB.bit.CBU = 2;
    EPwm1Regs.AQCTLB.bit.CBD = 1;
    EPwm3Regs.ETCLR.bit.INT = 1;
    EPwm3Regs.ETCLR.bit.SOCA = 1;
    EPwm3Regs.TBCTR = 0;
    EPwm3Regs.TBPHS.half.TBPHS = 0;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;
    EPwm3Regs.TBCTL.bit.PHSEN = 0;
    EPwm3Regs.TBCTL.bit.PRDLD = 0;
    EPwm3Regs.TBCTL.bit.SYNCOSEL = 1;
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm3Regs.AQCTLA.all = 0;
    EPwm3Regs.AQCTLB.all = 0;
    EPwm3Regs.AQCTLA.bit.CAU = 1;
    EPwm3Regs.AQCTLA.bit.CAD = 2;
    EPwm3Regs.AQCTLB.bit.CAU = 1;
    EPwm3Regs.AQCTLB.bit.CAD = 2;
    EPwm3Regs.DBCTL.bit.IN_MODE = 2;
    EPwm3Regs.DBCTL.bit.OUT_MODE = 3;
    EPwm3Regs.DBCTL.bit.POLSEL = 2;
    EPwm3Regs.AQSFRC.all = 0;
    EPwm3Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm3Regs.AQCSFRC.bit.CSFA = 1;
    EPwm3Regs.AQCSFRC.bit.CSFB = 2;
    EPwm3Regs.DBRED = 115;
    EPwm3Regs.DBFED = 115;
    EPwm3Regs.ETSEL.all = 0;
    EPwm3Regs.ETSEL.all = 0;
    EPwm4Regs.ETCLR.bit.INT = 1;
    EPwm4Regs.ETCLR.bit.SOCA = 1;
    EPwm4Regs.TBCTR = 0;
    EPwm4Regs.TBPHS.half.TBPHS = 0;
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm4Regs.TBCTL.bit.CTRMODE = 2;
    EPwm4Regs.TBCTL.bit.PHSEN = 0;
    EPwm4Regs.TBCTL.bit.PRDLD = 0;
    EPwm4Regs.TBCTL.bit.SYNCOSEL = 1;
    EPwm4Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm4Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm4Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm4Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm4Regs.AQCTLA.all = 0;
    EPwm4Regs.AQCTLB.all = 0;
    EPwm4Regs.AQCTLA.bit.CAU = 2;
    EPwm4Regs.AQCTLA.bit.CAD = 1;
    EPwm4Regs.AQCTLB.bit.CAU = 2;
    EPwm4Regs.AQCTLB.bit.CAD = 1;
    EPwm4Regs.DBCTL.bit.IN_MODE = 2;
    EPwm4Regs.DBCTL.bit.OUT_MODE = 3;
    EPwm4Regs.DBCTL.bit.POLSEL = 2;
    EPwm4Regs.AQSFRC.all = 0;
    EPwm4Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm4Regs.AQCSFRC.bit.CSFA = 1;
    EPwm4Regs.AQCSFRC.bit.CSFB = 2;
    EPwm4Regs.DBRED = 115;
    EPwm4Regs.DBFED = 115;
    EPwm4Regs.ETSEL.bit.INTEN = 1;
    EPwm4Regs.ETSEL.bit.INTSEL = 1;
    EPwm4Regs.ETSEL.bit.SOCAEN = 1;
    EPwm4Regs.ETSEL.bit.SOCASEL = 3;
    EPwm4Regs.ETPS.bit.INTPRD = 1;
    EPwm4Regs.ETPS.bit.SOCAPRD = 1;
    EPwm5Regs.TBCTR = 0;
    EPwm5Regs.TBPHS.all = 0;
    EPwm5Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm5Regs.TBCTL.bit.CTRMODE = 2;
    EPwm5Regs.TBCTL.bit.PHSEN = 0;
    EPwm5Regs.TBCTL.bit.PRDLD = 0;
    EPwm5Regs.TBCTL.bit.SYNCOSEL = 0;
    EPwm5Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm5Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm5Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm5Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm5Regs.AQCTLA.bit.CAU = 1;
    EPwm5Regs.AQCTLA.bit.CAD = 2;
    EPwm5Regs.AQCTLB.bit.CAU = 1;
    EPwm5Regs.AQCTLB.bit.CAD = 2;
    EPwm5Regs.DBRED = 115;
    EPwm5Regs.DBFED = 115;
    EPwm5Regs.DBCTL.bit.IN_MODE = 2;
    EPwm5Regs.DBCTL.bit.OUT_MODE = 3;
    EPwm5Regs.DBCTL.bit.POLSEL = 2;
    EPwm5Regs.AQSFRC.all = 0;
    EPwm5Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm5Regs.AQCSFRC.bit.CSFA = 1;
    EPwm5Regs.AQCSFRC.bit.CSFB = 2;
    EPwm5Regs.ETSEL.all = 0;
    EPwm5Regs.ETPS.all = 0;
    EPwm7Regs.ETCLR.bit.INT = 1;
    EPwm7Regs.ETCLR.bit.SOCA = 1;
    EPwm7Regs.TBCTR = 0;
    EPwm7Regs.TBPHS.all = 0;
    EPwm7Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm7Regs.TBCTL.bit.CTRMODE = 2;
    EPwm7Regs.TBCTL.bit.PHSEN = 0;
    EPwm7Regs.TBCTL.bit.PRDLD = 0;
    EPwm7Regs.TBCTL.bit.SYNCOSEL = 1;
    EPwm7Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm7Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm7Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm7Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm7Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm7Regs.AQCSFRC.bit.CSFB = 1;
    EPwm7Regs.AQCSFRC.bit.CSFA = 1;
    EPwm7Regs.AQCTLA.bit.CAU = 2;
    EPwm7Regs.AQCTLA.bit.CAD = 1;
    EPwm7Regs.AQCTLB.bit.CBU = 1;
    EPwm7Regs.AQCTLB.bit.CBD = 2;
    EPwm7Regs.ETSEL.bit.INTEN = 0;
    EPwm8Regs.ETCLR.bit.INT = 1;
    EPwm8Regs.ETCLR.bit.SOCA = 1;
    EPwm8Regs.TBCTR = 0;
    EPwm8Regs.TBPHS.all = 0;
    EPwm8Regs.TBCTL.bit.HSPCLKDIV = 0;
    EPwm8Regs.TBCTL.bit.CTRMODE = 2;
    EPwm8Regs.TBCTL.bit.PHSEN = 0;
    EPwm8Regs.TBCTL.bit.PRDLD = 0;
    EPwm8Regs.TBCTL.bit.SYNCOSEL = 0;
    EPwm8Regs.CMPCTL.bit.SHDWAMODE = 0;
    EPwm8Regs.CMPCTL.bit.SHDWBMODE = 0;
    EPwm8Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm8Regs.CMPCTL.bit.LOADBMODE = 0;
    EPwm8Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm8Regs.AQCSFRC.bit.CSFB = 1;
    EPwm8Regs.AQCSFRC.bit.CSFA = 1;
    EPwm8Regs.AQCTLA.bit.CAU = 2;
    EPwm8Regs.AQCTLA.bit.CAD = 1;
    EPwm8Regs.AQCTLB.bit.CBU = 1;
    EPwm8Regs.AQCTLB.bit.CBD = 2;
    EPwm8Regs.ETSEL.bit.INTEN = 0;
    EPwm1Regs.TBPRD = 2343;
    EPwm3Regs.TBPRD = 2343;
    EPwm4Regs.TBPRD = 2343;
    EPwm5Regs.TBPRD = 2343;
    EPwm7Regs.TBPRD = 2343;
    EPwm8Regs.TBPRD = 2343;
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
}

/* Trips ePWM1..6 at once by software (one-shot TZ1 and TZ3 selected, outputs low on a trip) with
   the ePWM3 trip interrupt off, so the bridges are off. init_system calls it before InitEPwm.
   ePWM2 and ePWM6 have no clock, so their writes do nothing. */
void epwm_trip_force(void)
{
    EALLOW;
    EPwm3Regs.TZEINT.bit.OST = 0;
    EPwm1Regs.TZSEL.bit.OSHT1 = 1;
    EPwm1Regs.TZSEL.bit.OSHT3 = 1;
    EPwm1Regs.TZCTL.bit.TZA = 2;
    EPwm1Regs.TZCTL.bit.TZB = 2;
    EPwm1Regs.TZFRC.bit.OST = 1;
    EPwm2Regs.TZSEL.bit.OSHT1 = 1;
    EPwm2Regs.TZSEL.bit.OSHT3 = 1;
    EPwm2Regs.TZCTL.bit.TZA = 2;
    EPwm2Regs.TZCTL.bit.TZB = 2;
    EPwm2Regs.TZFRC.bit.OST = 1;
    EPwm3Regs.TZSEL.bit.OSHT1 = 1;
    EPwm3Regs.TZSEL.bit.OSHT3 = 1;
    EPwm3Regs.TZCTL.bit.TZA = 2;
    EPwm3Regs.TZCTL.bit.TZB = 2;
    EPwm3Regs.TZFRC.bit.OST = 1;
    EPwm4Regs.TZSEL.bit.OSHT1 = 1;
    EPwm4Regs.TZSEL.bit.OSHT3 = 1;
    EPwm4Regs.TZCTL.bit.TZA = 2;
    EPwm4Regs.TZCTL.bit.TZB = 2;
    EPwm4Regs.TZFRC.bit.OST = 1;
    EPwm5Regs.TZSEL.bit.OSHT1 = 1;
    EPwm5Regs.TZSEL.bit.OSHT3 = 1;
    EPwm5Regs.TZCTL.bit.TZA = 2;
    EPwm5Regs.TZCTL.bit.TZB = 2;
    EPwm5Regs.TZFRC.bit.OST = 1;
    EPwm6Regs.TZSEL.bit.OSHT1 = 1;
    EPwm6Regs.TZSEL.bit.OSHT3 = 1;
    EPwm6Regs.TZCTL.bit.TZA = 2;
    EPwm6Regs.TZCTL.bit.TZB = 2;
    EPwm6Regs.TZFRC.bit.OST = 1;
    EDIS;
}

/* Clears the one-shot trips of ePWM1..6 and turns the ePWM3 trip interrupt (tz_trip_isr) back
   on. */
void epwm_trip_clear(void)
{
    EALLOW;
    EPwm1Regs.TZCLR.bit.INT = 1;
    EPwm1Regs.TZCLR.bit.OST = 1;
    EPwm1Regs.TZCTL.bit.TZA = 2;
    EPwm1Regs.TZCTL.bit.TZB = 2;
    EPwm2Regs.TZCLR.bit.INT = 1;
    EPwm2Regs.TZCLR.bit.OST = 1;
    EPwm2Regs.TZCTL.bit.TZA = 2;
    EPwm2Regs.TZCTL.bit.TZB = 2;
    EPwm3Regs.TZCLR.bit.INT = 1;
    EPwm3Regs.TZCLR.bit.OST = 1;
    EPwm3Regs.TZCTL.bit.TZA = 2;
    EPwm3Regs.TZCTL.bit.TZB = 2;
    EPwm4Regs.TZCLR.bit.INT = 1;
    EPwm4Regs.TZCLR.bit.OST = 1;
    EPwm4Regs.TZCTL.bit.TZA = 2;
    EPwm4Regs.TZCTL.bit.TZB = 2;
    EPwm5Regs.TZCLR.bit.INT = 1;
    EPwm5Regs.TZCLR.bit.OST = 1;
    EPwm5Regs.TZCTL.bit.TZA = 2;
    EPwm5Regs.TZCTL.bit.TZB = 2;
    EPwm6Regs.TZCLR.bit.INT = 1;
    EPwm6Regs.TZCLR.bit.OST = 1;
    EPwm6Regs.TZCTL.bit.TZA = 2;
    EPwm6Regs.TZCTL.bit.TZB = 2;
    EPwm3Regs.TZEINT.bit.OST = 1;
    EDIS;
}

/* eCAP1 for the grid period: capture mode, continuous, wrapping after event 1, falling edges
   (CAPxPOL 1), the counter never reset, an interrupt on event 1, re-armed and started. */
void InitECap(void)
{
    ECap1Regs.ECEINT.all = 0;
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 0;
    ECap1Regs.ECCLR.all = 0xFFFF;
    ECap1Regs.ECCTL1.bit.CAPLDEN = 0;
    ECap1Regs.ECCTL2.bit.CAP_APWM = 0;
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 0;
    ECap1Regs.ECCTL2.bit.STOP_WRAP = 0;
    ECap1Regs.ECCTL1.bit.CAP1POL = 1;
    ECap1Regs.ECCTL1.bit.CAP2POL = 1;
    ECap1Regs.ECCTL1.bit.CAP3POL = 1;
    ECap1Regs.ECCTL1.bit.CAP4POL = 1;
    ECap1Regs.ECCTL1.bit.CTRRST1 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST2 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST3 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST4 = 0;
    ECap1Regs.ECCTL2.bit.SYNCI_EN = 0;
    ECap1Regs.ECCTL2.bit.SYNCO_SEL = 0;
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;
    ECap1Regs.ECEINT.bit.CEVT1 = 1;
    ECap1Regs.ECCTL2.bit.REARM = 1;
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;
}

/* Waits 500 ms, then measures the zero offsets of ADC results 2, 3, 7, 8 and 15 as the mean of
   128 software-started conversions, and clamps them: 2 and 3 to 0..100, 7, 8 and 15 to
   1843..2252 (mid-scale 2048 +- 10 %). It waits only for ADCBSY to go high, not for the
   conversion to end, so each read may be of the previous conversion (docs/findings-comm.md). */
void adc_offset_measure(void)
{
    unsigned int i, j;
    for (i = 0; i < 500; i++) DELAY_US(1000L);
    for (j = 0; j < 128; j++) {
        AdcRegs.ADCSOCFRC1.all = 0x818C;
        while (AdcRegs.ADCCTL1.bit.ADCBSY == 0) {}
        adc_offsets.res8.sum += AdcResult.ADCRESULT8;
        adc_offsets.res3.sum += AdcResult.ADCRESULT3;
        adc_offsets.res2.sum += AdcResult.ADCRESULT2;
        adc_offsets.res7.sum += AdcResult.ADCRESULT7;
        adc_offsets.res15.sum += AdcResult.ADCRESULT15;
    }
    adc_offsets.res15.offset = adc_offsets.res15.sum >> 7;
    adc_offsets.res15.sum = 0;
    adc_offsets.res8.offset = adc_offsets.res8.sum >> 7;
    adc_offsets.res8.sum = 0;
    adc_offsets.res2.offset = adc_offsets.res2.sum >> 7;
    adc_offsets.res2.sum = 0;
    adc_offsets.res3.offset = adc_offsets.res3.sum >> 7;
    adc_offsets.res3.sum = 0;
    adc_offsets.res7.offset = adc_offsets.res7.sum >> 7;
    adc_offsets.res7.sum = 0;
    limiti(&adc_offsets.res2.offset, 100, 0);
    limiti(&adc_offsets.res3.offset, 100, 0);
    limiti(&adc_offsets.res8.offset, 2252, 1843);
    limiti(&adc_offsets.res7.offset, 2252, 1843);
    limiti(&adc_offsets.res15.offset, 2252, 1843);
}

/* DMA channel 1: bursts of 16 words from ADCRESULT0..15 to adc_dma_buf, two bursts per
   transfer; the source wraps back to ADCRESULT0 after each burst and the destination after
   two, so adc_dma_buf holds two sets of results. The mode is this firmware's
   DMACH1ModeConfig. */
void InitDmaAdc(void)
{
    DMACH1AddrConfig(adc_dma_buf, &AdcResult.ADCRESULT0);
    DMACH1BurstConfig(15, 1, 1);
    DMACH1TransferConfig(1, 1, 1);
    DMACH1WrapConfig(0, 0, 1, 0);
    DMACH1ModeConfig();
    StartDMACH1();
}

