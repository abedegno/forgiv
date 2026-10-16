# Control: measurement, monitoring, scheduling

Files: `src/app/control/measure.c`, `monitor.c`, `schedule_p1.c`, `schedule_p3.c`.

## Architecture

Three tasks share the control work:

- `task_measure` (priority 0, each measurement window or 25 ms): the inverter output enable,
  the RMS values of the window, and the battery pack poll (`bms_poll`), which stages FC4
  requests for the DSP and unpacks the replies the DSP relays.
- `task_monitor` (priority 5, 20 ms): LEDs, NTC temperatures, the aux MCU link and the CT
  power it gives, SOC and battery temperature, DRM, the power flow figures, the fault LED and
  the EEPROM save loop.
- `task_schedule` (priority 6, 500 ms): the clock, the charge and discharge decision, the SOC
  targets, the energy counters, the dongle watchdog and the panel indicators.

The ARM does not control charge or discharge currents. It decides three flags
(`charge_active`, `discharge_active`, `hold_active`) and two SOC targets
(`charge_target_soc_now`, `discharge_min_soc_now`) and sends them, with the battery limits
from the settings, to the DSP, which runs the power stage and applies the limits.

## Data

- `bms_modules[6]` (`bms_t`), `bms_packs_online`, the poll state.
- `fault_flags`, `fault_code`, the fault and event logs (never written).
- `heatsink_temp`, `charger_temp`, `batt_temp`, `batt_soc`.
- `ct_power`, `ct_i_rms`, `p_*` power figures, `e_*` energies, `energy_totals` (block 4).
- `charge_active`, `discharge_active`, `hold_active`, the SOC targets, `night_charge_*`.

## Rules found in the code

SOC and battery type:
- Lithium (HR54 = 1): SOC and battery temperature are the DSP's (from the BMS), copied every
  20 ms. Lead acid (HR54 = 0): SOC is linear in voltage between HR97 and HR98 (96 % of HR98
  when not charging), capped at 97 % while charging; 0 when the DSP's battery state is 3 or
  more; 100 on the DSP's "full" byte. Both read 0 at charge status 7 with lead acid.
- The battery type only changes by writing HR54 or HR332. HR58 "auto judge" reaches nothing:
  `batt_auto_judge_type` has no caller.

Charge and discharge (normal mode):
- 10 charge slots and 10 discharge slots, HHMM times, end minute included, start = end means
  off, start > end wraps past midnight. Charging needs HR96 = 1, discharging HR59 = 1; both
  need the inverter on grid (IR49 = 1). A discharge also needs not (HR110 = 100 with HR27 = 1).
  A charge always cancels a discharge.
- SOC targets: the active charge slot's SOC (else HR310) and the active discharge slot's SOC
  (else HR110); HR310 and HR114 in remote mode (HR494 = 1).
- Calibration (HR29 non-zero, lithium only): discharge until the DSP reports "empty", then
  charge until it reports "full"; no timeout in the ARM.
- Night recovery (no calibration): from 01:00 to 04:59, with no charge active, a battery flat
  for 30 s (lithium: SOC 0 or SOC + 10 <= HR114; lead acid: below HR97 - 5.00 V) is charged for
  HR108 minutes, for lithium only until SOC reaches HR114. Once a night.
- Table mode (HR322 = 1): per half hour, discharge when the first table's value is above
  HR323 and HR325, charge when the second table's value is below HR324, else hold. It
  bypasses the slots, the calibration and the night recovery.
- At charge status 7 nothing is charged or discharged.
- HR329 forcing ends by itself after 2 hours with the DSP SOC below 10.

Output, faults, measurement:
- The output is enabled 20 `task_measure` passes after the DSP allows it (frame byte 9) and no
  fault is set; it goes off at once otherwise.
- Fault code = lowest set fault bit; any bit but the aux link one puts the run mode to 3.
- The CT direction is found by itself when HR42 is 2 or 3: a CT power more than 5000 above the
  inverter output for 16 captures flips it.
- DRM: for grid codes 3, 20, 21 (and 2, 8, 12 in another form) the RJ45 inputs limit the
  power to 2, 40, 70 or 100 % and can request DRM0.

Energy counters:
- Every 500 ms each power is integrated; 3,600,000 units make 0.1 kWh (inference: powers in
  0.1 W). PV counts at 96 % of the DSP's PV power. Day counters reset when the RTC date
  changes, and stop at 32700 (PV 65530).
- Block 4 is marked for saving every 20 minutes while running and when the inverter stops
  after running more than a minute. A save of whatever is dirty is requested every 3 minutes,
  and 100 `task_monitor` passes (2 s) after a Modbus write.

The battery pack poll:
- One FC4 request every 500 `task_measure` passes (about 10 s): start 0 (21 registers),
  start 21 (19), start 40 (20) for each device 1-5 in turn. A field is kept only when it is
  in range; nothing checks the replying device, function code or CRC, and nothing times out.
  The full table of fields and ranges is in docs/battery.md, section 2.

## Open questions

- What `g_200002B8` (calc6 energy) and `dsp_p2_d17` measure; both are integrated into
  energy counters.
- Whether `sys_mode` 1 means on grid. I follow the public register list (IR49).
- The units of the CT power. The 5000 threshold and the /0x776 scaling suggest 0.1 W.
- What the table mode's two tables mean. A price table fits (discharge when the first is high,
  charge when the second is low), but the code only shows numbers.
- Whether the PV 96 % factor is an efficiency estimate.
