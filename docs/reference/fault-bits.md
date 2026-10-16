# Fault and status bits

Every fault and status bit that the code of either chip sets, with where it is set and what it
does. The DSP's words are `fault_words[8]` at 0xCF12 to 0xCF19, called CF12 to CF19 here and in
the per-tree notes. The names are those of
[faults.h](../../inverters/gen3-lv/d316/src/include/faults.h) where it has one. A bit not listed
is not set anywhere in the code I searched (every write to the eight words and their aliases).
How the bits travel is in [operation/faults.md](../operation/faults.md).

On the ARM, IR217 to IR224 are CF12 to CF19 with the two bytes of each word swapped
([operation/faults.md](../operation/faults.md#on-the-arm)).

## CF12: grid faults (high byte)

| Bit | Name | Set by | Meaning |
|---|---|---|---|
| 3 | | none found; `dc_measure.c` reads and clears it | while set, `pv1_ctrl` and `pv2_ctrl` do not run |
| 4 | `FG1_TZ_TRIP` | `tz_trip_isr` (converter_ctrl.c) | the hardware trip zone fired |
| 12 | `FG1_GRID_VOLT` | `grid_volt_protect` (grid_support.c); `fast_fault_check` (converter_ctrl.c) | grid voltage trip: a stage, the 10-minute mean, the instant 300 V, or the 396 V peak |
| 15 | `FG1_GRID_FREQ` | `grid_freq_protect` (grid_support.c) | grid frequency trip, or RoCoF |

Only the high byte counts as a grid fault (`has_grid_fault`). Task 3 clears the low byte
(meas_task.c).

## CF13: grid faults

| Bit | Set by | Meaning |
|---|---|---|
| 15 | `state_init` (op_states.c); `grid_support.c` | grid lost; set at start-up until the grid is measured |

## CF14: grid (low byte) and EPS (high byte) faults

| Bit | Name | Set by | Meaning |
|---|---|---|---|
| 5 | `FGE_EXPORT_OVERLOAD` | `export_overload_check` (grid_support.c) | export over the HR26 limit for too long (codes 7, 3, 19); also follows CF19 bit 2 for codes 3 and 19 |
| 7 | `FGE_RECONNECT_WAIT` | `grid_reconnect_check` (grid_support.c) | off the grid, V or f outside the reconnect window HR79-HR82 |
| 8 | | `eps_output_check` (ac_measure.c) | EPS under-voltage |
| 11 | `FGE_EPS_BUS_UV` | `dc_measure.c` | the DC bus too low for EPS |
| 12 | `FGE_EPS_OVERLOAD` | `eps_output_check` (ac_measure.c) | EPS overload |
| 13 | | `eps_output_check` (ac_measure.c) | EPS over-voltage |

## CF15: battery faults

Any bit is a battery fault (`has_batt_fault`): the battery converter stops (except in the
low-battery wake and aging mode 1) and the soft start is withdrawn. Details:
[D316 battery.md](../../inverters/gen3-lv/d316/docs/battery.md).

| Bit | Name | Set by | Set when | Cleared |
|---|---|---|---|---|
| 1 | `FB_V_MISMATCH` | `batt_voltage_monitor` (power_limits.c) | BMS mode: battery under 24.0 V, or measured and BMS HR22 voltages more than 5.0 V apart at low current; no BMS: under 20.0 V | 10 s within 3.0 V; no BMS: over 30.0 V |
| 2 | `FB_UNDER_V` | `batt_voltage_monitor`; `state_init` | under HR97 for 5 checks; set at start-up | 3 s above HR97 + 2.0 V |
| 3 | `FB_CAL_END` | `task6_bms_link` (bms_link.c) | the fall of HR175 | after 10 s |
| 4 | `FB_DCDC_RATIO` | `batt_voltage_monitor` | converter mode 5: the high side 50.0 V or more from 5.5 x the battery voltage for 50 passes | 500 passes later |
| 5 | `FB_SOFT_START_TIMEOUT` | `soft_start` (op_states.c) | 160 s of soft start without finishing | never, until the DSP restarts |
| 6 | `FB_OVER_V` | `batt_voltage_monitor` | over HR98 + 2.0 V for 2 checks or HR98 + 1.0 V for 50 (56.0 V without a BMS) | 10 s under the limit |
| 8 | `FB_BMS_ALARM` | `soc_floor` (power_limits.c) | the OR of alarm bits that nothing writes | always 0 |
| 9 | `FB_BMS_COMMS` | `bms_lost` (bms_link.c) | 30 s without a good BMS reply, BMS mode | the first good reply |
| 15 | `FB_HR19_B4` | `soc_floor` | schedule window 3 (HR318), or BMS HR19 bit 4 | when neither |

## CF16: PV faults

| Bit | Name | Set by | Meaning |
|---|---|---|---|
| 5 | `FPV_ABSENT` | `dc_measure.c`; `state_init` | both PV inputs under the start voltage for 51 cycles; set at start-up |

## CF17: stop faults

Any bit is a stop fault (`has_stop_fault`): the state machine goes to state 3.

| Bit | Name | Set by | Meaning | In the 32-bit word |
|---|---|---|---|---|
| 0 | `FS1_DCI` | `dci_control` (grid_support.c) | DC injection | bit 27 |
| 1 | `FS1_ISO` | `dc_measure.c` | insulation test failed 3 times; clears after 150 s | bit 25 |
| 2 | `FS1_NTC` | `ntc_temp_check` compares a constant, so never | | |
| 3 | `FS1_BUS_OV` | `dc_measure.c` | DC bus over its limit + 10.0 V for 5 cycles | bit 22 |
| 4 | `FS1_PV_OV` | `dc_measure.c` | PV over its limit + 35.0 V for 6 cycles | bit 28 |
| 6 | `FS1_CHARGER_OT` | `dc_measure.c` | charger over-temperature (93.0 degrees) | |
| 7 | `FS1_HEATSINK_OT` | `dc_measure.c` | heatsink over-temperature | bit 31 |
| 8 | | `eps_output_check` (ac_measure.c) | EPS short: under 40.0 V with over 15.0 A on 5 ticks | |
| 9 | | `arm_link_frame` (arm_link.c) | the CEI 0-21 auto test failed (result 3 or 4) | |
| 10 | `FS1_BAD_PRESET` | `arm_page0` (arm_link.c); `rating_apply` (settings.c) | unknown model byte in HR2 | |
| 11 | | `relay_test_check` (ac_measure.c) | the relay test failed 5 times; held for 1 hour | bit 17 |
| 14, 15 | `FS1_PV_SHORT` | `dc_measure.c` | PV1 or PV2 under 30.0 V with over 3.00 A; clears above 120.0 V | |

## CF18: stop faults (low byte) and ARM faults (high byte)

| Bit | Name | Set by | Meaning | In the 32-bit word |
|---|---|---|---|---|
| 2 | `FS2_ARM_LINK_LOST` | `task5_arm_link` (arm_link.c); `state_init` | 300 failed or empty wake-ups without a good ARM frame; set at start-up; 40 good frames clear it | bit 21 |
| 6 | `FS2_CAL_FAULT` | `rcmu_fast_check` (rcmu_fast_check.c) | the residual current monitor tripped (faults.h calls it a calibration fault; the RCMU code shows what it is) | bit 19 |
| 8 | | `arm_frame_parse` (arm_link.c) | header byte 2 bit 2: the ARM's aux link fault (0x40) | |
| 9 | | `arm_frame_parse` | header byte 2 bit 4: ARM fault bit 0x100, which nothing on the ARM sets | |

The low byte counts in `has_stop_fault`; `select_state` also requests state 3 for any bit of
the high byte.

## CF19: battery blocks

| Bit | Name | Set by | Effect |
|---|---|---|---|
| 2 | | `arm_page4` (arm_link.c) | page 4 byte 28, the ARM's meter link state: mode 0 battery command 0 |
| 7 | `BB_EPS_OVERLOAD` | `eps_output_check` (ac_measure.c) | EPS output over 1.1 x rated |
| 8 | `BB_FULL` | `soc_floor` (power_limits.c) | battery full: no charging, `full_current_trim` runs |
| 9 | `BB_FORCE_CHARGE` | `soc_floor` | SoC at or below the floor minus 3: at least 300 W of charge |
| 10 | | `soc_floor` | follows a BMS status bit nothing writes: always 0 |
| 11 | `BB_NO_DISCHARGE` | `soc_floor` | SoC at or below the floor: no discharge |

## The 32-bit fault word (header bytes 3-6, IR39 and IR40)

| Bit | From |
|---|---|
| 6 | ARM `FAULT_AUX_LINK` |
| 7 | ARM `FAULT_DSP_LINK` |
| 8 | ARM 0x100 (never set) |
| 17 | CF17 bit 11, relay test |
| 19 | CF18 bit 6, residual current |
| 21 | CF18 bit 2, ARM link lost |
| 22 | CF17 bit 3, bus over-voltage |
| 25 | CF17 bit 1, insulation |
| 27 | CF17 bit 0, DC injection |
| 28 | CF17 bit 4, PV over-voltage |
| 31 | CF17 bit 7, heatsink over-temperature |

Bits 6 to 11 are the ARM's (`task_dsp_rx` replaces them with `fault_flags & 0xFC0`); the rest
come from `arm_fault_word_build`. IR39 is the high half, IR40 the low half.

## The warning word (header bytes 7-8, IR57)

| Bit | From |
|---|---|
| 8 to 15 | low byte of `bms_alarm`, BMS HR20: bit 10 is HR20 bit 2 ("full"), bit 11 is HR20 bit 3 ("empty") |
| 1 | CF15 bit 6, over-voltage |
| 2 | CF15 bit 2, under-voltage |
| 3 | CF15 bit 5, soft-start timeout |
| 5 | CF15 bit 9, BMS comms |
| 7 | CF17 bit 6, charger over-temperature |

From [`batt_warning_update`](../../inverters/gen3-lv/d316/src/app/battery/bms_link.c) and
`task5_arm_link`. HR351 = 1 makes the ARM store 0; the meter fault makes IR57 read 1.

## The ARM's own bits

| Bit | Name | Meaning |
|---|---|---|
| 0x40 | `FAULT_AUX_LINK` | the auxiliary MCU silent; does not set run mode 3 on the ARM, but reaches the DSP as CF18 bit 8 |
| 0x80 | `FAULT_DSP_LINK` | the DSP link failing; sets run mode 3 |
| 0x100 | | set only by dead code |

The meter fault is a separate byte (`board_init_done`, a misleading name the A316 notes keep):
it makes IR0 read 2 and IR57 read 1, and is sent to the DSP as page 4 byte 28.

## Sources

- DSP: [faults.h](../../inverters/gen3-lv/d316/src/include/faults.h),
  [op_states.c](../../inverters/gen3-lv/d316/src/app/state/op_states.c),
  [power_limits.c](../../inverters/gen3-lv/d316/src/app/power/power_limits.c),
  [bms_link.c](../../inverters/gen3-lv/d316/src/app/battery/bms_link.c),
  [dc_measure.c](../../inverters/gen3-lv/d316/src/app/meas/dc_measure.c),
  [ac_measure.c](../../inverters/gen3-lv/d316/src/app/meas/ac_measure.c),
  [grid_support.c](../../inverters/gen3-lv/d316/src/app/grid/grid_support.c),
  [converter_ctrl.c](../../inverters/gen3-lv/d316/src/app/control/converter_ctrl.c),
  [arm_link.c](../../inverters/gen3-lv/d316/src/app/comm/arm_link.c),
  [rcmu_fast_check.c](../../inverters/gen3-lv/d316/src/app/rcmu/rcmu_fast_check.c).
- ARM: [faults.h](../../inverters/gen3-lv/a316/src/include/faults.h),
  [monitor.c](../../inverters/gen3-lv/a316/src/app/control/monitor.c),
  [dsp_link.c](../../inverters/gen3-lv/a316/src/app/comm/dsp_link.c),
  [meter.c](../../inverters/gen3-lv/a316/src/app/comm/meter.c).
