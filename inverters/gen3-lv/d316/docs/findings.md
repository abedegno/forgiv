# Findings

What reading the byte-exact D316 source turned up: likely bugs (each re-read in the source
before it was listed, with a confidence label), dead code, mismatches with the ARM (A316) and open
questions. Effects in the inverter are inference. Nothing here is fixed; the tree stays byte-exact.
The battery behaviour itself is described in [battery.md](battery.md).

## Most important

- **The battery voltage tapers sit on the opposite limits** ([battery.md](battery.md), findings-power):
  the charge limit (BMS HR26) is cut below 48.0 V and the discharge limit (HR27) above 54.5 V. Near
  full the inverter does not taper charge by voltage, near empty it does not taper discharge.
- **In BMS mode the DSP has no battery voltage regulation.** Its only voltage protection is the
  HR98 trip (+2.0 V for 2 checks or +1.0 V for 50) and a 63.0 V check; HR332 (aging test) turns
  the 63.0 V check off.
- **Battery temperature has no effect on the DSP**: its over-temperature counter is never read.
- **sci_rx clears task 5's pending events on every ARM byte** (0xDAAC is os_tcb[5].events), so
  ARM link ticks and frame events can be lost.
- **The DSP update path has no image check**: per-frame 16-bit byte sums and the 0xAA word only.

## By area

- [Control, measurement, RCMU and grid](findings-control.md)
- [Power limits, battery link and states](findings-power.md)
- [ARM link, boot loader, RTOS and drivers](findings-comm.md)
