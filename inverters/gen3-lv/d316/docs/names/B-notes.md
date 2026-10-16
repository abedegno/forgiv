# Range B names: notes (0x3EBAB2 to 0x3EF54A)

File names: this is a log of its time, so it keeps the flat file names src/ had then. Step 3
of the readability pass renamed the sources and moved them into subdirectories;
map/filenames.tsv gives each old name its new path.

These are the notes for build/names/B.tsv. The names are proposals only. I have not changed src/.

Units. "Pass" means one run of the task, so 20 ms for tasks 0, 1 and 2 and 10 ms for task 3.
Voltages in 0xD2xx and 0xD8DE/0xD8DF are in 0.1 V. Powers in 0xD8xx and 0xD5xx are in 0.1 W.
The BMS limits 0xD504/0xD505 and the caps 0xD522..0xD528 are in 0.01 A. Temperatures are in
0.1 degree. The current bounds 0xC212/0xC214 are in 0.1 A on the HV side of the DC-DC.

## Where the meanings come from

- The ARM's `dsp_build_page` (ARM src/app/comm/dsp_link.c) gives the frame byte of each setting.
  The DSP handlers index `var_CFC0` by that frame byte. This ties 0xD8C5 to HR111, 0xD8C6 to HR112,
  0xD8C7 to charge_target_soc_now, 0xD8C8 to HR114, 0xD8C9 to HR110, 0xD8E0 to HR55, 0xD8E1 to HR54,
  0xD8D3 to HR27, 0xD8EF to HR332 (fD2), 0xD909 to HR175, 0xD803 to HR60, 0xD892 to HR308,
  0xD400 to HR309, 0xD896/0xD898 to HR313/HR314, 0xD85D/0xD85E to HR20/HR116 (ARM names), and
  the calibration gains 0xD808..0xD818 to HR4097..HR4146.
- The DSP's `arm_link_frame` (range A) and the ARM's `dsp_parse_page` give the measurements:
  0xD204/0xD206 PV voltages, 0xD21C/0xD220 PV currents, 0xD228/0xD22A PV powers, 0xD210 bus
  voltage, 0xD212 battery voltage, 0xD214 the ARM's bus_hv_voltage, 0xD218 battery power, 0xD3CC
  grid voltage, 0xD31C frequency, 0xD482 power factor, 0xD3E8 EPS voltage, 0xD484 load percent.
- The settings page 0xD8xx is reached both as `var_D8C0.vD8xx` and as separate externs
  (`var_D8E1`, `var_D8E9` ...). The TSV gives a var and its field the same name so that they can
  be merged into one object.

## Functions

### ARM pages and task 3 (arm_page_b.c, task3_b.c)
- arm_page6: stores the HR4120/HR4146 gains, the BMS request the ARM forwards (bytes 21..26, flag
  byte 20), HR300..HR303, HR178/HR179 bits and HR305.
- arm_page7: stores HR306..HR309, the charge target SoC now, HR101, HR312..HR316, the HR102 bits
  (EPS enable 0xD7C0 bit 9), the schedule window, HR4000, HR20 and HR116.
- task3_measure (10 ms): runs the DC measurements and posts the fault events (1 stop, 2 grid,
  3 PV, 5 EPS). After an MPPT rescan it waits 100 passes for the bus.

### DC measurement (meas_b.c)
- meas_batt_dcdc: battery voltage 0xD212, the HV-side voltage and current, battery power 0xD218.
- meas_pv / meas_pv_power / meas_bus_voltage / meas_iso / meas_aux_f16: scale the averages with
  the 1/1024 gains.
- meas_init: clears the accumulators and the MPPT block.
- pv_input_detect / pv_input_detect_run: decide which PV inputs are present (above 120 V) and the
  PV mode 0xD8E9.
- pv_boost_select: per input, turns the boost on when the MPPT reference is more than 20 V below
  the bus reference, and passes the bus reference to the ISR.
- pv_voltage_check: PV over-voltage (CF17 bit 4) and PV absent (CF16 bit 5).
- pv_short_check: under 30 V with more than 3 A (CF17 bits 14/15).
- pv_power_limit_update / pv_power_limit_flags: PV power limit, derated above 480 V.
- pv_iso_check / iso_fault_clear: the insulation test (CF17 bit 1, cleared after 150 s).
- bus_voltage_check / bus_voltage_filter / eps_bus_voltage_check: bus over-voltage (CF17 bit 3),
  the modulation gain, and EPS bus under-voltage (CF14 bit 11).
- temp_check: heatsink and charger over-temperature (CF17 bits 7 and 6).
- ntc_temp_check, ntc_to_temp: dead (see below).
- pv_ref_max_update: MPPT upper limit = open-circuit voltage + 2 V.

### Control task (control_b.c)
- task2_control (20 ms) runs, in order: status_led_and_reset, hv_current_max_set,
  ctl_voltage_limits_set, soc_floor, batt_voltage_monitor, full_current_trim,
  batt_power_setpoint, soft_start_permit, aging_test_update.
- batt_power_setpoint (was forced_charge_discharge): the battery power command in every mode,
  then batt_power_blocks, the current command 0xC216, and current_limit_tapers.
- batt_power_blocks (was force_charge_power): applies the CF19 blocks to the command.
- soc_power_limits (was charge_taper_soc): charge power by SoC, and both HR20 bits.
- current_limit_tapers: the current limits, the power limits and the ISR current bounds.
- soc_floor: CF19 bits 8, 9, 11, CF15 bits 8 and 15, the EPS latch.
- batt_voltage_monitor: CF15 bits 1, 2, 3, 4, 6 and the start permit.
- full_current_trim (was ctl_step6): while the battery is full, trims a small current from the
  BMS pack current.
- soft_start_permit (was fault_hold): 0xD52E.
- aging_test_update (was ctl_step9): 0xD8F0 from HR332.

### Start-up (init_b.c)
TI code under TI names: InitSysCtrl, XtalOscSel, InitPll, InitPeripheralClocks, InitPieCtrl,
InitPieVectTable, AdcOffsetSelfCal, AdcChanSelect, AdcConversion, ConfigCpuTimer. The others use
TI's style: InitAdc, InitGpio, InitEPwm, InitECap, InitDmaAdc, epwm_trip_force, epwm_trip_clear,
adc_offset_measure, plus init_system, sci_init and timer2_init.

### State machine (task0_b.c, state_b.c)
- States: 0 idle, 1 start, 3 fault, 4 firmware update, 5 grid with PV, 6 grid without PV, 7 EPS
  with PV, 8 EPS without PV. The event bits are os_post(0, n) = bit n: 1 stop fault, 2 grid
  fault, 3 PV fault, 4 reset, 5 EPS fault, 6 update, 7 update data.
- start_grid_pv / start_grid_batt / start_eps_pv / start_eps_batt: the start checks for modes 5..8.
  Each runs soft_start, counts down 0xD8CB, runs the relay test, and returns the new state.
- state_init, converters_stop, relay_test_step, relay_test_abort, pv_boost_precharge,
  grid_in_range.
- Fault queries: has_stop_fault (CF17, CF18 low), has_grid_fault (CF12 high, CF13, CF14 low),
  has_pv_fault (CF16), has_eps_fault (CF14 high), has_batt_fault (CF15, 0xD502 bit 11; was
  any_fault), has_fault_cf12_low (not called).
- soft_start: the DC-DC soft start. Its duty ramps to 1054, faster when the HV side tracks
  5.5 x Vbatt. After 8000 passes (160 s) without finishing it sets CF15 bit 5.

### AC task (rating_b.c)
- task1_ac_measure (was task1): RMS values, power, power factor, frequency, the EPS voltage loop,
  the grid-loss watchdog, the relay test check, and select_state.
- grid_voltage_rms, grid_current_rms, inv_voltage_rms, current2_rms, eps_voltage_rms,
  eps_current_rms, eps_power_calc, grid_power_pf_calc, grid_power_filter, grid_freq_measure,
  grid_freq_detect (50/60 Hz), bus_voltage_ref_update, eps_voltage_control, eps_dc_offset_trim,
  eps_output_check, arm_power_check, grid_power_limit_pi, relays_off_when_disabled,
  grid_loss_watchdog(_kick), init_ac_meas (was init_rating).

### MPPT (mppt_b.c)
- mppt_pv1 / mppt_pv2: the start-up sweep, power averaging over 50 passes, a rescan when the
  power stays low, resync when the reference drifts 30 V (20 V for PV2) from the measured voltage.
  The reference goes to the ISR (0xC00D/0xC008) and is limited to 120 V .. Voc + 2 V.
- mppt_po_pv1 / mppt_po_pv2: perturb and observe steps of 5 V (2.5 V).

## Battery rules found in range B

BMS mode means 0xD8E1 (HR54) is not 0. "BMS active" means HR19 bits 0 and 1 are not both 0.

1. Start permit 0xD52D (batt_voltage_monitor): BMS active and HR22 above HR97 - 2.0 V for
   500 passes (10 s). Not in BMS mode it is always 1. soft_start runs only with the permit and no
   battery fault, or with HR175 latched (0xD90A), or in aging mode 1.
2. Soft start allowed 0xD52E: no battery fault, and the HV side at 220 V or more, or 120 s.
3. Voltage mismatch, CF15 bit 1: in BMS mode, measured and HR22 differ by more than 5.0 V, with BMS
   active, HR22 not 0 and |HR23| under 2.00 A, for 5 checks. Cleared after 500 checks within 3.0 V.
   Under 24.0 V it is set at once. Not in BMS mode: under 20.0 V for 5 sets it, over 30.0 V for 5
   clears it.
4. Under-voltage, CF15 bit 2: below HR97 for 5 checks (not while the BMS is idle). Cleared after
   150 checks above HR97 + 2.0 V (500 with 0xD90A). While fewer than 50 BMS replies have arrived
   and 0xD90A = 1, it is held set.
5. Over-voltage, CF15 bit 6: above max + 2.0 V for 2 checks or max + 1.0 V for 50. The max is
   HR98 in BMS mode, else a fixed 56.0 V. Cleared after 500 checks below max. The counters are
   totals since the last trip, not runs.
6. DC-DC ratio, CF15 bit 4: only in converter mode 5, the HV side more than 50 V away from
   5.5 x Vbatt for 50 checks. It clears itself after 500 passes.
7. Full, CF19 bit 8: BMS active, HR22 not 0, no comms fault, and (HR26 = 0, or SoC >= 100, or
   0xD502 bit 0, or HR20 bit 2) for 1500 checks (30 s). Cleared after 250 checks at SoC < 99. BMS
   HR15 bit 0 clears it outside a calibration. Effect: power command at most 0 (no charge).
8. SoC floor: the level L is HR114 (0xD8C8). In modes 5 and 6 it is raised to HR110 now (0xD8C9).
   The force-charge SoC is L - 3, or 1 when L <= 4. Force charge, CF19 bit 9: SoC at or below
   L - 3 for 250 checks, cleared at L + 1. No discharge, CF19 bit 11: SoC at or below L for 250,
   cleared at L + 4.
9. During a calibration (HR29 != 0) bits 8, 9 and 11 of CF19 are cleared every pass. Without a
   BMS reply (BMS mode) CF19 bits 8, 9, 10, 11 and CF15 bit 8 are cleared. Outside BMS mode only
   bits 8 and 9 are cleared, so bits 10 and 11 keep their last value.
10. EPS latch 0xD4C6 bit 3: either floor bit for 50 passes blocks EPS (0xD7C0 bit 9 cannot be set).
    It is released 30000 passes (10 min) after both bits clear, or at once while there is a
    grid fault.
11. Power blocks (batt_power_blocks), positive = charge: (a) if CF19 bit 9 is set, or BMS mode
    with HR19 bit 2 clear and no calibration and no aging mode, a command of 300 W or less becomes
    +300 W; (b) else CF19 bit 11 makes a negative command 0; (c) else CF19 bit 8 caps it at 0.
    Then HR19 bit 5 (outside a calibration) forces at least 120 W of discharge.
12. Charge power by SoC (soc_power_limits): rated x (1 - 0.095 x (SoC - 90)), clamped from 90 % to
    98 %. In a calibration it is 0.075 per % to 100 %. HR20 bit 2 (full) gives 0 (5 % in a
    calibration). HR20 bit 3 (empty) gives a discharge limit of 10 % of rated + 300 W.
13. Current limits (current_limit_tapers), BMS mode:
    - charge = HR26 x (1 - f), where f is the filtered (48.0 V - V) x 0.0225, from 0 to 0.9;
    - discharge = HR27 x (1 - g), where g is the filtered (V - 54.5 V) x 0.0257, from 0 to 0.9;
    - charge cap = capacity (HR55) x HR111 % + 1.50 A, at most HR309 (0xD400, default 60 A);
    - discharge cap = capacity x HR112 % + 1.50 A, at most HR309 + 15 A;
    - when 0xD502 bits 6/7 are set (or bits 1/3 for discharge), the taper is skipped and the limit is
      min(HR26, 20 % of HR309). 0xD502 is never written, so the taper always applies;
    - without a BMS both limits are 60 A. The final limits are clamped to 1..80 A (charge) and
      2..80 A (discharge).
14. Power limits: min of the SoC limit, the current limit x V (at most rated), the charger
    temperature derate (5 % per degree above 87.0), and the bus derate (above 520 V). The charge
    side is also capped by 0xD4E2, which falls when the PV power exceeds its rating or a PV current
    exceeds 18 A (two PIs).
15. ISR current bounds 0xC212/0xC214 = 10 x power / V_hv. Schedule window 1 sets charge to
    0.25 A; window 2 sets discharge to -0.25 A. HR19 bit 5 (when discharging more than 1 A) ramps the
    charge bound between -0.5 and 0.1 A. The clamps are -0.5..15 A (charge) and -15..0 A
    (discharge), on the HV side.
16. Power command (batt_power_setpoint): mode 0 updates the command only when the ARM setpoint
    changes, to -P_batt - 0.5 x setpoint. It is 0 when HR27 is 0 or CF19 bit 2 is set. Mode 1
    (forced charge: HR175, aging mode 1, or the ARM) is min(rated x HR313 %, capacity x HR111 % x V).
    In BMS mode it stops at the charge target SoC unless BMS HR15 bit 0 is set (1000 W during a
    calibration). Mode 1 ends when HR20 is on, PV is faulted and SoC > HR116. Mode 2 is
    max(-(rated + 300 W) x HR314 %, -capacity x HR112 % x V). In BMS mode it stops at HR110 (or
    -1000 W under 4 % during a calibration). A remote command (0xCE00) overrides the command. The
    magnitude is capped by 0xCF1C.
17. Full-battery current trim (full_current_trim): while CF19 bit 8 is set in BMS mode, a 0..2.0 A
    trim is built from the pack current. It is dropped after 250 checks at -5 A or below.
18. CF15 bit 15 is set by schedule window 3 or HR19 bit 4. CF15 bit 8 is the OR of 0xD500 bits 0,
    3, 5, 8, 12, which nothing sets (see below).

## Open questions

- 0xD214/0xD216: I read them as the DC-DC's HV-side voltage and current (about 5.5 x Vbatt). The
  ARM calls 0xD214 bus_hv_voltage. 0xD212 is the battery voltage, so the DC-DC's turns ratio is
  probably 5.5.
- 0xC208/0xC20A = 560/460 (540 or 500 in aging mode). If these are 0.1 V battery limits for the
  current controller, there is a fixed 56.0 V ceiling whatever HR98 says. Range C's controller
  needs checking.
- HR19 bits 2, 4 and 5 drive force charge, a fault bit and forced discharge. Their BMS meaning
  needs bms-analysis.
- 0xD8EF/0xD8F0: the ARM docs call HR332 "lead acid". On the DSP it starts what looks like a
  burn-in mode (forced charge or discharge, a fixed PV reference of 250 V, bus limits).
- What 0xD3C0.f54 (ARM page 1 bytes 8-11, ARM p_eps_sample) measures. arm_power_check trips on it.
- 0xD232 (channel f16), 0xD3D2 (current2), 0xD8C4, 0xD8D6, 0xD8D8, and the relay mapping of
  0xC034..0xC036.
- 0xD819 (page 6 bytes 28-29): the A316 ARM sends 0 there, so the gain clamps to 870 (0.85). It
  is not used in range B.
- D8C8 (HR114) and D8C9 (HR110) as "floor": the floor is HR114 by default and HR110 in on-grid
  modes. Does HR114 really mean "EPS reserve"?

## Likely bugs and dead code

- ntc_temp_check compares the constant fmax(2437, 2437) with 3350, so CF17 bit 2 is never set.
  ntc_to_temp (the table it would use) is not called. A temperature check was stubbed out.
- Battery over-temperature: temp_check counts 0xD157 when 0xD230 > 0xD8AA (100.0), but nothing
  reads the count, so battery temperature has no effect in range B.
- soc_power_limits computes a discharge taper b = 15 - SoC (0..5) and never uses it.
- 0xD500 bits 0, 3, 5, 8, 12, and 0xD502 and 0xD503, are read but have no writer in the image
  (refs.tsv; bms_parse_hr writes only bits 1 and 2). So CF15 bit 8, CF19 bit 10, the taper bypass,
  has_batt_fault's 0xD502 bit 11 and 0xD520 are dead. This looks like code for another BMS
  protocol.
- 0xD50B, 0xD520, 0xD52B, 0xD52C, 0xD15E and 0xD170 are written and never read.
- state0_idle's board id adds GPIO33 << 2 and GPIO33 << 1. One of them was probably meant to be
  GPIO34.
- soc_floor writes the raised floor into 0xD8C8, the HR114 copy. It stays raised until the next
  ARM page 0 frame rewrites it.
- The batt_voltage_monitor counters are totals, not runs. My earlier Ghidra notes already say this.
- Non-BMS mode: the OV max is a fixed 56.0 V, and the current limits are a fixed 60 A.

## Naming notes

- Existing names I would change: any_fault -> has_batt_fault, fault_hold -> soft_start_permit,
  force_charge_power -> batt_power_blocks, charge_taper_soc -> soc_power_limits,
  forced_charge_discharge -> batt_power_setpoint, init_rating -> init_ac_meas,
  cpu_timer_config -> ConfigCpuTimer (TI), and ctl_stepN to what each step does.
- The b_adc.h, b_epwm.h, b_ecap.h, b_gpio.h and b_sysctrl.h types are TI's with a B_ prefix. The
  TSV gives the TI names. It would be better to include the TI headers (src/include/F2806x_*.h).
- The 0xD3C0 PI blocks should become C's PI_BLK (c_ctrl.h). The 0xD500 page should be merged
  with C's BMS_DATA, and 0xD240 with C's PHASE.
- The collision check found the TI names (XtalOscSel, InitPll, ...) only in comments. PieVectTable
  is TI's object at 0x0D00, the same object.
