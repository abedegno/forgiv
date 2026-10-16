# State: the operating state machine, start-up and settings

Files: `src/app/state/state_task.c`, `op_states.c`, `app_init.c`; `src/app/settings/settings.c`,
`rating_presets.c`; headers `state.h`, `faults.h`, `settings.h`.

## The state machine (task 0, every 20 ms)

`task0_state_machine` calls `state_init` once and then the function of `op_state` (0xCF0D) for
ever. Each state function waits for events (os.h `EV0_*`) and returns when it picks a new state.

| State | Function | What it does |
|---|---|---|
| 0 idle | `state0_idle` | stops everything, waits for a run mode from the ARM; modes 5/6 after 10 s and the RCMU test, others at once (30 s with HR175) |
| 1 start | `state1_start` | runs the start check of the mode: `start_grid_pv` (5), `start_grid_batt` (6), `start_eps_pv` (7), `start_eps_batt` (8) |
| 3 fault | `state3_fault` | stops everything until the stop faults (CF17, CF18 low byte) clear |
| 4 update | `state4_update` | stops everything and runs the boot loader's update |
| 5 grid with PV | `state5_grid_pv` | soft start, conv_mode 5; grid fault -> 7, PV fault -> PV reset |
| 6 grid, battery only | `state6_grid_batt` | soft start, conv_mode 6; grid fault -> 8 |
| 7 EPS with PV | `state7_eps_pv` | soft start, conv_mode 7; PV fault -> 8 |
| 8 EPS, battery only | `state8_eps_batt` | soft start, conv_mode 8 |

The start checks count down `connect_countdown` (`rating_apply`: HR61 start time or HR62 restart
delay x 50 passes, 30 s .. 10 min), soft-start the battery DC-DC (`soft_start`), run the relay
test (`relay_test_step`) and close the relays. `soft_start` needs the start permit and no battery
fault (or the low-battery wake, or aging mode 1), raises the DC-DC duty to 1054 and sets CF15 bit
5 after 160 s without finishing.

The fault queries `has_pv_fault`, `has_grid_fault`, `has_stop_fault`, `has_eps_fault` and
`has_batt_fault` read the fault words CF12..CF19 that `state_init` clears at start-up; each task
sets its own bits (faults.h lists them).

## Start-up and settings

`app_init` (from main) clears the application words (no BMS until the ARM says so), then
`settings_defaults` (grid protection, calibration gains 1.0, PF and Q(U) curves) and
`init_defaults` (the rating block: 5 kW, 230 V 50 Hz, battery capacity 120 Ah, HR111/HR112 50 %,
HR97/HR98 42.0/56.0 V, SoC floor 4 %, battery rating 3.6 kW). The ARM's frames then overwrite
most of these. `rating_apply` (every idle pass with a run mode) picks the model's preset from
`rating_presets` (5.0, 4.6, 3.6, 3.0, 2.0 kW); an unknown model sets CF17 bit 10 and runs the
2 kW preset.

Battery behaviour across these files is in [../battery.md](../battery.md); bugs in
[../findings-power.md](../findings-power.md) items 1, 5 and 7.
