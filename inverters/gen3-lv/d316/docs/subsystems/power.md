# Power: the control task and the MPPT

Files: `src/app/power/power_limits.c`, `mppt.c`, `mppt_step.c`; headers `power_limits.h`,
`mppt.h`, `rating.h` (the `ac` block's battery current maxima and PV PIs).

## power_limits.c: RTOS task 2, every 20 ms

`task2_control` runs, in the image's order: `status_led_and_reset`, `hv_current_max_set`,
`ctl_voltage_limits_set`, `soc_floor`, `batt_voltage_monitor`, `full_current_trim`,
`batt_power_setpoint` (which calls `batt_power_blocks` and `current_limit_tapers`, which calls
`soc_power_limits`), `soft_start_permit`, `aging_test_update`.

What it produces:

- the battery fault bits CF15 1, 2, 3, 4, 6, 8, 15 and the blocks CF19 8, 9, 10, 11;
- the start permit 0xD52D and `soft_start_enable` 0xD52E that op_states.c needs;
- the battery power command (0.1 W) and current command (0.1 A, HV side) for the interrupt code;
- the current limits 0xD522..0xD528, the power limits 0xD50E..0xD51C and the current bounds
  0xC212/0xC214 that `current_ctrl.c` clamps to;
- the full-battery trim 0xC206 and the ISR voltage limits 0xC208/0xC20A (used only without a BMS);
- the status LED and four self-resets through GPIO27.

Every rule, with its thresholds and formula, is in [../battery.md](../battery.md). The likely bugs
and dead code are in [../findings-power.md](../findings-power.md).

Matching notes: the file reaches the BMS page as `union bms_page_u` (named words and an array view)
and the fault words as `union fault_words_u`; three globals are signed here where the header has
them unsigned (`OWN_*` defines at the top); its .ebss order is kept by `tools/ebss_solve.py`.

## mppt.c and mppt_step.c: the PV inputs

`pv_mppt_step` runs from task 3 once per slow average (one grid period). In states 5 and 7 it ramps
each boost's current limit up (5 or 50 per call to i_max 1500), takes the open-circuit voltages on
a start or rescan (25 calls of waiting), and calls `mppt_pv1`/`mppt_pv2` by PV mode (1, 2, 4
parallel, 5 independent). Each MPPT first sweeps down from Voc - 2 V in 2 V steps (to 0.83 x Voc,
120 V or 50 steps) and then runs perturb and observe (`mppt_po_pv1/2`) on 50-sample power sums:
5.0 V steps, 2.5 V half steps to dither at the peak. A reference that drifts 30 V (PV2: 20 V) from
the measured voltage for 500 (250) calls is pulled back. Low power for 161 sums, or both inputs
under 120 V for 51 calls, asks for a rescan. Aging mode 1 (HR332) fixes both references at 250 V.

The two MPPT files probably were one original file: `pv_mppt_step` uses two of mppt.c's
variables. They were matched by different agents.
