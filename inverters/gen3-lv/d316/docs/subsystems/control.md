# Control: the control interrupt and its loops

Files: `src/app/control/` (converter_ctrl.c, ac_ctrl.c, current_ctrl.c, pll.c, ctrl_glue.c,
island_detect.c, pi_ctrl.c, pid_inc.c, pid_pos.c, eps_vctl_init.c, eps_vctl_clear.c,
grid_freq_setup.c) and `src/include/control.h`. The measurement steps the interrupt runs are in
[meas.md](meas.md), the residual current checks in [rcmu.md](rcmu.md), and the task 4 grid code
that sets the current bounds and the reactive reference in [grid.md](grid.md).

Everything here is read from the matched C. Where I say what the hardware does, that is
inference from the code, and I say so.

## Timing

- The control interrupt `control_isr` is EPWM4_INT (PIE group 3). The PWM period is
  `pwm_period` = 2343 counts in up-down mode, so 90 MHz / (2 x 2343) = 19.2 kHz. The 50 Hz cycle
  is 384 samples and the 60 Hz cycle 320 (`grid_freq_setup`); both are 19.2 kHz / f.
- `ctrl_slot` (0xC83A) counts 0..7. `meas_avg8` steps it every sample and takes the
  eight-sample means when it wraps. The slow loops run on fixed slots, so each runs at 2.4 kHz:

| Slot | What runs |
|---|---|
| 0 | grid current controller's bus voltage PI (`grid_ictl`), PV1's MPPT voltage PI (`pv1_ctrl`) |
| 1 | PV2's MPPT voltage PI (`pv2_ctrl`) |
| 2 | PV boost precharge of the bus (start state only, in `control_isr`) |
| 4 | the battery converter's outer loops (`current_controller`) |
| 6 | the PV bus limiter (`pv_bus_limit`) |
| 3, 5, 7 | nothing extra |

- The other interrupts: `tz_trip_isr` (EPWM3 trip zone, group 2) and `grid_period_isr` (ECAP1
  capture of the grid period, group 4; it hands the cycle's RMS sums to task 1).

## The control interrupt, in order

1. Wait for ADCINT1, then `adc_scale`, `meas_avg8`, `grid_pll_run`, `island_detect_run`.
2. A start request (state_flags bit 12 for EPS, bit 13 for grid) sets `inv_on` and clears the
   trips (`epwm_trip_clear`).
3. `iso_dcdc_pwm` (the isolated stage), `fast_fault_check`, `converter_mode` (the battery
   buck/boost), `grid_angle_update`.
4. The bridge (EPWM3/EPWM4), while `inv_on` is 1 or state_flags bit 15 is set:
   - op_state 5 or 6: `grid_ictl_run`, and the EPS controller is kept clear;
   - bit 15 alone: the bridge voltage is the grid voltage (a pre-sync before the relay closes;
     inference);
   - op_state 7 or 8: `eps_vctl_run`, and the grid controller is kept clear;
   - otherwise EPWM3/EPWM4 are forced off and both controllers cleared.
   The duty is the controller's bridge voltage times `mod_gain` (pwm_period / bus voltage, from
   task 3), and `inv_pwm_update` loads it as a unipolar pair of compares.
5. `grid_rms_accum`, `relay_drive`, `q_loop_run`.
6. The PV boosts (EPWM1A, EPWM1B). The word at 0xCF1A (`power_limits`, the precharge step) is
   1 or 2 while op_states.c precharges the bus. When the bus is within 10.0 V of its reference
   it becomes 3 in op_states 5..8. In the start state with the word at 2 the boosts precharge
   the bus on slot 2. With the word at 3 and PV in use (op_state 5 or 7, or MPPT flag bit 14)
   `pv_bus_limit`, `pv1_ctrl` and `pv2_ctrl` run. The compares they leave pending are loaded
   at the end.
7. `dc_acc_step` and `eps_rms_accum` hand per-cycle sums to tasks 3 and 1.

## Grid current controller (grid_ictl, block 0xC400)

This is the controller the readability pass renamed: the old `VCTL` block is `grid_ictl_t`.
It runs in op_states 5 (grid with PV) and 6 (grid, battery only).

- Outer loop, on slot 0: a PI on `bus_v_ref - bus_v` (error clamped to +-20.0 V, kp 0.6,
  ki 0.008) gives the active current amplitude `amp_cmd` (negated). It is limited to
  `amp_min..amp_max`. Those are the peak current bounds task 4 writes every 20 ms (`i_ref_max`,
  `i_ref_min`). While the bus is above 520.0 V (`bus_high`, reset at 480.0 V) they are cut to
  +-23.5 A. So the grid side regulates the DC bus, and task 4's power limits act as a current
  clamp on it.
- Mode 1 (an amplitude ramp of 3 per cycle) needs `pv_bus_mode` (0xD805), which nothing sets to
  a non-zero value, so it never runs.
- References: active `amp x sin`, reactive `(q_cmd + island_inj + q_corr) x cos`, with sin and
  cos from the PLL. `q_cmd` is task 4's reactive current (0xC408), `island_inj` the islanding
  detector's output and `q_corr` the reactive loop's output.
- Inner loop, every sample: `v_cmd = 4.5 x i_err + repetitive filter + v_ff + v_ff2`. `v_ff`
  is the grid voltage plus `adc_scale`'s lead term (a filtered second difference, +-10.0 V).
  `v_ff2` (0xC42A) is task 4's DC injection correction. The repetitive filter reads the delay
  line at the PLL's sample index n and writes 5 samples ahead, over one grid period (a 0.8,
  b 0.923, gain 1.0, +-700).
- Units: currents in 0.1 A (peak for the amplitudes), voltages in 0.1 V.

`grid_ctrl_init` sets the bus PI limits to +-1.1 x sqrt 2 x `grid_current_max` (the rated RMS
current), and task 4 then narrows them.

## EPS voltage controller (eps_vctl, block 0xC840)

The old `ICTL` block is `eps_vctl_t`. It runs in op_states 7 and 8 with EPS enabled (0xD7C0
bit 9).

- The amplitude `v_peak` rises 0.25 V per sample to `sqrt 2 x eps_v_ref`. `eps_v_ref` is task
  1's RMS reference, which itself ramps 10.0 V per cycle from 50.0 V to the nominal voltage.
  Once there, task 1's slow RMS loop adds `amp_trim` (0xCC76, 0..50.0 V).
- Voltage loop: `pi_v` on `v_ref - v_meas` plus the repetitive filter on the same error (once
  the ramp is done) plus a filtered inverter current (a load feed-forward) gives the current
  reference. It is limited to +-37.0 A. `eps_vctl_init` sets ki to 0, so this "PI" is
  proportional only (kp 0.4).
- Current loop: `pi_i` on `i_ref - i_meas` (kp 1.9, ki 0) plus the measured inverter voltage is
  the duty.
- `dc_trim` (0xCC74) is always 0 (see findings).
- The angle comes from `grid_angle_update`. In the grid states it follows the PLL, so a
  change to EPS starts in phase. Otherwise it free-runs at `grid_omega_step`.

## PLL (pll_step, block 0xC100)

A single-phase PLL, once per sample:

- Phase detector: `in x cos(theta)`, with `in` the grid voltage in V. Two first-order low-pass
  stages follow (gain 0.0098 each).
- An incremental PI (`pi_inc_step`, kp 7e-5, ki 5e-8) gives the angle step `w` in rad per
  sample, limited to 0.00818..0.02618 (25 Hz to 80 Hz at 19.2 kHz).
- At each wrap of theta, `period` takes the cycle's sample count. The summed phase error decides
  the lock: `|sum| < 20 x period` is a good cycle. 20 good cycles set `locked`, 10 bad cycles
  clear it. Task 3 needs `pll_locked` before it restarts the grid inverter after a rescan.
- The outputs are sin and cos of theta + 2.8 degrees. I read the lead as compensation for the
  sampling and filter delay; that is inference.

## Islanding detector and reactive loop

- `island_detect` runs once per grid cycle (PLL sample 100) in op_states 5/6 with 0xD7C2
  bit 11. Its input is 1000 x the PLL angle step; one unit is about 3.06 Hz. `d` is minus the
  drift summed over the last three cycles. `50 x d`, clamped to +-1.5 A, goes into the reactive
  current reference. This is a positive feedback that pushes the frequency away once the grid is
  gone. 60 cycles with `|d| > 0.97` (it counts down below 0.485) stop the bridge (`inv_off`).
- `q_loop_step` sums the reactive reference and the measured current x cos over each cycle, and
  steps a PI once a cycle at PLL sample 20 (kp 0.00061, ki 0.00049, +-5.1 A). Its output is
  `q_corr`.

## Battery path: two stages

The battery reaches the DC link through two converters (docs/names/C-notes.md, finding 4):

1. The isolated stage on EPWM7/EPWM8 (`iso_dcdc_pwm`). Its phase-shifted compares come from
   `dcdc_ref.soft_duty`, which op_states.c's `soft_start` ramps to 1054. It runs while
   `soft_start_phase` is 4 or 50. During a battery calibration request (HR175) EPWM8 is forced
   low and only EPWM7 runs. Off, both are forced low at a fixed point of EPWM8's count (1055..1153)
   and the duty resets to 1. B's notes say the high side tracks about 5.5 x the battery voltage,
   so the turns ratio is probably 5.5.
2. EPWM5, a buck/boost between that high side (`hv_v`, ADC c6) and the DC link (`bus_v`,
   c5). The controller regulates the high-side current `hv_i` (c9), not the battery current.

### Converter modes (converter_mode)

| conv_mode | Controller mode | What the battery side does |
|---|---|---|
| 5, 6 (grid) | 1 | follows task 2's current command `i_cmd`, slewed 0.1 A per slot and limited to +-15.0 A; the bus is held by the grid controller, with two band PIs only outside bus_v_ref +-10.0 V |
| 7, 8 (EPS) | 2 | holds the DC bus: a PI on the bus error (+-50.0 V); with a BMS and the battery full (CF19 bit 8) it may not charge |
| other | 0 | EPWM5 forced off, controller cleared |

`low_batt_wake` (0xD90A) takes another path once the isolated stage is at full duty and 0xCF1A
is 2. `wake_batt_pi` then drives EPWM5 (1..900 counts) to bring the battery terminal to a fixed
56.0 V. Its integrator is cleared above 59.0 V, with the high side over 450.0 V, or with the
battery over 30.0 V and the bus under 320.0 V. `wake_bus_pi` is computed but never used.

`aging_test_mode` 1 (from HR332, both PV inputs at 160 V or more) soft-starts EPWM5: the duty
ceiling rises one count per 50 samples to 2300, or jumps there once the battery is over 50.0 V.

### The current controller (current_controller, block 0xC240)

On slot 4 it runs the start sync (until the bus is within 8.0 V of its reference), the mode
logic above, then the limits in this order:

1. +-`i_cmd_max` (15.0 A), the band PIs, then +-`i_limit` (30.0 A);
2. not in BMS mode only: the battery voltage limits (next section);
3. the charge and discharge bounds from task 2 (0xC212/0xC214), with `at_chg_bound` flagging
   the charge bound (1.0 A hysteresis);
4. minus the full-battery trim `i_offset` (0xC206).

Every sample, a current PI on `i_ref + hv_i` (kp 0.3, ki 0.04, +-2343) plus the feed-forward
`duty_ff = pwm_period x hv_v / bus_v` gives the EPWM5 duty, clamped to 500..2300 counts.

Units: currents in 0.1 A on the high side. `meas_fast.hv_i` is in 0.01 A, and converter_mode
scales it by 0.1. I read the 0.01 A from the limits applied to it (a 20.00 A fast trip, 15.0 A
command limit); it is not stated anywhere. Positive current charges the battery (B's sign for the
power command).

## The 0xC208/0xC20A battery voltage limits: is 56.0 V a fixed ceiling?

Open item from docs/names/B-notes.md. Task 2's `ctl_voltage_limits_set` writes
`isr_limits.batt_v_max` (0xC208) and `batt_v_min` (0xC20A) every 20 ms:

| aging_test_mode | batt_v_max | batt_v_min |
|---|---|---|
| 0 (normal) | 560 (56.0 V) | 460 (46.0 V) |
| 1 | 500 (50.0 V) | 460 |
| 2 | 540 (54.0 V) | 460 |

In `dcdc_ref_t` these words are `batt_v_max` and `batt_v_min`, at offsets 8 and 0xA from 0xC200.
I confirmed the layout from the disassembly of `converter_mode`, which loads @0x4, @0xE and @0x12
of page 0x308 into the controller's `i_limit`, `i_cmd_max` and `chg_bound`. `converter_mode`
copies them into the controller block every sample. `current_controller` uses them only inside
`if (s->bms_mode == 0)`, where `bms_mode` is `dsp_settings.battery_type` (0xD8E1, HR54):

- while the current charges, `pi_v_max` on `batt_v_max - batt_v` (kp 1.25, ki 0.007) caps it.
  Below 56.0 V its output saturates at +15.0 A and has no effect; above 56.0 V it winds down and
  cuts the charge current;
- otherwise `pi_v_min` on `batt_v_min - batt_v` floors the discharge current to 0 below 46.0 V.

So the answer is: **yes, it is a fixed 56.0 V charge ceiling (and a 46.0 V discharge floor),
but only when HR54 = 0 (no BMS)**. In BMS mode (HR54 != 0) the current controller has no battery
voltage loop at all. Whatever HR98 says, nothing in the converter regulates the battery voltage
in that mode. What does act on the battery voltage in BMS mode:

- task 2's over-voltage fault, CF15 bit 6: above HR98 + 2.0 V for 2 checks, or HR98 + 1.0 V
  for 50 checks (20 ms each). It clears after 500 checks below HR98 (power_limits.c,
  `batt_voltage_monitor`; with no BMS the maximum there is the same fixed 56.0 V);
- the fast 63.0 V trip in `fast_fault_check` (off while HR332 is set; see findings);
- the BMS current limits and their voltage tapers (task 2, power_limits.c), which reach this
  controller as the charge and discharge bounds.

The low-battery wake path is separate. It regulates the battery terminal to 56.0 V whatever
HR54 says, but only while `low_batt_wake` is set.

For a pack swap I read this as follows (inference): with a BMS the DSP relies on the BMS current
limits and the HR98 fault to stop a charge. Without a BMS, 56.0 V is the hard regulation point
and HR98 plays no part.

## PV boosts

- `pv1_ctrl` and `pv2_ctrl` run every sample while the boost is on and CF12 bit 3 is clear.
  Once a cycle the soft current ceiling rises 0.10 A towards `i_limit`. On their slot the MPPT
  voltage PI acts on `pv_v - pv_v_ref` (a higher PV voltage asks for more current). Its output
  is kept to at most 0.05 A over the soft ceiling and to `0..i_limit`. The current reference is
  capped by `pv_bus_limit`'s output. The current PI's ki drops from 0.012 to 0.002 above 3.00 A
  (back below 2.00 A). With the inputs in parallel (pv_input_mode 4), PV2 takes PV1's
  reference.
- `pv_bus_limit` (slot 6) holds the bus at its reference + 10.0 V (op_state 7) or + 20.0 V
  (op_state 5, or the PV restart flag) by capping both PV currents (kp 2.32, ki 0.08, 0..15 A).
  It also clamps the MPPT voltage references to 120.0..545.0 V.

## Fast faults (fast_fault_check, every sample)

| Check | Level | Count | Action |
|---|---|---|---|
| grid voltage peak (`grid_v_check`) | 396.0 V | > 76 consecutive samples (4 ms) | CF12 bit 12 |
| grid voltage window (with 0xD7C0 bit 10) | quarter-cycle mean of abs(v) outside 91..345 V | after 2 s settling | CF12 bit 12 |
| inverter current | 40.0 A | 1 sample, op_states 5..8 | PV boosts forced low, bridge off, MPPT rescan |
| DC bus (8-sample mean) | bus_ov_limit, 585.0 V | 1 sample | EPWM5 trip, PV stop, bridge off, stop event |
| high-side current | 20.00 A | 2 samples (reset below 10.00 A) | EPWM5 trip, stop event |
| high-side voltage | 560.0 V | 5 samples, never reset below | EPWM5 trip, stop event |
| battery voltage (8-sample mean) | 63.0 V | > 40 samples, never reset below; skipped while HR332 is set | EPWM5 trip, stop event |
| battery fault (has_batt_fault) | CF15 and others | 1 sample, not in wake or aging mode 1 | converter stopped |
| residual current fast mean | 135 mA step | every 10 samples | rcmu_fast_check, CF18 bit 6 |

`fast_trip_code` keeps the last cause: 1 grid peak, 2 inverter current, 3 bus,
4 high-side current, 5 high-side voltage, 6 battery fault.

`tz_trip_isr` (the hardware trip zone) forces every PWM off, stops the converter and the
bridge, sets CF12 bit 4 and counts the trip.

## Relays (relay_drive)

The state machine sets the commands in `relay_ctl` (0xC034..0xC036):

- `relay_a` (GPIO12) closes at one of two sync samples of the PLL cycle. These are samples
  24/216 at 50 Hz and 320/160 at 60 Hz, half a cycle apart. 50 cycles later the command
  becomes 2.
- `relay_b` (GPIO21) and `relay_c` (GPIO53) are on at 1.
- At 2, an output toggles every sample, a 9.6 kHz drive. I read that as a reduced hold current
  for the coil (inference).

## PI blocks

`PI_BLK` is 18 words: e, kp, ki, kd, umax, umin, e1, i, u.

- `pi_step` (positional) is the one every loop uses: `i += ki e` clamped to the output limits,
  `u = kp e + i` clamped. The caller writes e; e1 is not used.
- `pi_inc_step` (incremental: `u += (kp + ki) e - kp e1`) is the PLL's.
- `pi_reset`, `pi_reset_b`, `pid_inc_step` and `pid_pos_step` are never called.
- pi_ctrl.c, pid_inc.c, pid_pos.c and pll.c each declare their own copy of the block, with
  different members volatile. The image needs the different volatile members to match.

## Open questions

- What state_flags bit 15 does to the bridge (it is driven at the grid voltage). I think it
  pre-syncs the inverter side before relay_a closes, but I have not traced it through
  op_states.c's relay test.
- The units of `hv_i`, inferred from its limits only.
- Whether `dcdc_ctrl_init` can run before task 2 has written `hv_current_max`. If it can, the
  battery voltage PIs get limits of 0 (see findings, "possible").
- Which hardware the 9.6 kHz relay toggle drives (a coil economiser, by the look of it).
