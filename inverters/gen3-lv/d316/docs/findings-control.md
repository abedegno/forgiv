# Findings: control, measurement, RCMU and grid code

These are the likely bugs, dead code and open questions I found while I commented src/app/control,
src/app/meas, src/app/rcmu, src/app/grid/grid_support.c and src/app/util. I re-read each one in
the matched source before listing it. The candidates from docs/names/*-notes.md are marked
"(notes)".

Confidence:

- **confirmed**: the code does this, by reading.
- **likely**: the code does it, and I think it is unintended, but a reason could exist.
- **possible**: it depends on something I could not check statically (timing, RAM contents,
  the ARM's side).

What a finding does in a running inverter is always inference. I have not run any of it.

## Likely bugs

### 1. The PF(P) curve (Q mode 3) can never act. Confirmed (notes)

`q_pf_curve` (grid_support.c) acts only when `pf_curve_p1 <= 100 && pf_curve_p2 <= 100`. These
are D846..D849, the curve's power points. The only writer of D846..D849 in the image is
`settings_defaults`, which sets 0xFF. The ARM link's page 3 handler (`arm_page3`, arm_link.c)
stores the four PF codes D84A..D84D from offsets 0xA, 0xE, 0x12, 0x16. It decodes the words at
0xC, 0x10 and 0x14 (HR132, HR134, HR136) and drops them, and it has no case for HR130.

Effect (inference): choosing PF(P) (HR122 = 3) gives Q = 0, whatever the curve settings.

### 2. D806 (the Q(U) maximum) is reset to 438 on every frame byte. Confirmed (notes)

`arm_frame_parse` (arm_link.c) runs `qu_q_max = 438;` inside its loop over the frame bytes,
before each page handler call. No page handler writes D806, and `settings_defaults` sets the
same 0x1B6. So the store has no effect on the value; it only pins it. `q_of_u_curve` uses it as
the Q(U) maximum (43.8 %) for every code but 3 and 19.

Effect: none on behaviour in this image. If D806 was meant to be a setting, the ARM cannot set it.

### 3. The battery's 63.0 V fast trip is off while HR332 is set. Confirmed (notes)

`fast_fault_check` (converter_ctrl.c) checks the battery voltage (8-sample mean over 630 for
more than 40 samples) only when `dsp_settings.aging_test_enable == 0`. Otherwise it zeroes the
count. `aging_test_enable` is 0xD8EF, HR332, which the ARM's documentation calls "lead acid".
In aging_test_mode 1 (HR332 set and both PV inputs at 160 V or more), the same function also
skips the converter stop on a battery fault (`has_batt_fault`).

Effect (inference): with HR332 = 1 the DSP has no fast battery over-voltage trip. Task 2's
CF15 bit 6 still applies (HR98 + 2.0 V for 2 checks of 20 ms; 56.0 V + 2.0 V without a BMS),
but in aging mode 1 that fault no longer stops the converter from this function. Without a BMS,
the current controller still caps charging at 50.0 V (mode 1) or 54.0 V (mode 2). Whether HR332
is a lead-acid mode or a factory burn-in is open.

### 4. Two ramp rates are used the opposite way in two places. Possible

`run_status_bits` bit 2 is set by task 3 on any grid voltage or frequency fault, and nothing
clears it.

- `soft_start_ramp` uses HR302 (`power_ramp_rate_reconnect`) when the bit is set and HR306
  otherwise.
- `active_power_rate_limit` (European group, op_state 6) uses HR306 when the bit is set and
  HR302 otherwise.

One of the two is probably swapped. Effect: none with the defaults (both 1000). With different
settings, the HR50 slew limit after a grid fault uses the normal rate and the other way round.

### 5. ofr_power_limit writes the HR303 setting. Likely

`ofr_power_limit` (op_state 5, OF enabled) assigns `ofr_recover_delay`, which is the setting
0xD83A (HR303) that arm_page_write.c stores from page 6. It writes 1500 for codes 3/19, 0 for
8/12/16/17 and 15150 for the rest except the European group and 7/18. The ARM rewrites the
word with each page 6 frame. So the value alternates between the code's constant and HR303.
`ofr_storage_response` (op_state 6) reads the same word as its recovery delay.

Effect (inference): small. In op_state 5 the constant wins, apart from up to 20 ms after each
page 6 frame. After a spell in op_state 5, op_state 6 can see the constant until the next page 6
frame. A local variable was probably meant.

### 6. RoCoF flags a trip on its first window after start-up. Likely

`rocof_detect` keeps the last frequency in `rocof_prev`, which starts at 0.0 and is cleared to 0
only while the detector is disabled. For the European group it is enabled on every run.
`grid_freq_protect` first calls it when the frequency becomes valid, so the first difference
is 0 - 5000 and the first window's sum is far over 110.

Effect (inference): CF12 bit 15 for 5 s after the first valid grid measurement for these codes.
Task 3 then also sets run_status bit 2, so the "after a grid fault" ramp rates and the restart
delay apply from the first start. The inverter is not on the grid then, so the trip itself does
little.

### 7. Trip counters that are never reset below the threshold. Likely

These counts add up samples over the limit since the last trip, not consecutive samples:

- `batt_ov_count` (63.0 V, more than 40 samples) and `hv_ov_count` (560.0 V, 5 samples) in
  `fast_fault_check`;
- `vt_inst_ov_cnt` (300.0 V, 2 ticks) in `grid_volt_protect`;
- `eps_short_count` (5 ticks) in `eps_output_check`.

B's notes found the same in task 2's battery voltage monitor (my earlier Ghidra notes).

Effect (inference): rare spikes add up over hours, so a trip can come from noise that never
lasted. For the 63.0 V check, 41 samples is about 2 ms in total.

### 8. The current controller's battery voltage PIs take their limits once, at init. Possible

`dcdc_ctrl_init` sets `pi_v_max` to +-`dcdc_ref.i_cmd_max` and `pi_v_min` to
-`i_cmd_max`..0. It reads `i_cmd_max` (0xC20E), which task 2 writes as 15.0 A on every tick.
Task 0 runs first after `os_start` and its idle state calls `dcdc_ctrl_init` at once. Nothing I
found clears 0xC200..0xC23F before that, and the word has no initial value in .cinit. So at a
cold start the limits come from whatever the RAM held. They are set again only when the state
machine next enters idle, fault or update.

Effect (inference): these PIs act only without a BMS (HR54 = 0). If the RAM held 0, the charge
and discharge current would be clamped to 0 in that mode until the first stop. This needs a
check on hardware or in an emulator of the start-up order.

### 9. The RCMU step gate compares the RMS with the mean's base. Possible

In `rcmu_watch`, the sudden-change differences are computed only while
`fabs(m[1] - p->base1) > 0.0061f || fabs(m[0] - p->base1) > 0.0061f`. The second term pairs the
RMS (m[0]) with base1, the mean's open-circuit value. Everywhere else m[0] goes with base0
(`rcmu_watch`'s clear test).

Effect (inference): base1 is a DC mean near 0, and the RMS includes the sensor's noise and any
AC leakage. So the gate is open whenever the RMS is over about 6 mA. The step checks then run
on more readings than intended, which can only make them trip sooner, never later.

### 10. HR301 = 0 turns cos phi(P)'s Q to 0 for code 7. Possible

`q_pt1_response` returns 0 when `q_pt1_tau` (HR301) is 0. The fixed-Q counterpart,
`q_restore_response`, passes the input through in the same case.

Effect (inference): for code 7 with HR301 = 0, cos phi(P) gives no reactive power. The default
is 150, so this only matters if someone sets 0.

### 11. Smaller oddities. Confirmed code, effect small or none

- The grid code tests are duplicated in places (notes): `grid_v_10min_update` lists 11 twice,
  and `q_of_u_lockin` lists 6 twice. Harmless.
- One test is always true (notes): in `grid_volt_protect`, Italy's stage 1 under-voltage branch
  runs `if (autotest_step == 1 || ... || autotest_step == 6)` inside `if (autotest_step == 6)`.
  Harmless.
- `grid_volt_trip_std` uses `>=` for the over-voltage 2 time and `>` for the other three stages:
  one tick (20 ms) of difference.
- `dc_component_calc` divides the grid-cycle DC sum by task 3's sample count
  (`dc_acc.a.inv_n`) instead of the grid sums' count. The two are within a sample of each other
  on a 50/60 Hz grid.
- `pv_power_limit_flags` sets MPPT flag bits 0 and 1 from the same total PV power, so the two
  differ only in their counters.
- `ofr_power_limit` (Italy) only range-checks `ofr_it_param` (0xD854, default 24). The droop is
  a fixed 0.77 % of the base per 0.01 Hz. A value of 24 may have meant a 2.4 % droop.
- `rcmu_init` sets the watch's continuous limit to 0.86 A, and `rcmu_watch_step` overwrites it
  with 0.3 A on every run. So the 0.9 factor of the other levels does not apply to this one
  (300 mA, not 270 mA).

## Checked and not a bug

### q_restore_response (sub_3EAA22) does not decay Q to 0 (notes)

docs/names/A-notes.md reads `q_restore_response` as "always decays to 0" because it sets
`q_rec_target = 0` before the PT1 formula. The formula is
`x = q_rec_start + (q_rec_target - q_rec_start) x exp(-k t / tau)`. With the target 0 that is
`q_rec_start x (1 - exp(-k t / tau))`, which rises from 0 to the value captured when the
inverter came back. It is a restore ramp after a low-voltage momentary cessation (Q is 0 during
the cessation and for 1 s after it). After `tau x 4 + 10` the input passes through again.

### pv_bus_limit's unset offset is latent only (notes; confirmed latent)

`pv_bus_limit` sets `off` to 100 (op_state 7) or 200 (op_state 5 or MPPT flag bit 14), with no
else. `control_isr` calls it only under that same condition. No task can change op_state or the
flag inside the interrupt, and the other interrupts do not write them. So the missing case is
never reached in this image. It would become live if the call condition changed.

## Dead code and dead data

- `pi_reset`, `pi_reset_b`, `pid_inc_step`, `pid_pos_step`: never called.
- util.c: `checksum_xor`, `clampu`, `cal_gain_or_unity`, `labs32`, `limit_sym`, `isqrt_div`,
  the four `debounce_*` functions and `hexdigit` are never called.
- `iq_consts_init` writes constants (0xCCB2..0xCCBA) that nothing reads.
- `ntc_temp_check` compares the constant 2437 with 3350, so CF17 bit 2 is never set, and
  `ntc_to_temp` is never called (notes B).
- `temp_check` counts the battery temperature against 100.0, but nothing reads the count
  (notes B).
- The EPS DC offset trim: `eps_dc_clear` zeroes `ac.eps_dc` every EPS cycle, so
  `eps_dc_offset_trim`'s filter input is always 0. The value it hands the interrupt
  (`eps_dc_trim_isr`, 0xCC74, the EPS controller's `dc_trim`) is the constant 0.0.
- `clear_d3e6` and `clear_d3e4` zero words nothing reads.
- `rcmu_offset` (0xD412): the RCMU's zero offset is computed and stored, and nothing reads it.
  Only the `done` flag matters (it arms the self test).
- `wake_bus_pi` in `converter_mode`'s low-battery wake path is stepped every sample, and its
  output is never used (notes C).
- `pv_bus_mode` (0xD805) is never set to non-zero. So `grid_ictl` mode 1 (amplitude ramp), the
  PV boosts' bus PI and `bus_voltage_ref_update`'s grid-tracking reference are unreachable.
- `meas_fast.aux17` and `aux18` copy ADC channels nothing fills, so they are always 0 (notes C).
- `dc_acc.b` is only cleared and counted by `meas_init`.

## Open questions

- Whether `dcdc_ctrl_init` runs before task 2 writes 15.0 A into 0xC20E at a cold start
  (finding 8).
- The meaning of HR332: "lead acid" in the ARM's documentation, a burn-in mode in the DSP's use
  (forced charge or discharge, a fixed 250 V PV reference).
- The units of `hv_i` (0.01 A, inferred), of the DC injection channel and `dci_limit`, and of the
  insulation resistance threshold (50).
- The grid code to country mapping beyond 4 (3 and 19 probably Australia and New Zealand).
- The unit of HR301 (seconds if `q_pt1_time` counts seconds).
- What state_flags bit 15 is for: the bridge then follows the grid voltage with no current loop.
