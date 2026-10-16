# Grid: task 4, protection and the grid code

File: `src/app/grid/grid_support.c` (task 4, 20 ms) with `src/include/grid.h`. Its RAM is reached
through `src/include/ram_view.h`. The settings come from the ARM (arm_link.c,
arm_page_write.c). The HR numbers below are from map/names.tsv, where the naming pass matched
each DSP word to the ARM's frame byte and register. All rules are read from the code. Where I
name a standard, that is inference from the numbers.

Units: grid voltage 0.1 V, frequency 0.01 Hz, power 0.1 W. Times are in ticks of 20 ms unless
the setting is in ms. op_state 5 is grid with PV, 6 grid with the battery only.

## Task 4, in order

`ctl_timers_tick`, `grid_reconnect_check`, `grid_v_10min_update`, `grid_volt_protect`,
`grid_freq_protect`, `rcmu_step`, `dci_control`, `grid_v_fast_trip`, `autotest_run`,
`export_overload_check`, `reactive_power_update`, `power_limits_update`,
`grid_support_update`.

## Grid codes

`grid_code` (0xD34A) is the high byte of HR2. The code never names a country; it tests numbers
in groups:

| Group | Codes | What sets it apart |
|---|---|---|
| Italy | 4 | CEI 0-21 auto test, own frequency and voltage logic, `freq_response_it` |
| Australian-style | 3, 19 | volt-watt 253 to 260 V (3) or 242 to 250 V (19), under-voltage storage response, latched under-frequency response, cos phi(P) 0.95 |
| European-style | 2, 9, 11, 13, 22, 23, 24, 25, 26, 28 | RoCoF, Q(U) fixed curves (modes 8/9), cos phi(P) 0.9, over-voltage storage response |
| PT1 code | 7 | PT1 smoothing of Q, PF by voltage, its own export overload check |
| fixed OF recovery | 8, 12, 16, 17 | over-frequency recovery at 50.20/60.20 Hz, volt-watt at 1.13 Vn |
| others | 0, 1, 6, 15, 18 | appear in some lists only |

The ARM maps codes 20 and 21 to 3 (docs/names/A-notes.md). The volt-watt points 253/260 V and
242/250 V match the Australia and New Zealand settings of AS/NZS 4777.2, as far as I know them.
So 3 is probably Australia and 19 New Zealand, but that is inference.

## Voltage and frequency protection

| Stage | Setting | Time | Fault |
|---|---|---|---|
| over-voltage 1 | HR64 `v_ac_high_out` | HR68 | CF12 bit 12 |
| under-voltage 1 | HR63 `v_ac_low_out` | HR67 | CF12 bit 12 |
| over-voltage 2 | HR72 `v_ac_high_in` | HR76 | CF12 bit 12 |
| under-voltage 2 | HR71 `v_ac_low_in` | HR75 | CF12 bit 12 |
| 10-minute mean | HR83 | 50 ticks (Italy: HR76) | CF12 bit 12 |
| instant | 300.0 V (276.0 V for code 18) | 2 ticks | CF12 bit 12 |
| over-frequency 1 | HR66 | HR70 | CF12 bit 15 |
| under-frequency 1 | HR65 | HR69 | CF12 bit 15 |
| over-frequency 2 | HR74 | HR78 | CF12 bit 15 |
| under-frequency 2 | HR73 | HR77 | CF12 bit 15 |
| RoCoF (European group) | 1.10 Hz over 26 samples (about 2.2 Hz/s) | one window | CF12 bit 15 |

- The voltage checks use the one-cycle RMS (`grid_voltage_fast`, 0xD3CA). The stage counters
  compare with `>` for three stages and `>=` for over-voltage 2, so the times differ by one tick.
- A voltage fault clears after 101 ticks (2 s) inside the stage 1 window with the 10-minute mean
  under HR83. A frequency fault clears after 250 ticks (5 s) inside the stage 1 window.
- The 10-minute mean (`grid_v_10min_update`) takes a 3 s mean into a ring of 200 slots. The ring
  starts at 230.0 V.
- Frequency protection waits for 10 captured periods and the 50/60 Hz detection.
- Italy: stage 1 frequency is used only during the auto test's steps 7 and 8. The instantaneous
  stage 1 over-voltage counts only in auto test step 5; normal stage 1 over-voltage is the
  10-minute mean. That matches CEI 0-21's 59.S1 on the 10-minute mean and 81.S1 only when
  enabled (inference).
- The grid-loss watchdog is in task 1 (3 ticks without a period). While the grid is lost, CF13
  bit 15 is set.
- Reconnect window (`grid_reconnect_check`): off the grid, CF14 bit 7 is set while V or f is
  outside HR79..HR80 / HR81..HR82. It clears after 101 ticks inside.
- Momentary cessation (`grid_v_fast_trip`; European group, 3, 7, 19 with 0xD7C2 bits 4 and
  15): at or below HR315, or at or above HR316, the bridge stops at once but the inverter stays
  in its grid state. 6 ticks after the voltage is back, state_flags bit 13 restarts the bridge.

## DC injection (dci_control)

In op_states 5/6 with `dci_comp_enable` (0xD8D8). After 20 s the DC component is filtered, and
once a second a correction of +-1 (limit +-200) is added to the bridge voltage while it is over
25. After 60 s, abs(DC) over `dci_limit` for `dci_trip_time` ticks sets CF17 bit 0. For Italy
the levels are 1000 and `dci_trip_time_it`. The fault clears 30 s after leaving the grid.

## Active power limits (power_limits_update)

`out_power_limit` (0xCF2C) is the smallest of these:

| Limit | Rule |
|---|---|
| soft start | rated / 6000 per tick after a start (2 minutes); the aging test 24 s; with `slow_ramp_req` HR306 per mille of rated per minute (HR302 after a grid fault) |
| P(f) over-frequency (op_state 5) | see below |
| temperature | 8 % per degree over the heatsink or charger derate point (87.0 C by default), 10 % floor |
| volt-watt | see below |
| DC side | 0.2 %/V over a 520.0 V bus, 0.4 %/V over 560.0 V, 28 % over 570.0 V; while charging, PIs on PV power and on PV current over 18.00 A |
| rated | |
| P beside Q | sqrt(S^2 - Q^2) |
| HR50 rate | HR50 % of rated, slew-limited by code (50 W, 40 W or HR306/HR302 per tick) and raised by P(f) under-frequency |

The current bounds the interrupt uses come from this limit plus task 1's trim (0xD3F6), as a
peak current (0.1 A). The export limit (HR178) and the import limit (HR102 bit 6, HR101 A) can
narrow them. The bounds are at least +-0.5 A. With the charge target on (HR20) and SoC at HR116
after the ramp, the import side is held at -0.5 A, so the grid does not charge the battery.

Volt-watt: the voltage is filtered 0.9/0.1.

- Codes 3 and 19 slew to `rated x (1 - 0.8 (V - start) / (top - start))` every 100 ms, with
  start/top 253/260 V and 242/250 V.
- The European group steps down 0.04 % of rated per tick above 1.10 Vn.
- Codes 8/12/16/17 use 1.13 Vn and 0.029 %.
- Below `grid_v_low_derate` (220.0 V) the power is cut in proportion to the voltage, which holds
  the current.

## Frequency and voltage responses

In op_state 5 (with PV), as limits on the output:

- Over-frequency (HR125 bit for OF, 0xD7C2 bit 3): from HR321, after HR300 ticks, the limit
  falls by HR123 / 10000 of the base per 0.01 Hz. It never rises while active for 3, 19 and the
  European group with HR179. Recovery below HR328 (50.20/60.20 Hz for 8/12/16/17) after the
  recovery delay, ramping 3.05e-5 x rated per tick (5.5e-5 for 3/19).
- Italy: above HR321 after HR300 ms, a fixed 0.77 % of the base per 0.01 Hz while
  `ofr_it_param` is 20..50. Recovery within 49.90..50.10 Hz after 5 minutes.
- Under-frequency (0xD7C2 bit 12): below HR326 after HR312 ticks, the limit rises by HR327 /
  10000 of rated per 0.01 Hz. Codes 3/19 latch the base and recover above 49.85 Hz after 30 s.

In op_state 6 (battery only), as a battery power command (`freq_resp.grid_resp_power`, 0xCE1E).
The priority is over-frequency, under-frequency, over-voltage, under-voltage. Italy has its own
version (`freq_response_it`):

| Response | Codes | Rule |
|---|---|---|
| over-frequency | European, 7, 3, 19 | from the battery power at HR321 down with HR123's droop; 3/19 to 0 at 50.75 Hz (51.00 Hz for 19), then to -95 % at 52.00 Hz; recovery ramps |
| under-frequency | European, 3, 19 | from the battery power at HR326 up with HR327's droop; 3/19 to rated at 48.00 Hz (through 0 at 49.00 Hz when charging) |
| over-voltage | European | over 253.0 V (248.4 V for 13): -1 % of rated every 1.4 s down to -85 % |
| under-voltage | 3, 19 | under 215.0 V (224.0 V for 19): charging over 18 % of rated is cut to 18 % at 207.0 V (216.0 V) |
| Italy | 4 | UF 49.80/49.60 to 49.10 Hz up to rated; OF HR321 to 51.50 Hz down to -rated; OV over 253.0 V |

## Reactive power (reactive_power_update)

Q mode is HR122. Q is limited to 60 % of rated. The output filter capacitor's own reactive power
(2 pi f C V^2 with C = 8.4 uF, from the constant) is added. The reactive current reference goes
to the interrupt slewed 0.1 A per tick.

| Mode | Function | Rule |
|---|---|---|
| 1 | `q_fixed_pf` | PF from HR52: codes 1..2500 give 1.0 down to 0.75, 17500..20000 the same with the other sign; Q = P tan phi; Q = 0 below 4 % load |
| 2 | `q_cosphi_p` | from 50 % load: PF 1.0 down to 0.95 at full load (3, 7, 19) or 0.9 (4 and European, with the HR144/HR145 voltage lock-in) |
| 3 | `q_pf_curve` | PF(P) curve; never acts (see findings) |
| 4, 5 | `q_fixed_percent` | HR51 % of rated (1..60), two signs |
| 6 | `q_of_u_lockin`, `q_of_u_curve` | Q(U) on HR138..HR141; maximum 60 %/42 % (code 3), 60 % (19), 43.8 % (others); lock-in above HR142 % load and lock-out below HR143 for 4 and European |
| 8, 9 | `q_of_u_fixed` | European only: 43.6 % (PF 0.9) by voltage bands 207..253 V |

Code 7 smooths cos phi(P) with a PT1 (time HR301) and restores fixed Q from 0 after a low-voltage
cessation.

## CEI 0-21 auto test (code 4)

The ARM starts it (`auto_test_state` 1). There are eight steps, in this order: 59.S2 (HR72),
27.S1 (HR63), 81>.S2 (HR74), 81<.S2 (HR73), 59.S1 (HR64), 27.S2 (HR71), 81>.S1 (HR66) and
81<.S1 (HR65).

Each step waits 5 s, then moves a test value from the measured one by 2.0 V (2.5 V for step 6)
or 0.02 Hz every second. The protection uses the test value until it trips. The trip time must
be within 27 ms of HR time x 20 + 20 ms. The result codes are 2 pass, 4 out of time or 300
steps, 3 no trip, and 5 done. The ARM reads them on D frame page 4.

## Open questions

- The grid code to country mapping beyond 4 (and probably 3 and 19).
- The meaning of 0xD7C0, 0xD7C1 and 0xD7C2 bits beyond those used here (A-notes has the HR
  sources).
- The unit of HR301 (seconds, if `q_pt1_time`'s 0.02 per call is seconds).
- Why `ofr_power_limit` writes the HR303 setting for some codes (see findings).
