# RCMU: the residual current monitor

Files: `src/app/rcmu/` (rcmu.c, rcmu_selftest.c, rcmu_watch.c, rcmu_fast_check.c) and
`src/include/rcmu.h`. Before the naming pass this code was read as an "offset calibration" and a
"relay test". docs/names/C-notes.md (finding 1) showed it is the residual current monitor, and
the comments now say so.

## Inputs

The sensor is ADC channel c12 (`meas_fast.leak_i`, in mA: 0.484 mA per count about 2075).
Three values in A feed the monitor, `rcmu_i[0..2]` at 0xD488..0xD48C:

| Index | What | Made by | Rate |
|---|---|---|---|
| 0 | RMS over a grid cycle | task 1, `current2_rms` | per grid cycle |
| 1 | mean over a cycle (the DC part) | task 3, `meas_aux_f16` | per nominal cycle |
| 2 | mean of the last 10 samples | `fast_fault_check` in the control interrupt | every 10 samples, about 0.52 ms |

## Blocks and switches

`rcmu` at 0xDB40 holds three blocks: `a` (zero offset), `b` (self test) and `c` (watch).
`rcmu_step` runs them in that order from task 4, every 20 ms.

- `dsp_settings.rcmu_enable` (0xD8D6, HR1 bit 1 inverted) turns all three on.
- `rcmu_step_check_off` (0xD8DA, HR1 bit 4 inverted) turns off the sudden-change checks.
- A failed self test disables all three blocks until `rcmu_init` runs again (at start-up).

## Zero offset (block a)

After task 4 has run 500 times (10 s) it averages `rcmu_i[1]` over 8 runs. A result above
0.06 A counts as 0. The result goes to `rcmu_offset` (0xD412), which nothing reads. Its only
effect is `done`, which arms the self test.

## Self test (block b, GPIO52)

One step per 20 ms run, about 5 s in all:

1. Steps 1..14: test circuit open. Steps 16..34: the mean of `rcmu_i[1]` is the open-circuit
   reading.
2. Step 40: GPIO52 set, which closes the test circuit (my reading of the hardware).
3. Steps 46..89: the reading must differ from the open-circuit mean by 0.02 to 0.15 A. More than
   5 misses in a row restart the test, and the second restart sets `failed`.
4. Step 115: circuit open. Steps 151..249: the open-circuit means of `rcmu_i[0]` and `[1]`
   (avg0, avg1), which become the watch's base for clearing.
5. Then `passed`, which is run_status bit 7 (0xD7C1).

op_states.c will not leave the idle state while the RCMU is enabled and bit 7 is clear. So a
failed test keeps the inverter idle until the next start-up.

## Watch (block c) and the trip levels

The watch is armed in op_states 5/6 by the self test's `passed`. It waits 1100 runs (22 s) for
its two slow filters to settle (0.99 old / 0.01 new). The levels are 0.9 x VDE 0126-1-1's. The
standard values in this table are as I know them; the code holds only the 0.9 x numbers.

| Check | VDE 0126-1-1 | Code | Time in the code |
|---|---|---|---|
| continuous | 300 mA, 0.3 s | `rcmu_i[0]` or abs(`rcmu_i[1]`) over 0.3 A | more than 10 runs (220 ms) |
| sudden 30 mA | 0.3 s | after a step of 30 mA or more between readings 40 ms apart, more than 27 mA from the filtered value | 10 runs (about 200 ms) |
| sudden 60 mA | 0.15 s | the same with 54 mA | 3 runs (about 60 ms) |
| sudden 150 mA | 0.04 s | `rcmu_i[2]` or `[0]` more than 135 mA from the filtered value | at once (20 ms); `rcmu_fast_check` tests `[2]` every 0.52 ms |

The continuous limit is set to 0.3 A on every run. `rcmu_init`'s 0.86 never takes effect. So
the continuous level is 300 mA, not 0.9 x 300 mA.

A trip sets `trip` (or `fast_trip`, from the interrupt). CF18 bit 6 is their OR,
written by `rcmu_fast_check` on every call. The trip clears after 250 runs (5 s) with no check
counting and both readings within 16.8 mA above the self test's open-circuit means. While the
watch is disabled or unarmed, everything is cleared, including the trips.

## Open questions

- The step detector's gate compares `rcmu_i[0]` with `base1` where `base0` looks meant (see
  docs/findings-control.md).
- What CF18 bit 6 does further on (the stop fault query covers CF18, so the inverter stops; I have
  not traced the ARM's display of it).
- Whether `rcmu_offset` was meant to be subtracted from the readings somewhere. Nothing does it in
  this image.
