# Findings

This page lists what reading both firmwares turned up: likely bugs, surprising behaviour, dead
code, and questions that one chip's code answers about the other. Each item names its function and
file. Each also links to the per-tree findings, where I re-read it in the source before listing it:

- [A316 findings.md](../inverters/gen3-lv/a316/docs/findings.md)
- [D316 findings.md](../inverters/gen3-lv/d316/docs/findings.md), with
  [findings-control.md](../inverters/gen3-lv/d316/docs/findings-control.md),
  [findings-power.md](../inverters/gen3-lv/d316/docs/findings-power.md) and
  [findings-comm.md](../inverters/gen3-lv/d316/docs/findings-comm.md)

Confidence labels are the same as on the per-tree pages:

- **confirmed**: the behaviour follows from the code alone;
- **likely**: the code certainly does this, and it is probably not what was meant;
- **possible**: it depends on something outside the code (the board, timing, the other chip, the
  battery) or on intent I cannot see.

What a finding does in a running inverter is inference unless I say so. I have run none of this on
hardware. Nothing here is fixed in the sources: the trees stay byte-exact.

## Cross-chip findings

These come from reading the two trees together. The per-tree pages either do not have them or
leave them as open questions.

1. **A silent auxiliary MCU stops the DSP (confirmed code; effect possible).**
   - On the ARM, `fault_check_mode` ([monitor.c](../inverters/gen3-lv/a316/src/app/control/monitor.c))
     does not set run mode 3 for `FAULT_AUX_LINK` alone, so the ARM treats this fault as harmless.
   - The fault still reaches the DSP. `task_dsp_tx` sends `fault_flags >> 4` in header byte 2, and
     the DSP's `arm_frame_parse` turns its bit 2 (the aux link bit) into CF18 bit 8.
   - `select_state` ([ac_measure.c](../inverters/gen3-lv/d316/src/app/meas/ac_measure.c)) requests
     run mode 3 for any bit of CF18's high byte, so the DSP goes to its fault state while the aux
     link is down.
   - The ARM sets the bit after 20 s on grid without an aux page. Whether the aux link fails in
     service is not in the code.

   See [operation/faults.md](operation/faults.md#a-cross-chip-effect).
2. **The calibration end bits are BMS HR20 bits 3 and 2 and the DSP's over-voltage trip
   (confirmed).**
   - The A316 notes give 0x800, 0x400 and 0x002 of the warning word as inference.
   - The DSP builds that word from the low byte of BMS HR20 and from `batt_warning`, whose bit 1
     is CF15 bit 6 (`batt_warning_update`,
     [bms_link.c](../inverters/gen3-lv/d316/src/app/battery/bms_link.c)).

   See [operation/battery.md](operation/battery.md#calibration-the-bits-that-end-each-stage).
3. **IR49 "system mode" means "the DSP has no grid fault" (confirmed).** Page 0 byte 28 is 1 when
   `has_grid_fault()` is 0 (`arm_link_frame`,
   [arm_link.c](../inverters/gen3-lv/d316/src/app/comm/arm_link.c)). The ARM gates every charge
   and discharge slot on it.
4. **The ARM's lead-acid rule "SOC 0 when the DSP's battery state is 3 or more" never fires with
   D316 (confirmed).** The DSP always sends page 3 byte 30, which the ARM calls `batt_state_dsp`,
   as 0.
5. **HR332 means lead acid on the ARM and a burn-in test on the DSP (confirmed code; intent
   open).** HR332 = 1 sets HR54 to 0 on the ARM. On the DSP it sets `aging_test_enable`, which
   forces a charge or discharge and turns off the 63.0 V fast trip
   ([operation/battery.md](operation/battery.md#hr332-lead-acid-on-one-chip-a-burn-in-on-the-other)).
6. **A DSP image is accepted on its version word and per-frame sums alone (confirmed).**
   - The ARM checks only the first word of `DSPStore.bin` (`usb_check_update_files`).
   - The boot loader checks each frame's 16-bit byte sum, and the 0xAA word at the next reset.
   - There is no CRC or signature anywhere
     ([system/update-packages.md](system/update-packages.md)).
7. **The battery relay ends with a reset 5 s after the "END" frame (confirmed code).** The D316
   notes ask whether the 5-minute idle reset is the intended end of the relay.
   `status_led_and_reset` ([power_limits.c](../inverters/gen3-lv/d316/src/app/power/power_limits.c))
   also raises GPIO27 251 passes after the relay tag reads "END". The tag is `sci_end_tag`
   (0xCF42), read there as `upgrade_tag`, and the A316's last relay frame carries "END" in bytes
   4-6.
8. **Some values cross the link and are not used (confirmed).**
   - HR4000 reaches the DSP on page 7, and I found no reader there.
   - The ARM sends 0 in page 6 bytes 28-29, so the DSP's `cal_d819` is always 870, the minimum of
     its clamp. I found no reader of it either.
   - The ARM reads ten page fields that D316 always sends as 0
     ([reference/arm-dsp-link.md](reference/arm-dsp-link.md)).
9. **IR217 to IR224 carry the DSP's fault words with their bytes swapped (confirmed).** The DSP
   sends each word low byte first, and the ARM puts the first byte in the register's high byte
   ([operation/faults.md](operation/faults.md#on-the-arm)). This is not a bug, but it is a trap for
   anyone decoding these registers. HR351 = 1 also blanks the battery fault bytes (CF15) there.

## Protection and battery behaviour

10. **The battery voltage tapers sit on the opposite limits (confirmed code; likely a bug).** The
    charge limit (BMS HR26) is cut below 48.0 V, and the discharge limit (HR27) is cut above
    54.5 V (`current_limit_tapers`). Source: D316 battery.md and findings-power.
11. **With a BMS the DSP does not regulate the battery voltage (confirmed).** The 56.0/46.0 V loops
    run only with HR54 = 0 (`current_controller`, D316 control.md). With a BMS, the top of charge
    is controlled by:
    - BMS HR26;
    - HR20 bit 2;
    - the SoC taper;
    - the HR98 trip (+2.0 V for 2 checks or +1.0 V for 50), with the 63.0 V fast trip behind it.
12. **Battery temperature has no effect on the DSP (confirmed).** Its battery temperature counter
    is never read, and BMS HR24 goes only to the ARM (D316 findings-power).
13. **HR97 and HR98 are reset at every boot (likely).** `cfg_load_all` sets them to 43.20 V and
    58.50 V because a layout byte is never raised (A316 findings item 7). The DSP also clamps them
    to 20.0..48.0 V and 54.0..63.0 V.
14. **The battery soft-start timeout latches until the DSP restarts (likely).** Nothing clears
    CF15 bit 5 after start-up (`soft_start`, D316 findings-power item 1).
15. **Trip counters add up rather than count runs (likely).** These counters are reset only when
    they fire (D316 findings-power item 4, findings-control item 7):
    - the battery over- and under-voltage counts;
    - the 63.0 V and 560.0 V fast trips;
    - the instant 300 V grid trip;
    - the EPS short count.
16. **The 63.0 V fast trip is off while HR332 is set (confirmed).** See `fast_fault_check` (D316
    findings-control item 3).
17. **With the short BMS poll the DSP takes its current limits from the CRC (confirmed code;
    effect possible).** This happens only when HR109 is not 1 and the BMS firmware is above 3010
    (`bms_parse_hr`, D316 findings-power item 2).

## Robustness of the ARM's Modbus slave

18. **Reads outside the served ranges use an uninitialised length (confirmed).** With one read, any
    Modbus master, including the cloud through the dongle, can make the ARM compute a CRC over an
    arbitrary offset and store two bytes there. That can corrupt RAM or end in a reset (A316
    findings item 3).
19. **The RS485 slave expects the CRC high byte first (possible).** A standard Modbus master is
    then refused on RS485, unless GivEnergy's own tools send the bytes in that order (A316 item 14).
20. **An FC16 write of more than 73 registers repeats one byte (likely)** (A316 item 13).
21. **A stray UART4 byte clears the first byte of the RS485 receive buffer (likely)** (A316 item 8).

## Schedules and updates

22. **The half-hour tables are lost at a restart, and table mode then charges all day (likely)**
    (A316 findings item 12).
23. **The battery firmware relay sends a constant CRC-32 and times out on blocks of 0xFF bytes
    (confirmed)** (A316 items 1, 2 and 10).
24. **Setting the serial number from a stick overwrites HR138's low byte (confirmed)** (A316 item 4).

## Grid code

25. **The PF(P) curve (Q mode 3) never acts (confirmed).** Its power points are never written
    (D316 findings-control item 1).
26. **RoCoF trips once, on the first valid frequency, for the European codes (likely)** (D316
    findings-control item 6).
27. **The RCMU's continuous level is 300 mA, not 0.9 x 300 mA (confirmed).** Its step gate also
    compares the RMS with the mean's base (possible; D316 findings-control item 9).

## The DSP's scheduler

28. **`sci_rx` clears task 5's pending events on every ARM byte (high confidence).** Ticks and frame
    events can be lost while a frame arrives (D316 findings-comm item 1).

## Dead code worth knowing

- HR58, "auto judge battery type": `batt_auto_judge_type` has no caller, so the battery type never
  changes by itself (A316).
- The fault and event logs are never written, because `fault_log_push` and `event_log_push` have
  no caller (A316).
- `crc32_table_init` has no caller (A316; item 23 above).
- Nothing writes the BMS alarm bits of 0xD500 or the words 0xD502 and 0xD503, so CF15 bit 8, CF19
  bit 10 and a taper bypass never act. This looks like code for another BMS protocol (D316).
- On both chips, the RTOS's interrupt-exit switch and several RTOS calls have no callers: both
  schedulers are cooperative.

## Open questions

- What the board wires to these pins:
  - on the DSP, GPIO27 (a reset request?) and GPIO57 (the RS485 driver enable of the BMS bus?);
  - on the ARM, PD2, PC3, PC9 and PB5.
- What the auxiliary MCU, the device at Modbus 0x11 and the ARM boot loader do. None of them is in
  this repository.
- Which country each grid code stands for, beyond the inferences for 3, 4 and 19.
- What the BMS does with HR1 to HR4 and HR14, and whether a GivEnergy BMS checks the relay CRC.
- Whether `dcdc_ctrl_init` can run before task 2 has set its limits at a cold start (D316
  findings-control item 8).
