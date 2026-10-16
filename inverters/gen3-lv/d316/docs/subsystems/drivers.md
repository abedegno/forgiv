# Drivers and device set-up

Files: `src/drivers/device_init.c`, `sci.c`, `sci_isr.c`, `pwm_force.c`, `src/include/sci.h`,
`src/include/init.h`, and TI's support files in `src/ti/` with the F2806x headers in
`src/include/`.

## Device set-up (init_system)

main calls init_system first. In order:

1. Interrupts off; InitSysCtrl: watchdog off, Device_cal (the ADC calibration in OTP), the
   crystal (XtalOscSel), the PLL x 9 / 2 and the peripheral clocks. Everything in the image
   assumes SYSCLK = 90 MHz, so the crystal is 20 MHz. LSPCLK = 22.5 MHz.
2. InitPieCtrl, InitPieVectTable (TI's default handlers everywhere).
3. The ramfuncs are copied from 0x3F1BB7 to 0x8000, then InitFlash runs from RAM (OTPWAIT 3).
4. InitGpio, InitAdc, InitDmaAdc, sci_init, timer2_init, InitECap.
5. epwm_trip_force (all bridges tripped), InitEPwm, adc_offset_measure (500 ms wait, then 128
   conversions).
6. The vectors: TINT2 timer2_isr, USER12 os_ctx_sw_isr, SCI-A and SCI-B receive and transmit,
   EPWM4_INT control_isr, EPWM3_TZINT tz_trip_isr, ECAP1_INT grid_period_isr.
7. IER = 0x210E (INT2, INT3, INT4, INT9, INT14) and the PIE enables 2.3, 3.4, 4.1, 4.2,
   9.1..9.4. Interrupts stay off until main's EINT.

### Pins

| Pins | Use |
|---|---|
| GPIO0..9 | ePWM1A..ePWM5B |
| GPIO40, 42, 43, 44 | ePWM7 and ePWM8 |
| GPIO16, 17, 50 | trip-zone inputs (third function) |
| GPIO24 | eCAP1, the grid period |
| GPIO28, 29 | SCI-A RX, TX: the ARM |
| GPIO23, 18 | SCI-B RX, TX: the BMS bus (GPIO18 is XCLKOUT until InitGpio) |
| GPIO57 | output: raised while the DSP drives the BMS bus (my reading: the RS-485 driver enable) |
| GPIO27 | output: raised by the update's 'E' and by power_limits.c in some fault cases |
| GPIO10, 11, 12, 15, 20, 21, 22, 25, 31, 39, 41, 52..56 | plain outputs, start low |
| GPIO26, 32, 33, 58 | plain inputs |

### ADC, ePWM, DMA, eCAP

- ePWM1, 3, 4, 5, 7 and 8 count up and down with TBPRD 2343: 19.2 kHz at 90 MHz. ePWM3, 4 and 5
  have a 115-count dead band (active high complementary). ePWM2 and ePWM6 have no clock.
- ePWM4 gives the control interrupt (control_isr) at counter zero and the ADC start (SOCA) at
  zero and at the period.
- All 16 ADC SOCs convert channels 0..15 on ePWM4's SOCA, with the external reference.
  DMA channel 1 copies ADCRESULT0..15 to adc_dma_buf, two sets per transfer.
- eCAP1 captures falling edges and interrupts on each (grid_period_isr).
- adc_offset_measure takes the zero offsets of results 2, 3, 7, 8 and 15 and clamps them (0..100
  for 2 and 3, 1843..2252 for the others).

### TI's code and what changed

| File | TI's | Changed here |
|---|---|---|
| ti/CodeStartBranch.asm | yes | branches to _boot_main instead of _c_int00 |
| ti/F2806x_CSMPasswords.asm | yes | a password instead of all 0xFFFF |
| ti/F2806x_DefaultIsr.c | yes | nothing I know of |
| ti/F2806x_Dma.c | yes | DMACH1ModeConfig takes no arguments and sets channel 1's mode itself |
| ti/F2806x_MemCopy.c | yes | nothing |
| ti/F2806x_PieVect.c | yes | nothing |
| ti/F2806x_SysCtrl.c | InitFlash only | OTPWAIT 3 instead of 5 |
| ti/F2806x_usDelay.asm | yes | nothing |
| drivers/device_init.c | InitSysCtrl, XtalOscSel, InitPll, InitPeripheralClocks, InitPieCtrl, InitPieVectTable, ConfigCpuTimer, AdcOffsetSelfCal, AdcChanSelect, AdcConversion | the crystal and PLL chosen; this firmware's clock set; XtalOscSel has no wait after the crystal starts; InitPieCtrl ends by enabling the PIE and acknowledging all groups |
| boot/boot_csm.c | CsmUnlock | the keys |
| boot/boot_flash_loader.c | the clock, PIE, flash and SCI-A set-up written inline | OTPWAIT 5 as TI has it |

The TI default handlers stop at ESTOP0 and loop. The watchdog is never enabled, so an interrupt
that reaches one stops the DSP until an outside reset. PIE 4.2 (eCAP2) is enabled with its
default handler, but eCAP2 has no clock and cannot interrupt.

## SCI drivers (sci.c, sci_isr.c)

Both ports run at 9600 8N1 (BRR 0x0124), with receive, transmit and receive error interrupts in
PIE group 9.

| Port | SCI | Use | Receive | Send |
|---|---|---|---|---|
| 0 | SCI-A | the ARM | ring 0, only frames that start with 0x43; EV5_FRAME to task 5 at 45 bytes | sci_send: a copy in sci_tx_buf0, one byte per TX interrupt, sci_tx_done = 1 at the end |
| 1 | SCI-B | the BMS (Modbus RTU) | ring 1, every byte; bms_link.c finds the frames | bms_link.c fills bms_tx_buf and calls sci_tx(1); bms_tx_state = 1 at the end |

### Rings

sci_rx_ring[port] is a SCI_RX: empty, count, wr, rd, buf, size, over sci_rx_buf[port] (0xD000,
150 words each). app_init sizes both rings to 150; bms_link.c resets ring 1 to 60 on errors.
sci_rx writes at wr and wraps it at size. ring_read reads at rd, and when the count reaches 0
the ring starts again at index 0. The read index itself never wraps: a read that reaches size
resets the ring.

A receive error (RXERROR) resets the port with a software reset. On SCI-A it also drops the
frame being sent (sci_tx_done = 1, busy = 0). The interrupt and sci_rx both do this, so it
happens twice.

### The BMS pass-through

bms_relay_active joins the two ports so that the ARM can send a battery firmware image to the
BMS through the DSP. arm_link_frame sets it when the BMS's HR14 (bms_relay_request, 0xD8E5)
reads 1, and tells the ARM in page 4 byte 28.

ARM to BMS: sci_rx(0) uses ring 0's wr as a byte counter.

| Byte of the relay frame | What sci_rx does |
|---|---|
| 0, 1, 2 | must be 'B', 'M', 'S', else it starts again; after 'S', GPIO57 goes high |
| 3 | dropped |
| 4, 5, 6 | kept in sci_end_tag (the tag: "HED" for data, "END" for the last frame) and forwarded |
| 7 .. 1032 | forwarded to SCI-B |
| 1033 | not forwarded; GPIO57 low, the counter starts again |

The A316's relay frames are "BMS", 1, "HED", 1024 data bytes, a 16-bit sum, "E": 1034 bytes. So
the BMS gets "HED", the data and the sum. The A316's last frame is "BMS", 1, "END" and a CRC-32,
which is shorter, so the DSP stays in the middle of a frame with GPIO57 high afterwards.

BMS to ARM: sci_rx(1) writes each byte to SCI-A and clears bms_relay_idle.

Neither direction waits for TXRDY; at the same baud rate each byte has left before the next one
arrives. While the relay runs, task 5 sends no 0x44 frames and bms_link.c does not poll. Only
arm_link_frame clears bms_relay_active, and it does not run during the relay, so the relay ends
with a reset: power_limits.c raises GPIO27 after 15000 passes without a byte from the BMS.

## PWM forcing (pwm_force.c)

Six ramfuncs set the action qualifier's continuous software force (AQCSFRC): pwm34_force_off and
pwm5_force_off force A low and B high (with the dead band, I read that as both outputs off),
pwm78_force_low forces all four outputs of ePWM7 and 8 low, and the release functions hand the
outputs back to the compare logic. The trip zones (epwm_trip_force, epwm_trip_clear in
device_init.c) act on top of this.

## Open points

See docs/findings-comm.md.
