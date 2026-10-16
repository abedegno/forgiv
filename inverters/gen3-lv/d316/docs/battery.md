# How the D316 DSP handles the battery

This page describes what the DSP firmware of the GivEnergy Gen3 LV inverter (D316) does with the
battery. I read it from the matched C in this tree. Every rule below names the function it comes
from, so it can be checked. Where I say what a rule does in the inverter (beyond what the code
computes), that is inference and I mark it so.

Files: `src/app/battery/bms_link.c` (the Modbus link), `src/app/power/power_limits.c` (task 2:
limits, tapers, SoC floor, voltage checks, power command), `src/app/state/op_states.c` (soft
start, start checks, `has_batt_fault`). The interrupt-side code that acts on the results is in
`src/app/control/converter_ctrl.c` and `current_ctrl.c`.

## Units and conventions

- Voltages are in 0.1 V (540 = 54.0 V). BMS currents, BMS limits and current caps are in 0.01 A
  (800 = 8.00 A). Powers are in 0.1 W (36000 = 3.6 kW). The current bounds and the current command
  that go to the interrupt code are in 0.1 A on the high-voltage side of the DC-DC converter.
- Positive power and current mean charge. I checked this from `batt_power_blocks`: the
  force-charge block makes the command at least +300 W, and the full block makes it at most 0.
- "HRn" alone is an inverter holding register that the ARM sends to the DSP in its frames.
  "BMS HRn" is a battery register that the DSP reads over Modbus. The two sets overlap in
  numbers (for example inverter HR27 and BMS HR27 are different things).
- "BMS mode" means `dsp_settings.battery_type` (HR54) is not 0. "The BMS reports" means BMS
  HR19 bits 0 and 1 are not both 0 and the pack voltage (BMS HR22) is not 0.
- Task 2 runs every 20 ms, so 50 passes are 1 s. Task 6 (the BMS link) ticks every 40 ms. Task 0
  (the state machine) runs every 20 ms.

## Settings that come from the ARM

| DSP word | Inverter register | What the DSP does with it |
|---|---|---|
| `battery_type` 0xD8E1 | HR54 | 0: no BMS (fixed limits). Not 0: BMS mode |
| `bms_full_poll` 0xD8E2 | HR109 `enable_bms_read` | 1: full poll of BMS HR0..HR27; anything else: short poll of HR17..HR25 |
| `batt_capacity_ah` 0xD8E0 | HR55 (Ah) | the capacity for the C-rate caps. With `battery_type` 1, HR109 not 0 and 50 or more replies, BMS HR11 replaces it on every page 0 frame (`arm_page0`) |
| `batt_charge_limit_pct` 0xD8C5 | HR111, 1..100 % | charge cap and forced charge cap |
| `batt_discharge_limit_pct` 0xD8C6 | HR112, 1..100 % | discharge cap and forced discharge cap |
| `batt_max_charge_current` 0xD400 | HR309 x 100, 10..80 A (default 60 A) | the ceiling of the charge cap; + 15 A for the discharge cap |
| `batt_power_rating` 0xD892 | HR308 x 10, 1.0..6.0 kW (default 3.6 kW) | the battery power rating; the discharge rating is this + 300 W |
| `batt_max_voltage` 0xD8DE | HR98 x 0.1, 54.0..63.0 V (x 1.05 in a calibration) | the over-voltage trip level in BMS mode |
| `batt_min_voltage` 0xD8DF | HR97 x 0.1, 20.0..48.0 V (x 0.86 in a calibration, x 0.5 with HR175) | the under-voltage level and the start permit |
| `discharge_min_soc` 0xD8C9 | HR110, 1..100 % | the SoC floor in grid modes; forced discharge stops here |
| `soc_reserve` 0xD8C8 | HR114 | the SoC floor in other modes |
| `charge_target_soc_now` 0xD8C7 | page 7, 20..100 % | forced charge stops here |
| `force_charge_pct`, `force_discharge_pct` 0xD896/0xD898 | HR313, HR314, 0..100 % | forced charge and discharge power |
| `batt_force_mode` 0xD8CE | frame byte 4, 0..2 | 0 normal (the ARM setpoint), 1 forced charge, 2 forced discharge |
| `batt_power_mode` 0xD8D3 | HR27 `battery_power_mode` | 0 makes the mode 0 command 0 |
| `batt_cal_stage` 0xD907 | HR29 (calibration stage) | not 0: calibration rules, see below |
| `batt_cal_request` 0xD909 | HR175 | 1 forces charging and wakes the BMS, see below |
| `aging_test_enable` 0xD8EF | HR332 | a burn-in mode, see below |
| `enable_charge_target`, `charge_target_soc` | HR20, HR116 | end forced charge when PV is faulted and SoC is above HR116 |

The HR numbers come from the ARM's frame builder and the DSP's page handlers
(docs/names/B-notes.md, `arm_link.c`, `arm_page_write.c`). The ranges are the clamps in the DSP's
page handlers.

## The BMS link (bms_link.c)

**Requests.** Task 6 sends one request every 6th tick, so every 240 ms, to slave 1 on SCI-B.
`GPIO57` (the RS485 driver enable, per sci.c) is raised before sending. In order of priority:

1. FC6 BMS HR1 = 1, once every 21 polls (about 5 s), from the moment `batt_cal_request` (HR175)
   goes from 0 to 1.
2. FC6 BMS HR1 = 0, on polls 2, 4, 6, 8 and 10 after HR175 goes back to 0.
3. FC6 BMS HR4 = 1, on every poll while `hr201_request` (ARM page 5) is 1.
4. FC6 BMS HR2 = `bms_update_pending` every 11th poll while that is not 0, or FC6 BMS HR3 =
   `hr200_request` every 22nd poll while that is not 0 (both from ARM page 4).
5. A 6-byte request the ARM forwards (ARM page 6), sent as it is; its register count, capped at
   26, sets the expected length of an FC4 reply.
6. Otherwise the poll: FC3 from HR0, 28 registers (full poll), or FC3 from HR17, 9 registers
   (short poll).

The fall of HR175 also sets CF15 bit 3 (a battery fault for 10 s, see "Voltage checks") and
posts a stop to task 0.

**Replies.** The receive side runs on every wake-up of task 6. It accepts a frame only if:
byte 0 is below 0x10 (the address is not compared with 1; any of 0..15 passes), byte 1 is 3 or 4,
the length is exactly 2 x count + 5 for FC3 (count = the current poll's count, 28 or 9) or
2 x (count + 3) for FC4, and the Modbus CRC (low byte first) matches. Anything else, an FC6 echo
included, restarts the frame. The ring is emptied when the next request is built, so a reply has
to be complete within the 240 ms before the next poll.

A good FC3 reply goes to `bms_parse_hr`, a good FC4 reply to `bms_fc4_reply` (a copy for the
ARM). Both set `bms_present` = 1 and clear the loss counter.

**Timeout and the comms fault.** `bms_lost_count` counts ticks since the last good reply. At 750
ticks (30 s) `bms_lost` zeroes pack voltage, current, temperature, SoC, both limits, BMS HR17,
HR18 and HR20, clears `bms_present` and `bms_reply_count`, and in BMS mode sets CF15 bit 9 (the
comms fault). HR19 and the two alarm bits of 0xD500 keep their last values. Because the pack
voltage is now 0, task 2 treats the BMS as not reporting.

**Recovery.** `bms_parse_hr` clears CF15 bit 9 on each of the first 50 good replies after a loss
(`bms_reply_count` then stops at 50). So one good reply clears the fault; there is no debounce on
the DSP side. While HR175 is set, each of those 50 replies also posts a stop to task 0.

**Pass-through.** While `bms_relay_active` is 1 the task does nothing and SCI-A is wired to SCI-B
in the interrupt code (sci.c). `arm_link.c` sets it when BMS HR14 (`bms_hr14`, read in the full
poll) is 1. `status_led_and_reset` resets the DSP (GPIO27) after 15001 passes (5 min) of it.

## What the DSP takes from each BMS register (bms_parse_hr)

In the full poll the HR17.. fields start 0x22 bytes in; in the short poll they start at byte 3.

| BMS register | Poll | Stored in | Used for |
|---|---|---|---|
| HR0 | full | `bms_hr0` 0xD8E3 | sent to the ARM |
| HR5..HR9 | full | `bms_serial` (10 bytes) | sent to the ARM |
| HR11 | full | `bms_capacity` 0xD8E7, capped at 10000 | replaces HR55 after 50 replies (C-rate caps) |
| HR13 | full | `bms_fw_version` 0xD8E4 | above 3010: limits from HR26/HR27; else from HR25 |
| HR14 | full | `bms_hr14` 0xD8E5 | 1 starts the ARM's battery firmware relay |
| HR15 | full | `bms_hr15` | 0xFFFF reads as 0; bit 0 kept only while the last SoC was 100 or more |
| HR17, HR18 | both | `bms_hr17`, `bms_hr18` | nothing (never read) |
| HR19 | both | 0xD501 | bits 0-1 "reports", bit 2, bit 4, bit 5 (below) |
| HR20 | both | `bms_alarm` | bit 2 -> 0xD500 bit 1 ("full"), bit 3 -> 0xD500 bit 2 ("empty"); low byte to the ARM |
| HR21 low byte | both | `bms.soc` 0xD506 | SoC (%) |
| HR22 | both | `bms.pack_voltage`, x 0.1 | so HR22 in 0.01 V gives 0.1 V, truncated |
| HR23 | both | `bms.pack_current` | signed, 0.01 A |
| HR24 | both | `bms.pack_temp_x10`, x 10 | sent to the ARM only |
| HR25 | both | both limits | only with HR13 3010 or lower |
| HR26 | full | `bms.bms_charge_limit` 0xD504 | charge limit, 0.01 A (HR13 above 3010) |
| HR27 | full | `bms.bms_discharge_limit` 0xD505 | discharge limit, 0.01 A (HR13 above 3010) |

HR26 and HR27 are not in the short poll. With the short poll and HR13 above 3010 the parser
still reads them, from the CRC and from bytes past the 23-byte reply (see
docs/findings-power.md).

During a calibration (`batt_cal_stage` not 0) the parser holds SoC to HR110 + 1 .. 99 and raises
both limits to at least 800 (8.00 A).

## Current limits and tapers (current_limit_tapers, every 20 ms)

In BMS mode, with V the DSP's own battery voltage:

- Charge cap = capacity (Ah) x HR111 % + 1.50 A, at most HR309.
  In code units: `batt_capacity_ah * batt_charge_limit_pct + 150`, at most `batt_max_charge_current`.
- Discharge cap = capacity x HR112 % + 1.50 A, at most HR309 + 15.00 A.
- Charge limit = BMS HR26 x (1 - f), where f follows (48.0 V - V) x 0.0225 through a filter
  (f = 0.98 f + 0.02 x new, each pass) and is held to 0..0.9. So the HR26 limit is whole at 48.0 V
  and above and falls to 10 % at 44.0 V and below. Then at most the charge cap.
- Discharge limit = BMS HR27 x (1 - g), where g follows (V - 54.5 V) x 0.0257 through the same
  filter, held to 0..0.9. So the HR27 limit is whole at 54.5 V and below and falls to 10 % at
  58.0 V and above. Then at most the discharge cap.
- With BMS firmware 3010 or lower, HR25 stands in for both HR26 and HR27.
- The charge limit is then held to 1.00..80.00 A and the discharge limit to 2.00..80.00 A.
- Without a BMS both limits are 60.00 A before that clamp.

The low-voltage taper is on the charge limit and the high-voltage taper on the discharge limit.
That is what the code computes, and the charge side really is charge: the same minimum (`x` at the
end of current_limit_tapers) also takes the SoC taper over 90..100 % and the PV-surplus limit,
which only make sense for charging. So near full (above 54.5 V) the inverter does not taper the
charge current by voltage at all, and near empty (below 48.0 V) it does not taper the discharge
current by voltage. The usual design is the other way round, so I take the two tapers to be
swapped by mistake (findings-power.md); a deliberate gentle charge of a deeply flat pack would
explain the charge side but not the discharge side. Either way, the voltage end of the charge and
discharge current is left to the BMS's own HR26/HR27, the SoC taper and the HR98 trip.

A bypass exists in the code: if bits 6 or 7 of 0xD502 were set, the charge limit would be
min(HR26, 20 % of HR309) with no taper and no cap; bits 1, 3, 6 or 7 would do the same for
discharge. Nothing in the image writes 0xD502, so the tapers always apply.

Worked example (HR111 = HR112 = 50 %, 160 Ah, HR309 = 60 A): charge cap = 80 + 1.5 = 81.5 A,
held to 60 A; discharge cap = 81.5 A, held to 75 A. At V = 52 V and HR26 = HR27 = 100 A the limits
are 60 A and 75 A.

## Power limits and the current bounds

Each side's power limit is the smallest of (all 0.1 W):

- the current limit x V (`0.01 x I x V`), at most HR308 (charge) or the discharge rating
  (HR308 + 300 W);
- the charger temperature derate: above 87.0 degrees, 5 % of the rating less per degree;
- the bus derate: from a bus of 520.0 V, the filtered excess E (0..50.0 V) gives
  rating x (1 - 0.0018 x E), so 10 % at 50 V over;
- the SoC limits (`soc_power_limits`, only while the BMS reports): charge = HR308 x
  (1 - 0.095 x (SoC - 90)) with SoC - 90 held to 0..8, so 100 % up to 90 % SoC and 24 % from 98 %;
  HR20 bit 2 makes it 0. Discharge = the discharge rating, or 10 % of it with HR20 bit 3;
- for charge only, `chg_plim_pv`: the rating minus the larger of two PI outputs, one on PV power
  over its rating and one on a PV current over 18 A; it moves at most 10 W per pass. This runs
  in modes 0 and 2; in forced charge it is the rating.

The current bounds the interrupt code clamps to are bound = 10 x power / V_hv (V_hv held to
200..560 V): `isr_limits.chg_current_bound` (0xC212) and `dis_current_bound` (0xC214, negative).
Then:

- schedule window 1 (ARM page 7) makes the charge bound 0.25 A; window 2 makes the discharge
  bound -0.25 A;
- BMS HR19 bit 5, outside a calibration, makes the charge bound 0.1 A, or, when more than 1.0 A of
  discharge is allowed, a ramp between -0.5 A and 0.1 A that moves 0.05 A per pass, down while the
  bus is under 520.0 V;
- the charge bound is held to -0.5..15.0 A and the discharge bound to -15.0..0 A (HV side).

`current_ctrl.c` clamps the converter's current reference between the two bounds. In BMS mode the
controller has no battery voltage loop: the 56.0 V and 46.0 V loops (`ctl_voltage_limits_set`)
run only when `battery_type` is 0.

## The power command (batt_power_setpoint)

The command is worked out only in converter mode 5 with op_state 5 (grid with PV) or mode 6 with
op_state 6 (grid on battery), with no battery fault. Elsewhere it is 0. The mode is
`batt_force_mode`; HR175 or aging mode 1 force mode 1, aging mode 2 forces mode 2, and mode 1
falls to 0 when HR20 (`enable_charge_target`) is 1, PV is faulted and SoC is above HR116.

- Mode 0: each time the ARM's setpoint changes, command = -(measured battery power) - 0.5 x
  setpoint. It is 0 while HR27 `battery_power_mode` is 0 or CF19 bit 2 is set. Held to +-HR308.
- Mode 1 (forced charge): HR308 x HR313 %, at most capacity x HR111 % x V. In BMS mode it becomes
  0 at `charge_target_soc_now` unless BMS HR15 bit 0 is set (1000 W in a calibration).
- Mode 2 (forced discharge): -(HR308 + 300 W) x HR314 %, at most capacity x HR112 % x V in size. In
  BMS mode it becomes 0 at SoC at or below HR110.

Then, in this order: a grid frequency or voltage response replaces it while running; until the
grid soft-start ramp is done (run_status bit 4) its size is held to `lim_soft_start`; the blocks
(`batt_power_blocks`, below) apply; the current command = 10 x P / V_hv, held to +-15.0 A.

**Blocks** (`batt_power_blocks`, first match wins):

1. Force charge (CF19 bit 9), or BMS mode with HR19 bit 2 clear outside a calibration and the
   aging test: a command of 300 W or less becomes +300 W.
2. No discharge (CF19 bit 11): a negative command becomes 0.
3. Full (CF19 bit 8): the command is at most 0.

After that, BMS HR19 bit 5 (outside a calibration) makes the command at most -120 W.

## SoC floor, force charge and full (soc_floor)

In BMS mode while the BMS reports:

- The floor L is HR114. In run modes 5 and 6 it is raised to HR110 if HR110 is higher (the
  raised value is written back into the HR114 copy). F = L - 3, or 1 when L is 4 or less.
- Force charge, CF19 bit 9: set after 250 passes (5 s, counted in total) with SoC at or below F;
  cleared at once at SoC F + 4 or more. Effect: at least 300 W of charge (blocks, above).
- No discharge, CF19 bit 11: set after 250 passes with SoC at or below F + 3; cleared at F + 7.
- With L at most 4 (HR114 4 is the boot default): no discharge from 4 %, cleared at 8 %; force
  charge from 1 %, cleared at 5 %.
- Full, CF19 bit 8: set after 1500 passes (30 s) with any of HR26 = 0, SoC at or above 100, 0xD502
  bit 0 (never written) or HR20 bit 2, and no comms fault. Cleared after 251 passes with SoC below
  99. BMS HR15 bit 0 clears it on every pass outside a calibration. Effect: no charge (blocks),
  and `full_current_trim` runs.
- `full_current_trim`: every 11th pass while full, a PI on BMS HR23 builds a 0..2.0 A trim (HV
  side) that the current controller subtracts from its reference. 251 readings at -5.00 A or less
  reset it.
- In a calibration CF19 bits 8, 9 and 11 are cleared on every pass.

When the BMS does not report, bits 8, 9, 10 and 11 of CF19 and CF15 bit 8 are cleared. Without a
BMS only bits 8 and 9 are cleared.

Bits 9 and 11 also latch an EPS block (`soc_floor_latch` bit 3) after 51 passes; it is released
30001 passes (10 min) after both clear, or at once when a grid fault is present.

## Voltage checks (batt_voltage_monitor) and the faults

Each CF15 bit is a battery fault. `has_batt_fault` is true for any CF15 bit. Then the interrupt
code sets the converter mode to 0 (`converter_mode`, converter_ctrl.c), except during the
low-battery wake or aging mode 1, and `soft_start_permit` withdraws `soft_start_enable`.

| Bit | Set | Cleared |
|---|---|---|
| 1 mismatch | BMS mode: V under 24.0 V at once; or V and BMS HR22 more than 5.0 V apart while the BMS reports, HR22 is not 0 and BMS HR23 is under 2.00 A in size, 5 checks. No BMS: V under 20.0 V, 5 checks | 500 checks (10 s) within 3.0 V; no BMS: over 30.0 V, 5 checks |
| 2 under-voltage | V under HR97 for 5 checks (not counted while the BMS reports idle, HR19 bits 0-1 both 0). Also held set while fewer than 50 replies have come in and `low_batt_wake` is 1 | 150 checks (3 s) above HR97 + 2.0 V; 500 with `low_batt_wake` |
| 3 calibration end | the fall of HR175 (bms_link.c) | after 501 passes (10 s) |
| 4 DC-DC ratio | converter mode 5 only: V_hv 50.0 V or more away from 5.5 x V for 50 passes in a row | 500 passes after it was set |
| 5 soft-start timeout | 8001 soft-start calls (160 s) without finishing | never: only `state_init` (once, at start-up) clears CF15, so it holds until the DSP restarts |
| 6 over-voltage | V over max + 2.0 V for 2 checks, or over max + 1.0 V for 50 checks; max = HR98 in BMS mode, 56.0 V without | 500 checks (10 s) under max |
| 8 BMS alarm | the OR of 0xD500 bits 0, 3, 5, 8, 12, which nothing writes | always 0 |
| 9 comms | 30 s without a good reply (BMS mode) | the first good reply |
| 15 | schedule window 3 or BMS HR19 bit 4 | when neither |

The counts of bits 2 and 6 are totals: a counter is reset only when it fires, not when the voltage
comes back. So "50 checks over max + 1.0 V" means 50 such checks since the last trip, however far
apart. The clear counter for bit 6 also runs while the bit is clear. My earlier Ghidra notes
(unpublished) ran this and agree.

With HR98 = 58.5 V: bit 6 at 60.5 V for 40 ms in total or 59.5 V for 1 s in total, cleared after
10 s under 58.5 V.

There is one more over-voltage check in the interrupt code (`converter_mode`, converter_ctrl.c):
the battery over 63.0 V for 41 samples (counted in total) trips the DC-DC's PWM and stops the
state machine. It is off while HR332 is set.

**Start permit** (`bms.start_permit`, 0xD52D): in BMS mode, 500 passes (10 s) with the BMS
reporting and HR22 above HR97 - 2.0 V; cleared at once otherwise. Without a BMS it is always 1.

## Soft start and starting (op_states.c)

- `soft_start_permit`: `soft_start_enable` = 1 when there is no battery fault and the HV side
  reads 220.0 V or more, or after 6000 passes (120 s) without a fault. Any battery fault clears it.
- The run states 5..8 and the start checks call `soft_start` only while `soft_start_enable` is 1.
- `soft_start` runs while (start permit and no battery fault), or `low_batt_wake`, or aging mode 1.
  It raises the DC-DC duty by 1 per call, or by 50 while V_hv is within 10.0 V of 5.5 x V, to
  1054, and then reports done. After 8001 calls (160 s) without finishing it sets CF15 bit 5.
- Grid with PV (mode 5) can start with a battery fault: `start_grid_pv` then precharges the bus
  from PV and runs without the battery. Modes 6, 7 and 8 need the battery: the EPS starts need no
  battery fault, and all three need the soft start.
- `state0_idle`: with HR175 set and the BMS answering, modes other than 5 and 6 wait 1501 passes
  (30 s) before starting.

## Forced charge and discharge, HR175 and the low-battery wake

- Forced charge and discharge are `batt_force_mode` 1 and 2 (above). The ARM sets the mode in
  every frame.
- HR175 (`batt_cal_request` = 1) forces mode 1, makes the BMS link write BMS HR1 = 1 every 5 s
  (a wake request, by its use), and halves the HR97 copy (the ARM page 1 handler). `arm_link.c`
  sets `low_batt_wake` from it: to 1 after 200 frames with the battery between 40.0 and 46.0 V,
  back to 0 after 200 frames above 46.0 V, and straight to the HR175 value when the battery is at
  or under 40.0 V or the BMS comms fault is set. HR175 = 1 with the larger PV input under 160 V
  clears both. With `low_batt_wake`, the soft start runs without the permit, the EPS start waits
  up to 300 s and precharges from PV, the under-voltage bit is held set until 50 replies are in,
  and the interrupt code drives the DC-DC to a fixed 56.0 V at the battery terminal
  (converter_ctrl.c). This looks like a way to wake a flat battery from PV (inference).

## Calibration (HR29, batt_cal_stage)

While `batt_cal_stage` is not 0:

- SoC is held to HR110 + 1 .. 99 and both BMS limits are raised to at least 8.00 A (parser).
- The SoC charge taper is 0.075 per % over 90..100 % (25 % at 100 %), and HR20 bit 2 gives 5 % of
  HR308 instead of 0.
- CF19 bits 8, 9 and 11 are cleared on every pass, so there is no full, force-charge or
  no-discharge block. HR15 bit 0 no longer clears the full flag (there is none to clear).
- HR19 bit 2 no longer forces charge and HR19 bit 5 no longer forces discharge or limits the
  charge bound.
- Forced charge keeps going at 1000 W past the charge target. Forced discharge has a -1000 W
  branch under 4 % SoC that cannot be reached, because the parser holds SoC above HR110.
- The ARM page handlers raise the HR98 copy by 5 % and lower the HR97 copy by 14 %, and force
  `enable_charge_target` to 0.

My earlier Ghidra notes (unpublished) ran these paths and agree with the code.

## HR332 (aging_test_enable)

The DSP gets HR332 in ARM page 2 byte 0x27; setting it posts a stop to task 0.
`aging_test_update` waits 101 passes (2 s), then picks aging mode 1 if both PV inputs read 160 V
or more, else mode 2.

- Mode 1: forced charge, the PV references fixed at 250.0 V, the ISR battery maximum 50.0 V (a
  limit the current controller uses only without a BMS), the soft start without the permit, the DC-DC duty raised slowly (converter_ctrl.c), and a battery
  fault does not stop the converter.
- Mode 2: forced discharge, ISR battery limits 46.0..54.0 V (again only without a BMS).
- While HR332 is set: the 63.0 V check in the interrupt code is off, the HR19 bit 2 force is off.

The ARM documentation calls HR332 "lead acid". On the DSP it is a test or burn-in mode. With a
lithium battery and a BMS, setting it would force a charge or a discharge and drop the 63.0 V
check (inference from the code).

## What is dead or unused

- 0xD500 bits 0, 3, 5, 8, 12, all of 0xD502 and 0xD503 have no writer. So CF15 bit 8, CF19 bit
  10, the taper bypass, the 0xD502 term of `has_batt_fault` and the full condition "0xD502 bit 0"
  never act. This looks like code for another BMS protocol.
- Written and never read: `bms_hr17`, `bms_hr18`, `bms_alarm_masked`, `bms_hr15_copy` (0xC012),
  0xD50B, 0xD520 (`charge_voltage_copy`), 0xD52C.
- The battery temperature counter `batt_temp_count` (0xD157, dc_measure.c `temp_check`) counts
  the ARM's battery temperature 0xD230 above `batt_temp_limit` (100.0 degrees), and nothing
  reads it. BMS HR24 only goes to the ARM. So the DSP has no battery
  temperature limit.
- `soc_power_limits` computes a discharge taper b = 15 - SoC (0..5) below 15 % and throws it
  away.
- The `SCIHBAUD != 2` test in task 6 is always true.
- The calibration branch "-1000 W under 4 %" of forced discharge cannot be reached.

## What this means for a third-party BMS bridge

These are facts from the code, for a device that answers the DSP's Modbus polls in place of a
GivEnergy battery. Points marked "inference" go beyond what the code shows.

What the DSP needs:

- Answer FC3 as slave 1 with the exact length (2 x count + 5 bytes) and a good CRC. Answer both
  forms: 28 registers from HR0 when HR109 = 1, and 9 registers from HR17 otherwise. A reply has to
  arrive within 240 ms of its request; one good reply in 30 s keeps the comms fault away.
- HR19 bits 0 and 1 not both 0. Without that the start permit is never given, so the battery
  converter never starts; the full, floor and SoC-taper logic and the mismatch check also need it.
- HR22 (0.01 V) within 5.0 V of the voltage the inverter measures whenever |HR23| is under 2.00 A,
  and not 0. A wider gap stops the battery after 5 checks (100 ms) and needs 10 s within 3.0 V to
  clear. HR22 above HR97 - 2.0 V for 10 s gives the start permit.
- HR19 bit 2 set. Clear, outside a calibration, it forces at least 300 W of charge.
- HR19 bit 4 clear. Set, it is a battery fault (CF15 bit 15) and the converter stops.
- HR13 above 3010 with the full poll, so that HR26 and HR27 are used; or HR13 at 3010 or below
  with HR25 as the one limit. With the short poll and HR13 above 3010 the limits come from the
  CRC, not from the battery. The wire captures in bms-analysis (06-wire-captures.md, including my
  own G3 with BMS firmware 3020) show only the full poll, so HR109 is 1 on those systems; a bridge
  that reports HR13 of 3011 or more depends on HR109 staying 1.
- HR26 and HR27 in 0.01 A. They are capped by the C-rate caps (HR111/HR112 x capacity + 1.5 A,
  HR309, HR309 + 15 A) and by the tapers: the charge limit falls below 48.0 V and the discharge
  limit above 54.5 V (to 10 % at 44.0 V and 58.0 V). A 16-cell pack that rests above 54.5 V gets
  less than its HR27 for discharge (inference from the formula).
- HR11 (Ah) becomes the capacity for the caps after 50 replies. HR11 = 0 would drop the caps to
  1.5 A (and the clamp floors 1 A and 2 A apply).
- HR21 low byte as SoC. It drives the 90..98 % charge taper, the floors and the full flag (100 %).
- To stop charging at the top: HR26 = 0 sets the full block after 30 s, with 1 A allowed until
  then; HR20 bit 2 cuts the charge power limit to 0 at once and also sets the full block after
  30 s. The DSP has no battery voltage regulation in BMS mode; its only voltage protection is the
  CF15 bit 6 trip at HR98 + 1.0 V / + 2.0 V and the 63.0 V check.
- To stop discharging at the bottom: SoC at or below the floor blocks discharge after 5 s; HR27 =
  0 still leaves 2 A; HR20 bit 3 leaves 10 % of the rating.

What the DSP ignores or only passes on:

- HR17, HR18 and every HR20 bit except 2 and 3 (the low byte goes to the ARM). HR24 temperature
  (to the ARM only). HR0, HR5..HR9 and HR13 go to the ARM as identity.
- HR25 when HR13 is above 3010; HR26/HR27 when it is not.
- HR15 except bit 0 at 100 % SoC, where it keeps charging allowed (and forced charge past the
  target).
- The replies to its FC6 writes (HR1..HR4): they are thrown away. A bridge does not need to answer
  them for the DSP's sake.
- The reply's slave address beyond "below 0x10".

Things to avoid:

- HR14 = 1 lets the ARM take the bus for a battery firmware relay; the DSP stops polling and
  resets itself after 5 minutes of it.
- HR19 bit 5 forces 120 W of discharge and holds the charge bound near 0.
- The receive side is on from one 40 ms tick after the request has gone: `GPIO57` (the RS485
  driver enable, per sci.c) is dropped only on the tick after the last byte is out. If the board
  wires the receiver enable to the same pin, a reply that starts sooner may be lost in part
  (inference; the board wiring is not in the firmware).
