# Findings: power, battery, state and settings code

Scope: src/app/power (power_limits.c, mppt.c, mppt_step.c), src/app/battery/bms_link.c,
src/app/state (app_init.c, op_states.c, state_task.c), src/app/settings (settings.c,
rating_presets.c). I re-read each item in the source before listing it. "Confirmed" means the
code does this without doubt; the effect in the inverter is still inference unless I say it was
run. "Likely" means the code is clear and the effect very probably happens. "Possible" means it
depends on something outside the firmware or on timing I could not pin down.

Units and terms are as in docs/battery.md.

## Likely bugs

### 1. The battery soft-start timeout latches until the DSP restarts (likely)

`soft_start` (op_states.c) sets CF15 bit 5 (`FB_SOFT_START_TIMEOUT`) after 8001 calls (160 s)
without finishing. No code clears that bit: the only clear of CF15 as a whole is the `memset` in
`state_init`, which task 0 runs once at start-up, and no code clears bit 5 by itself (I searched
for every write to `fault_batt`, `fault_words[3]` and `w[3]`). Any CF15 bit is a battery fault,
so `soft_start_permit` withdraws `soft_start_enable`, `soft_start` stops being called, and the
interrupt code holds the converter mode at 0.

Effect (inference): one soft start that fails to finish (for example the HV side never comes
within 10.0 V of 5.5 x the battery voltage, so the completion check is skipped) keeps the battery
converter off until something resets the DSP. The DSP resets itself only through GPIO27 for the
reasons in `status_led_and_reset`; whether the ARM resets it on this fault is not in this image.

### 2. The short BMS poll reads HR26/HR27 from outside the reply (confirmed code; effect possible)

`bms_parse_hr` reads `RX16(base + 21)` and `RX16(base + 23)` when `bms_fw_version` is above 3010.
With the short poll (`bms_full_poll` not 1, base 0) the reply is 23 bytes: bytes 21 and 22 are the
CRC, and bytes 23 and 24 are left over from an earlier, longer reply (or 0). So the charge limit
is the CRC with its bytes swapped and the discharge limit is stale. Agent C's harness saw
368.86 A and 655.35 A (docs/names/C-notes.md).

`bms_fw_version` is written only by the full poll, and I found no code that sets it at start-up,
so this needs a full poll first (or whatever RAM holds at reset) and then HR109 changed to a value
other than 1. Effect (inference): the BMS's own limits are not used; the current limits are then
set by the C-rate caps and the 80 A clamp only.

### 3. HR4 requests have no length and stop the poll (confirmed code; effect possible)

In `task6_bms_link` the `hr201_request == 1` branch fills `bms_tx_buf` but does not set
`bms_tx_len`, unlike every other branch. It reuses the last length, which is always 8 once any
request has gone; if HR4 is the very first request after reset, `bms_tx_len` is 0 and the CRC is
computed over 0xFFFE words and stored at `bms_tx_buf[0xFFFE]` and `[0xFFFF]`, outside the buffer.
The branch also comes before the poll, so while `hr201_request` stays 1 no FC3 poll goes out and
the BMS link times out after 30 s. How long the ARM holds page 5 byte 0x2A at 1 is open.

### 4. Over- and under-voltage counts are totals, not runs (likely)

In `batt_voltage_monitor` the counters for CF15 bit 6 (`ov_fast_count`, `ov_slow_count`,
`ov_clear_count`) and bit 2 (`uv_set_count`, `uv_clear_count`) are reset only when they fire.
So "over HR98 + 1.0 V for 50 checks" is 50 such checks since the last trip, however far apart,
and the clear count of bit 6 also builds up while the bit is clear (my earlier Ghidra notes ran this).
The force-charge and no-discharge counts in `soc_floor` behave the same way (`force_chg_count`,
`no_dis_count`). Effect: occasional short peaks add up to a trip; a trip can clear after much less
than 10 s below the limit.

### 5. board_id reads GPIO33 twice (likely)

`state0_idle`: `GPIO32 + (GPIO33 << 2) + (GPIO33 << 1)`. One of the two was probably GPIO34. The
code is kept as it is for matching (a `match:` note is on the function). Nothing in these files
reads `board_id`.

### 6. The ARM's forwarded FC3 request is parsed as a poll (possible)

The receive side computes the expected FC3 length from `bms_poll_count`, the count of the DSP's
own poll, and treats every good FC3 reply as a poll reply (`bms_parse_hr`). A forwarded FC3
request (`arm_bms_request`, sent as it is) with another start or count either fails the length
and CRC test or, if its length happens to match, is parsed as HR0.. or HR17.. data. The reply's
address is also not compared with 1 (any byte under 0x10 passes). Whether the ARM ever forwards
FC3 is open; agent C's notes and the code's names suggest FC4.

### 7. state0_idle restarts itself every pass in modes 7 and 8 (possible)

In `state0_idle`, any run mode other than 0, 4, 5 and 6 reaches a `return` on every tick, even
when it does not move to state 1. Task 0 then calls `state0_idle` again, which reruns the
converter stop and the four control inits each pass (20 ms) while it waits (up to 30 s with
HR175). Effect (inference): harmless while idle, but the wait is not a plain wait.

### 8. The charger temperature derate can go negative (possible, latent)

`current_limit_tapers` holds the derate step t to 0..100 degrees and computes rating - t x 5 % x
rating, which is negative above 20 degrees over 87.0 (107.0 degrees). The charge bound would then
become -0.5 A (a small discharge). The charger over-temperature stop (CF17 bit 6 at 93.0 degrees,
dc_measure.c) comes first, so this is not reached in normal running.

### 9. The PV reference reaches the interrupt code before its clamp (possible)

`mppt_pv1`/`mppt_pv2` copy `mppt.pv1_ref` to `pv1_ref_isr` and only then clamp it to 120 V ..
`pv1_ref_max`. For one call the interrupt code can see a reference outside that range (for example
the resync value "measured - 2.5 V" with a low PV voltage). The next call clamps it.

### 10. HR15 bit 0 is tested against the previous SoC (possible)

`bms_parse_hr` keeps HR15 bit 0 only while `bms.soc` is 100 or more, but it tests `bms.soc` before
it stores the new SoC from the same reply. The bit is one reply (240 ms) late on both edges.

## Behaviour worth knowing (not bugs as far as I can tell)

- In BMS mode the current controller has no battery voltage loop: `ctl_voltage_limits_set`'s
  56.0 V/46.0 V limits are used only when `battery_type` is 0 (current_ctrl.c). Top-of-charge
  control is HR26, HR20 bit 2, the SoC taper and the CF15 bit 6 trip.
- The low-voltage taper (below 48.0 V) is on the charge limit (HR26) and the high-voltage taper
  (above 54.5 V) on the discharge limit (HR27). Positive is charge (from `batt_power_blocks`), and
  `current_limit_tapers` puts the HR26 path into the positive bound. Why the tapers sit this way
  round is open: the first could be a gentle charge for a deeply discharged pack; the second has
  no obvious purpose.
- During a calibration the parser holds SoC above HR110, so forced discharge never stops at the
  floor on the DSP side, and the BMS limits are held at 8 A or more. The stop comes from the ARM
  (HR20 bit 3 ends the stage, my earlier Ghidra notes), the BMS, or the under-voltage bit at HR97 x
  0.86.
- The fall of HR175 sets CF15 bit 3, a battery fault for 10 s, and posts a stop to task 0.
- While HR175 is set, each of the first 50 BMS replies after a loss posts a stop to task 0 (about
  12 s of stops).
- A battery fault does not stop the converter during the low-battery wake or in aging mode 1
  (`converter_mode`, converter_ctrl.c).
- Schedule window 3 (ARM page 7) sets CF15 bit 15, so it stops the battery converter.
- `soc_floor` writes the raised floor (HR110) into `soc_reserve`, the HR114 copy, which stays
  raised until the next ARM page 0 frame rewrites it.

## Dead code and unused data

- 0xD500 bits 0, 3, 5, 8, 12, the word 0xD502 and the word 0xD503 have no writer anywhere in the
  image. So CF15 bit 8 (`FB_BMS_ALARM`), CF19 bit 10, the taper bypass in `current_limit_tapers`
  (0xD502 bits 1, 3, 6, 7), the "0xD502 bit 0" term of the full condition and the 0xD502 bit 11
  term of `has_batt_fault` never act.
- Written, never read: `bms_hr17`, `bms_hr18`, `bms_alarm_masked`, `bms_hr15_copy` (0xC012),
  `bms.cfg_d50b` (0xD50B), `bms.charge_voltage_copy` (0xD520), `bms.max_charge_current_copy`
  (0xD52C).
- `soc_power_limits` computes b = 15 - SoC (0..5) and never uses it: a discharge taper below 15 %
  SoC that was dropped.
- The battery temperature counter `batt_temp_count` (0xD157, dc_measure.c) is never read, so the
  DSP has no battery temperature limit. It also decrements past 0 to 0xFFFF when the temperature is
  low and the count is 0 (harmless, since nothing reads it). BMS HR24 is only sent to the ARM.
- `task6_bms_link`'s `ScibRegs.SCIHBAUD != 2` test is always true (only device_init.c writes
  SCIHBAUD, with 1).
- The calibration branch "-1000 W under 4 % SoC" in forced discharge cannot be reached.
- Not called: `has_fault_cf12_low`, `run_mode_req_get`. `preset_none` fills five table slots;
  `rating_apply` limits the index to 1..6, so slots 0, 7 and 8 are never used.
- Unused locals in the matched code: `x` in `soc_floor`, `full_current_trim` and
  `current_limit_tapers`; `f` in `soc_floor`; `t` in `batt_voltage_monitor`.

## Open questions

- Does the board wire the RS485 receiver enable to GPIO57? `task6_bms_link` drops GPIO57 only on
  the 40 ms tick after `sci.c` reports the last byte gone, so if it does, the start of a reply that
  comes within about 30 ms of the request is lost.
- What do BMS HR19 bits 2, 4 and 5 mean on a GivEnergy battery? The DSP uses bit 2 clear as
  "force at least 300 W charge", bit 4 as a battery fault and bit 5 as "force at least 120 W
  discharge, hold the charge bound near 0".
- What are BMS HR1..HR4 on the battery side (the wake write and the three ARM-driven writes)?
- How long does the ARM keep `hr201_request` at 1 (item 3)?
- The units of `dc_meas.pv_p` are not pinned down here, so the MPPT low-power threshold (a mean
  under 60000) is given in raw units.
