/* F2806x_DefaultIsr.c: TI's default interrupt handlers (F2806x_DefaultIsr.c), in the file's
   order; I know of no change from TI's text. Each stops in the debugger (ESTOP0) and then loops
   for ever; EMPTY_ISR only returns. Their addresses fill PieVectTableInit (F2806x_PieVect.c),
   which InitPieVectTable copies to the PIE vector table; init_system then replaces nine of them
   with this firmware's handlers. The watchdog is off in this firmware, so an interrupt that
   reaches one of these stops the DSP until something outside resets it.

   Entry points: the handlers, through the PIE vector table only. Data: none.
   Neighbours: F2806x_PieVect.c, drivers/device_init.c (init_system's vectors).
   name: original (map/filenames.tsv): TI's file name and function names.
   at: 0x3F04FA to 0x3F0894 */

/* TI's default for vector 13, TINT1. */
interrupt void INT13_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 14, TINT2. init_system puts timer2_isr in its place. */
interrupt void INT14_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 15, DATALOG. */
interrupt void DATALOG_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 16, RTOSINT. */
interrupt void RTOSINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 17, EMUINT. */
interrupt void EMUINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 18, NMI. */
interrupt void NMI_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 19, ILLEGAL. */
interrupt void ILLEGAL_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 20, USER1. */
interrupt void USER1_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 21, USER2. */
interrupt void USER2_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 22, USER3. */
interrupt void USER3_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 23, USER4. */
interrupt void USER4_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 24, USER5. */
interrupt void USER5_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 25, USER6. */
interrupt void USER6_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 26, USER7. */
interrupt void USER7_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 27, USER8. */
interrupt void USER8_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 28, USER9. */
interrupt void USER9_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 29, USER10. */
interrupt void USER10_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 30, USER11. */
interrupt void USER11_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 31, USER12. init_system puts os_ctx_sw_isr in its place. */
interrupt void USER12_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 32, ADCINT1. */
interrupt void ADCINT1_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 33, ADCINT2. */
interrupt void ADCINT2_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 35, XINT1. */
interrupt void XINT1_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 36, XINT2. */
interrupt void XINT2_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 37, ADCINT9. */
interrupt void ADCINT9_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 38, TINT0. */
interrupt void TINT0_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 39, WAKEINT. */
interrupt void WAKEINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 40, EPWM1_TZINT. */
interrupt void EPWM1_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 41, EPWM2_TZINT. */
interrupt void EPWM2_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 42, EPWM3_TZINT. init_system puts tz_trip_isr in its place. */
interrupt void EPWM3_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 43, EPWM4_TZINT. */
interrupt void EPWM4_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 44, EPWM5_TZINT. */
interrupt void EPWM5_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 45, EPWM6_TZINT. */
interrupt void EPWM6_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 46, EPWM7_TZINT. */
interrupt void EPWM7_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 47, EPWM8_TZINT. */
interrupt void EPWM8_TZINT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 48, EPWM1_INT. */
interrupt void EPWM1_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 49, EPWM2_INT. */
interrupt void EPWM2_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 50, EPWM3_INT. */
interrupt void EPWM3_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 51, EPWM4_INT. init_system puts control_isr in its place. */
interrupt void EPWM4_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 52, EPWM5_INT. */
interrupt void EPWM5_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 53, EPWM6_INT. */
interrupt void EPWM6_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 54, EPWM7_INT. */
interrupt void EPWM7_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 55, EPWM8_INT. */
interrupt void EPWM8_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 56, ECAP1_INT. init_system puts grid_period_isr in its place. */
interrupt void ECAP1_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 57, ECAP2_INT. Its PIE enable (4.2) is set, but eCAP2 has no clock. */
interrupt void ECAP2_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 58, ECAP3_INT. */
interrupt void ECAP3_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 62, HRCAP1_INT. */
interrupt void HRCAP1_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 63, HRCAP2_INT. */
interrupt void HRCAP2_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 64, EQEP1_INT. */
interrupt void EQEP1_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 65, EQEP2_INT. */
interrupt void EQEP2_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 67, HRCAP3_INT. */
interrupt void HRCAP3_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 68, HRCAP4_INT. */
interrupt void HRCAP4_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 71, USB0_INT. */
interrupt void USB0_INT_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 72, SPIRXINTA. */
interrupt void SPIRXINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 73, SPITXINTA. */
interrupt void SPITXINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 74, SPIRXINTB. */
interrupt void SPIRXINTB_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 75, SPITXINTB. */
interrupt void SPITXINTB_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 76, MRINTA. */
interrupt void MRINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 77, MXINTA. */
interrupt void MXINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 80, DINTCH1. */
interrupt void DINTCH1_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 81, DINTCH2. */
interrupt void DINTCH2_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 82, DINTCH3. */
interrupt void DINTCH3_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 83, DINTCH4. */
interrupt void DINTCH4_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 84, DINTCH5. */
interrupt void DINTCH5_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 85, DINTCH6. */
interrupt void DINTCH6_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 88, I2CINT1A. */
interrupt void I2CINT1A_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 89, I2CINT2A. */
interrupt void I2CINT2A_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 96, SCIRXINTA. init_system puts scia_rx_isr in its place. */
interrupt void SCIRXINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 97, SCITXINTA. init_system puts scia_tx_isr in its place. */
interrupt void SCITXINTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 98, SCIRXINTB. init_system puts scib_rx_isr in its place. */
interrupt void SCIRXINTB_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 99, SCITXINTB. init_system puts scib_tx_isr in its place. */
interrupt void SCITXINTB_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 100, ECAN0INTA. */
interrupt void ECAN0INTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 101, ECAN1INTA. */
interrupt void ECAN1INTA_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 106, ADCINT3. */
interrupt void ADCINT3_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 107, ADCINT4. */
interrupt void ADCINT4_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 108, ADCINT5. */
interrupt void ADCINT5_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 109, ADCINT6. */
interrupt void ADCINT6_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 110, ADCINT7. */
interrupt void ADCINT7_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 111, ADCINT8. */
interrupt void ADCINT8_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 112, CLA1_INT1. */
interrupt void CLA1_INT1_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 113, CLA1_INT2. */
interrupt void CLA1_INT2_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 114, CLA1_INT3. */
interrupt void CLA1_INT3_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 115, CLA1_INT4. */
interrupt void CLA1_INT4_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 116, CLA1_INT5. */
interrupt void CLA1_INT5_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 117, CLA1_INT6. */
interrupt void CLA1_INT6_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 118, CLA1_INT7. */
interrupt void CLA1_INT7_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 119, CLA1_INT8. */
interrupt void CLA1_INT8_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 120, XINT3. */
interrupt void XINT3_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 126, LVF. */
interrupt void LVF_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's default for vector 127, LUF. */
interrupt void LUF_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's empty handler: returns at once. No vector uses it. */
interrupt void EMPTY_ISR(void)
{
}

/* TI's handler for the 13 reserved entries at the start of the table (0..12). */
interrupt void PIE_RESERVED(void)
{
    asm("      ESTOP0");
    for(;;);
}

/* TI's handler for the reserved PIE entries (26 of them). */
interrupt void rsvd_ISR(void)
{
    asm("      ESTOP0");
    for(;;);
}
