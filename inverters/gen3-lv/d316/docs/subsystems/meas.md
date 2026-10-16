# Measurement: ADC, averages, task 1 and task 3

Files: `src/app/meas/`. These run in the control interrupt: adc_scale.c, meas_avg8.c,
avg_win.c, dc_avg.c, grid_rms_accum.c and eps_rms_accum.c. Task 3 is meas_task.c and
dc_measure.c; task 1 is ac_measure.c. The headers are `src/include/meas.h` and `rating.h`.
Everything here is read from the code; the units are what the code's limits imply.

## Data flow

```
ADC (DMA, 0xDB86) -> adc_scale -> meas_fast (0xC0C0, every sample, 19.2 kHz)
   -> meas_avg8   -> meas_avg (0xC140, 8-sample means, bus notch filter)   -> control loops
   -> grid_rms_accum, eps_rms_accum -> ac_sums -> task 1 (once per grid / EPS cycle)
   -> dc_acc_step  -> dc_acc (0xD180, once per nominal cycle)             -> task 3
```

## ADC scaling (adc_scale)

Each channel is the mean of the two conversions in the DMA buffer, less an offset. The AC
channels use mid-scale 2048, the residual current 2075, and the currents the offsets measured
at start-up (eps_cycle page, 0xCC80..).

| Value | Scale per count | Unit | Read from |
|---|---|---|---|
| grid, inverter, EPS voltage | 2.2139 | 0.1 V | 396.0 V peak trip, 230 V nominal |
| inverter current | 0.2096 | 0.1 A | 40.0 A trip |
| PV, bus, high-side voltage | 1.4805 | 0.1 V | 585.0 V bus limit |
| battery voltage | 0.1792 | 0.1 V | 63.0 V trip, 56.0 V limits |
| PV currents | 0.8076 | 0.01 A | 3.00 A gain change, 18.00 A limit |
| high-side current | 1.1768 | 0.01 A | 20.00 A trip (inference) |
| residual current | 0.4844 | mA | RCMU levels in A after /1000 |
| DC injection | 0.8886 | not established | |
| insulation | 0.1858 | not established | |

`adc_scale` also keeps the last three grid voltage samples and a lead term, a filtered second
difference clamped to +-10.0 V. The grid current controller adds it to its feed-forward. PV
currents at or below 0 read as 1.

## Averages in the interrupt

- `meas_avg8`: eight-sample means of the PV, bus, high-side and battery voltages and the PV
  currents. It also steps `ctrl_slot`. The bus mean goes through a second-order notch at 2.4 kHz:
  99 Hz with the 50 Hz coefficients, 119 Hz with the 60 Hz ones. So `bus_v_filt` has the
  double-line-frequency ripple removed. The battery converter's feed-forward
  `duty_ff = pwm_period x hv_v / bus_v_filt` is computed here.
- `avg_win_step`: the mean of abs(v) over a quarter cycle (96 samples at 50 Hz, 80 at 60 Hz),
  for the alternative grid voltage check (window 91..345 V, which is about 101..383 V RMS for a
  sine).
- `grid_rms_accum`: per grid period, the sums of inverter v^2, current^2, grid v x i, grid v^2,
  residual current^2 and the DC injection channel. `grid_period_isr` hands them to task 1.
- `eps_rms_accum`: per EPS angle turn, EPS v^2, current^2 and v x i.
- `dc_acc_step`: per nominal cycle (384 or 320 samples), the sums of ten DC channels for task 3.

## Task 1 (ac_measure.c)

Grid period event (once per grid cycle):

- RMS of the grid voltage: one cycle calibrated (`grid_voltage_fast`, 0xD3CA) and filtered
  0.9/0.1 (`grid_voltage`, 0xD3CC);
- RMS of the inverter voltage, grid current and residual current. The last goes to
  `rcmu_i[0]` in A;
- DC injection: the cycle mean, 0xD3FA;
- frequency in 0.01 Hz from the median of the last three ECAP periods: `9e9 / period + 2`,
  0 below 41.00 Hz;
- active power, and power factor or apparent power;
- a PI on the headroom to task 4's power limit (kp 0.3125, ki 0.1, +-350 W). Its output
  `grid_pi_out` (0xD3F6) is task 4's `p_limit_trim`, added to the power limit before task 4
  turns it into the grid current bounds. This closes the loop on the measured power;
- 50/60 Hz detection: 26 periods in 46..54 Hz or 56..64 Hz;
- the grid-loss watchdog kick.

EPS event: EPS RMS voltage and current, power, and the slow EPS voltage loop. The loop ramps the
RMS reference 10.0 V per cycle from 50.0 V to nominal, and after 50 cycles a PI (kp 0.8, ki 0.1)
adds 0..50.0 V to the amplitude (`amp_trim`).

20 ms tick:

- power filters and the load %;
- the bus voltage reference: 430.0 V, or 400.0 V in EPS or with run mode 7/8, at most 5.0 V
  per tick;
- the relay test checks;
- the EPS output checks;
- the ARM power check;
- relays off when disabled;
- the grid-loss watchdog;
- the run mode request.

| Check | Rule | Fault |
|---|---|---|
| grid lost | no grid period for 3 ticks (60 ms); 16 ticks for code 25 with 0xD7C2 bit 4 | state_flags bit 8, grid fault event |
| relay test (state_flags bit 4) | inverter and grid more than 30.0 V apart for 10 ticks | stop; 5th time CF17 bit 11 for 1 hour |
| relay test (bit 5) | inverter side over 160.0 V or EPS side under 160.0 V for 10 ticks | as above |
| EPS over-voltage | over 1.1 Vn for 51 ticks, or 1.2 Vn for 26 | CF14 bit 13, clears after 30 s |
| EPS overload | S over 1.1 x rated for 151 ticks, or 1.5 x for 6 | CF14 bit 12 (CF19 bit 7 at once) |
| EPS under-voltage | under 0.85 Vn or 0.8 Vn for 101 ticks | CF14 bit 8; 3 trips lock for 1 hour |
| EPS short | under 40.0 V with over 15.0 A on 5 ticks (not reset in between) | CF17 bit 8 |
| ARM power | over 1.5 x rated for 25 ticks, or 1.1 x (0.9 x when forced charging) for 500 | relay_c open, ARM fault bit 3; twice 30 s, then 1 hour |

Run mode request (`select_state`), in order:

| Request | When |
|---|---|
| 4 | the ARM asks for an update |
| 0 | stop, disabled or reconfiguring |
| 3 | any stop fault (CF17, CF18) |
| 7 | low-battery wake with no PV fault |
| 5 | no grid fault and no PV fault |
| 6 | no grid or battery fault, but a PV fault |
| 7 | a grid fault, and no EPS, battery or PV fault |
| 8 | a grid fault, a PV fault and EPS enabled |
| 0 | anything else |

## Task 3 (meas_task.c, dc_measure.c)

On the slow average event (once per nominal cycle):

- PV, bus, battery and high-side values. Calibration gains are counts over 1024, accepted in
  870..1178;
- PV input detection: present above 120.0 V, absent at or below 90.0 V. The result gives
  pv_input_mode 0, 1, 2, 4 (parallel) or 5;
- the MPPT step (mppt_step.c);
- the bus filter and modulation gain;
- boost selection: a boost starts when its MPPT reference is 20.0 V under the bus reference;
- the PV power limit: rated, derated 0.06 %/V above the derate voltage, 0.2 %/V more than
  40 V above it, 24 % above 545.0 V.

| Check | Rule | Fault |
|---|---|---|
| PV over-voltage | max PV over pv_ov_limit + 35.0 V for 6 cycles | CF17 bit 4; clears after 500 cycles 10 V under |
| PV absent | both under the start voltage (HR60; 105.0 V in PV states) for 51 cycles | CF16 bit 5 |
| PV short | under 30.0 V with over 3.00 A for 6 cycles | CF17 bit 14 / 15; clears above 120.0 V |
| bus over-voltage | over bus_ov_limit + 10.0 V for 5 cycles | CF17 bit 3; clears after 500 cycles 10.0 V under |
| bus low (grid states) | 30.0 V under its reference for 51 cycles | stop event |
| heatsink / charger temperature | over the limit for 50 cycles | CF17 bit 7 / 6; 500 cycles 10 degrees under to clear |
| battery temperature | counted against 100.0, never read | none |
| EPS bus | under 0.79 x sqrt 2 x Vn for 150, under 0.75 x for 5, or less than 20.0 V over the high side for 50 | CF14 bit 11 |
| insulation (start state) | resistance under 50, 3 times | CF17 bit 1, clears after 150 s |

On the 10 ms tick task 3 posts the fault events to task 0: 1 stop, 2 grid, 3 PV, 5 EPS. It also
runs the MPPT rescan. Rescan 1 clears the PV loops and waits 100 us. Rescan 2 waits about 1 s
for the bus to come within 25.0 V of its reference, then asks for the bridge to start again.

## Open questions

- The units of the DC injection and insulation channels, and so of `dci_limit` and the
  insulation threshold of 50 (kilohm would fit, but nothing in the code says so).
- `dc_acc.b`: meas_init clears it and sets its count, and nothing else uses it.
