# Holding and input registers

Version scope: this is the 316 register reference. A319 adds HR104, HR107 and HR192 behavior
and expands meter records. See the [319 reconstruction](../../inverters/gen3-lv/a319/README.md)
and its matched source for those paths; this full table has not been re-audited for 319.

The inverter's Modbus registers as A316 serves them, with what the DSP does with each one that
it receives. Built from the code:

- reads: `modbus_hr_update` and `modbus_ir_update` (HR0-59, IR0-59, in
  [meter.c](../../inverters/gen3-lv/a316/src/app/comm/meter.c)), `modbus_fc03_read_holding`,
  `modbus_fc04_read_input` and the `modbus_fill_*` functions
  ([modbus_slave.c](../../inverters/gen3-lv/a316/src/app/comm/modbus_slave.c));
- writes: `modbus_fc06_write_reg` (FC16 runs the same checks, with two differences, below);
- defaults: `cfg_load_defaults` ([board.c](../../inverters/gen3-lv/a316/src/app/board.c));
- the DSP side: the page handlers ([reference/arm-dsp-link.md](arm-dsp-link.md)).

Names in `code` are the field names in the A316 headers; those of HR0 to HR116 follow the public
GivEnergy register list where the type and use agree (A316 readability.md), the rest are named
by number. "Accepts" is the range FC6 takes; anything else gets the error reply. The meaning is
from the code's use; where the code shows only that a value is stored and sent, I say so.

Frames, slaves and transports are in [operation/comms.md](../operation/comms.md).

## Read ranges

| Function | Range | Content |
|---|---|---|
| FC3 | HR0-59 | `hr_regs`, rebuilt every 25 ms |
| FC3 | HR60-119 | block 1: grid limits, battery settings; HR84-86 one internal word |
| FC3 | HR120-179 | block 1: power-quality settings; HR127 auto test; HR175 |
| FC3 | HR180-239 | block 8 (HR180-186); the device at 0x11 (HR202-231) |
| FC3 | HR240-299 | block 6: the slot times (the slot SOCs are not in this read) |
| FC3 | HR300-359 | block 1 and block 6 (HR300-332, HR351-353), HR329 |
| FC3 | HR360-479 | the two half-hour tables |
| FC3 | HR480-539 | a "remote control" view (below) |
| FC3 | HR4080-4199, HR4340-4399 | calibration gains and offsets |
| FC4 | IR0-59 | `ir_regs`, rebuilt every 25 ms |
| FC4 | IR60-119 | per slave: meters 1-8, 0x30 system, 0x31 battery unit, 0x32-0x36 packs |
| FC4 | IR120-179 | 13 records that nothing writes: always 0 |
| FC4 | IR180-239 | DSP fault words and auto test values |
| FC4 | IR240-299 | RMS values, total PV power, calibration offsets |
| FC22 | 60-119 of slaves 1-8 | meter records |

A read of at most 60 registers that crosses no multiple of 60. Reads outside these ranges use an
uninitialised length (A316 findings item 3).

## Holding registers 0-59 (read)

| HR | Content | Source |
|---|---|---|
| 0 | device type: 0x2001, 0x2002, 0x2003 for model bytes 0x32, 0x2E, 0x24 (5.0, 4.6, 3.6 kW on the DSP) | HR2 low byte |
| 1 | `inverter_module_h`; the DSP takes it as switch bits (page 0) | settings |
| 2 | `inverter_module_l`: high byte the grid code 0-28, low byte the model | settings |
| 3 | `num_mppt_phases` | |
| 4, 5 | `grid_power_max_pv`, 32 bits | DSP page 2 |
| 6 | 3600, fixed | |
| 7 | `enable_ammeter`, the bit mask of meters 1-8 | settings |
| 8-12 | the BMS serial, 10 characters | DSP page 3 |
| 13-17 | the inverter serial, 10 characters | settings |
| 18 | the BMS firmware version (BMS HR13) | DSP page 2 |
| 19 | the DSP version, 316 | DSP page 1 |
| 20 | `enable_charge_target` | settings |
| 21 | the ARM version, 316, fixed | |
| 22 | the USB class plugged in: 2 dongle, 8 stick | |
| 23, 24, 25 | the peek: chip (0 DSP, 1 ARM), address, value | |
| 26 | `grid_port_max_power` | settings |
| 27 | `battery_power_mode` | settings |
| 28 | `enable_60hz_freq_mode` | settings |
| 29 | `soc_force_adjust`, the calibration stage bits | settings |
| 30 | `modbus_address` | settings |
| 31, 32 | two values the code stores (HR31 also in settings) | |
| 33 | `user_code` | settings |
| 34 | 140, fixed | |
| 35-40 | the clock: year, month, day, hour, minute, second | RTC |
| 41 | `enable_drm_rj45` | settings |
| 42 | `enable_reverse_ct_clamp` | settings |
| 43 | settings +0xA2 | settings |
| 44, 45 | discharge slot 2 start, end | settings |
| 46 | the BMS chip version (BMS HR0) | DSP page 2 |
| 47 | `meter_type` | settings |
| 48, 49 | `reverse_115_meter_dir`, `reverse_418_meter_dir` | settings |
| 50 | `active_power_rate` | settings |
| 51 | `reactive_power_rate` | settings |
| 52 | `power_factor` | settings |
| 53 | `inverter_state`, with a second byte in the high byte | settings |
| 54 | `battery_type` | settings |
| 55 | the BMS capacity when the DSP reports one, else `battery_capacity` | DSP page 2 or settings |
| 56, 57 | discharge slot 1 start, end | settings |
| 58 | `enable_auto_judge_battery_type` | settings |
| 59 | `enable_discharge` | settings |

## Holding registers: writes and their effect

Units are those the code implies: 0.1 V for AC voltages, 0.01 Hz for frequencies, 0.01 V for the
battery limits, HHMM for slot times.

### Identity, clock and service

| HR | Accepts | Effect | DSP |
|---|---|---|---|
| 1 | any | `inverter_module_h` | page 0 bytes 15-16: iso check, RCMU, DCI compensation, step checks (each bit inverted) |
| 2 | high byte below 29, low byte up to 0x32 | `inverter_module_l`; reloads the defaults of blocks 1 and 6 and sets `cfg_changed_flags` | page 0 bytes 8-9: grid code, model |
| 7 | 0..255 | the meter mask; bit 0 also sets HR176 bit 3 | |
| 8-10 | none | refused | |
| 13-17 | any | the serial, two characters each; HR17 saves it | |
| 20 | 0..1 | `enable_charge_target` | page 7 byte 31 |
| 23 | 0..1 | the peek chip | |
| 24 | 0xC000..0xE000 for the DSP, any for the ARM | the peek address | page 3 bytes 35-36 |
| 30 | 1..255 | the Modbus address | |
| 31 | 0..4 | stored in the high nibble of settings HR31 | |
| 33 | 0..128 | `user_code` | page 0 byte 32, not used |
| 35-40 | year (0-99 or 2000 on), month, day, hour, minute, second | sets the RTC; HR40 writes it | |
| 162 | 1 | clears the energy counters, the fault and event logs | |
| 163 | 1, 2, 3, 100, 101, 200 | 1 and 2 factory reset of blocks 1 and 6 (2 also the gains), 3 the gains only, others `cfg_reset_mode` | |
| 164 | 1, 0x100 | 1 (not to slave 0): jump to the boot loader; 0x100 stops the DSP link and hangs | |
| 166 | 0..1 | 1 keeps most battery and schedule writes in RAM only | |

### Grid

| HR | Accepts | Effect | DSP |
|---|---|---|---|
| 26 | 0..60000 | `grid_port_max_power`, the export limit (W, inference from the DSP's x 10 to 0.1 W) | page 2 bytes 35-36: x 10, at least 1200; 6000 is sent while HR29 is not 0 |
| 28 | 0..1 | 60 Hz mode; reloads the country limits, adding 10.00 Hz | page 2 byte 37 |
| 41 | 0..1 | DRM on the RJ45 inputs | caps HR50 and replaces HR53 for some codes |
| 50 | 0..100 | `active_power_rate`, % of rated | header byte 3 |
| 51 | 0..60 (FC16: 0..50) | `reactive_power_rate`, % of rated | page 0 byte 18, below 61 |
| 52 | 0..20000 | `power_factor` code | page 0 bytes 19-20 |
| 53 | low byte 0..1, high byte 0 or 1 | `inverter_state`: 0 off, 1 on; forced to 1 at boot | page 0 byte 25; 0 requests run mode 0 |
| 60 | 800..10000 | PV start voltage, 0.1 V | page 0 bytes 12-13, 120.0..150.0 V |
| 61 | 0..600 | start time | page 0 bytes 10-11, `connect_countdown` |
| 62 | 0..900 | restart delay | page 0 bytes 29-30 |
| 63-66, 71-74, 79-82 | anything (`cfg_limit_check` accepts all) | voltage and frequency trip limits, stages 1 ("out"), 2 ("in") and reconnect ("c") | pages 1 and 2; HR63/64/71/72 clamped |
| 67-70, 75-78 | 1..50000 | the trip times | pages 1 and 2 |
| 83 | 0..3000 | the 10-minute mean voltage limit | page 2 bytes 26-27, 230.0..290.0 V |
| 101 | 1..255 | the import limit in A | page 7 byte 17 |
| 102 | 0..1 | HR102 bit 6, the import limit switch | page 7 bytes 26-27 |
| 121 | 0..255 | | page 2 byte 28, `freq_stage_sel` |
| 122 | 0..10 | Q mode | page 0 byte 27 |
| 123 | 5..100 | over-frequency droop | page 2 byte 25 |
| 124 | 0..1 | | page 4 byte 9, `grid_func_flags` bit 4 |
| 125 | 0..3 | | page 0 byte 14, `grid_func_flags` bits 3 and 12 |
| 126 | 0..1 | accepted, not stored | |
| 127 | 1, grid code 4 only | starts the CEI 0-21 auto test | page 1 byte 42 |
| 128 | 0..1 | | page 2 byte 24, not used |
| 129 | 0..1 | stored | |
| 130-137 | 1..300 (even), 1..50000 (odd) | PF curve points | page 3; only the odd ones are taken (D316 findings-control item 1) |
| 138-141 | 0..3000 | Q(U) voltage points | page 3 bytes 24-31 |
| 142, 143 | 0..100 | Q(U) lock-in and lock-out power | page 2 bytes 29-30 |
| 144, 145 | 0..3000 | cos phi(P) lock-in and lock-out voltage | page 2 bytes 31-34 |
| 176 | 0..255 | | page 4 byte 18, `grid_func_flags` bits 15, 6, 8 |
| 177 | 0..1 | | page 4 byte 25, `eps_cfg_bits` bit 10 |
| 178 | 0..1 | export limit on | page 6 byte 38, `grid_func_flags` bit 0 |
| 179 | 0..1 | | page 6 byte 39, `grid_func_flags` bit 2 |
| 300 | 0..30000 | over-frequency delay | page 6 bytes 30-31 |
| 301 | 0..30000 | PT1 time for code 7 | page 6 bytes 32-33 |
| 302 | 10..30000 | ramp rate after a grid fault | page 6 bytes 34-35 |
| 303 | 0..30000 | over-frequency recovery delay | page 6 bytes 36-37 |
| 305 | 0..4 | PV mode | page 6 byte 40, `pv_mode_cfg` |
| 306 | 10..30000 | ramp rate | page 7 bytes 8-9 |
| 307 | 2001..2400 | nominal grid voltage, 0.1 V | page 7 bytes 10-11 |
| 312 | 0..30000 | under-frequency delay | page 7 bytes 18-19 |
| 315, 316 | 0..3200 | momentary cessation voltages | page 7 bytes 22-25 |
| 317 | 0..1 | HR102 bit 0: EPS on (inference from its DSP use) | page 7 bytes 26-27, `eps_cfg_bits` bit 9 |
| 321 | 5020..5200 | over-frequency start | page 3 bytes 33-34 |
| 326 | 4600..5000 | under-frequency start | page 3 bytes 39-40 |
| 327 | 17..100 | under-frequency droop | page 3 byte 41 |
| 328 | 5000..5200 | over-frequency recovery | page 2 bytes 41-42 |

### Battery

| HR | Accepts | Effect | DSP |
|---|---|---|---|
| 27 | 0..1 | `battery_power_mode`: 1 lets the self-consumption loop run | page 0 byte 24 |
| 29 | 0..7, only with HR54 = 1 | calibration stage bits, ORed in; 0 clears | page 2 byte 38 |
| 46 | read only | BMS chip version | |
| 54 | 0..1 | battery type, 1 BMS; 0 also clears HR29 and HR58 and sets HR111 to 20; 1 sets HR111 to 50 | page 0 byte 34 |
| 55, 512 | 0..10000 | capacity, Ah | page 0 bytes 35-36 |
| 58 | 0..1 | auto judge battery type: nothing reads it (dead code) | |
| 97 | 0..65500 | low voltage limit, 0.01 V; reset to 43.20 V at boot | page 1, 20.0..48.0 V |
| 98 | 0..65500 | high voltage limit, 0.01 V; reset to 58.50 V at boot | page 1, 54.0..63.0 V |
| 108 | 0..100 | night recovery charge length, minutes | |
| 109 | 0..100 | 1: the DSP's full BMS poll | page 4 byte 17 |
| 110 | 0..99 | SoC reserve: the discharge stop outside discharge slots | page 4 byte 21 |
| 111, 112 | 0..250 | charge and discharge limit, % | page 4 bytes 19-20, 1..100 |
| 113 | 0..1 | stored | |
| 114 | 0..100 | floor in non-grid modes; remote-mode discharge stop; night recovery threshold | page 0 byte 21 |
| 115 | 0..1 | | page 4 byte 22, `grid_func_flags` bit 11 |
| 116 | 0..100 | charge target SOC | page 7 byte 32 |
| 175 | 0..1, RAM only | low-battery wake and forced charge | page 4 byte 16 |
| 200 | 0..1, RAM only | written to BMS HR3; cleared after 240 passes (2 min) | page 4 byte 27 |
| 201 | 0..1, RAM only | written to BMS HR4; sent in 5 frames | page 5 byte 42 |
| 308 | 501..6000 | battery power rating, W | page 7, x 10 |
| 309 | 10..80 | maximum charge current, A | page 7, x 100 |
| 310 | 20..100 | charge target SOC outside charge slots | page 7 byte 16 |
| 311 | 0..2 | stored | |
| 313, 314, 515 | 1..100 | forced charge and discharge power, % of HR308 (515 is HR314 again) | page 7 bytes 20-21 |
| 329 | 0..1, RAM only | forced discharge to 4 %, ends after 2 h below 10 % | header byte 4 |
| 332 | 0..1 | 1: lead acid on the ARM (HR54 0, 125 Ah, HR305 1) and the DSP's aging test | page 2 byte 39 |
| 494 | any, stored as a byte | 1: remote mode | header bytes 5-6 |
| 495 | any, RAM only | the remote setpoint | header bytes 5-6 |

### Schedules

| HR | Accepts | Effect |
|---|---|---|
| 56, 57; 44, 45 | 0..2400 | discharge slots 1 and 2 |
| 59 | 0..1 | discharge slots on |
| 94, 95 | 0..2400 | charge slot 1 |
| 96 | 0..1 | charge slots on |
| 230 | 0..1 | 1: the device at 0x11 gets 36000 |
| 242 | 0..100 | charge slot 1 SOC |
| 243-269 | times 0..2400, SOCs 0..100 | charge slots 2-10: start, end, SOC |
| 272, 275 | 0..100 | discharge slot 1 and 2 SOC |
| 276-299 | times 0..2400, SOCs 0..100 | discharge slots 3-10: start, end, SOC |
| 318 | 0..3 | the schedule window value ([energy-modes.md](../operation/energy-modes.md#the-schedule-window)) |
| 319, 320 | 0..2400 | the schedule window start and end |
| 322 | 0..1 | table mode |
| 323, 324, 325 | below 20000 | table mode thresholds |
| 360-407, 420-467 | below 20000, RAM only | the two half-hour tables |

The slot SOCs go to the DSP as the "now" values (page 4 byte 21, page 7 byte 16), not one by one.

### RAM-only copies and others

| HR | Accepts | Effect |
|---|---|---|
| 180-186 | any | block 8, raw words |
| 330, 331 | 0..255 | stored |
| 351 | 0..1 | 1 hides the DSP's warning word and its CF15 fault bytes on the ARM; IR0 then does not show 2 for charge status 6 |
| 352 | 0..1 | stored |
| 353 | below 65535 | stored (default 12000) |
| 481-490 | as HR27, 29, 56, 57, 59, 94, 95, 96, 50, 53 | the same settings, not saved to EEPROM |
| 4000 | any, RAM only | sent on page 7; I found no reader on the DSP |
| 4097-4385 | gains change only through 1.0 (0x400), offsets only through 0 | calibration gains and offsets; some go to the DSP on pages 5 and 6 |
| 4102-4144 | any | energy counters |

Cases 167-171, 240, 241, 270, 271, 273, 274, 304 and 408-417 are accepted and ignored. HR167 can
only be written with FC16 (A316 findings item 17).

### The remote-control view (HR480-539, read)

`modbus_fc03_read_holding` serves a block that gathers the values a remote controller needs: the
run mode, HR27, HR29, the slot 1 times and enables, HR50, HR53, the SOC, the battery power, the
load, HR494, HR495, the inverter power, BMS information, the warning word, bus voltages, the
charge status, the mode, the battery voltage, the capacity, the two SOC targets now, HR314, the
model and HR311.

## Input registers 0-59

From `modbus_ir_update`. "DSP" means the value comes from a DSP page; units are those the DSP
sends ([arm-dsp-link.md](arm-dsp-link.md)).

| IR | Content | Source |
|---|---|---|
| 0 | status: the run mode; 2 while the meter fault is set, or at charge status 6 with HR351 = 0 | DSP header byte 1 |
| 1, 2 | PV1, PV2 voltage | DSP page 0 |
| 3 | DC bus voltage | DSP page 1 |
| 4 | `v_n_bus` | |
| 5 | grid voltage | `v_ac1` |
| 6, 7 | battery charge plus discharge energy, total | `energy_totals` |
| 8, 9 | PV1, PV2 current / 10 | DSP page 0 |
| 10 | grid current / 10 | DSP page 1 |
| 11, 12 | PV energy total (with meters 2 and 3) | `energy_totals` |
| 13 | grid frequency (from meter 1 when present: A316 findings item 15) | `f_ac1` |
| 14 | charge status 0-7 | DSP header byte 2 |
| 15 | high-side bus voltage | DSP page 1 |
| 16 | inverter output power factor | |
| 17, 19 | PV1, PV2 energy today | `energy_totals` |
| 18, 20 | PV1, PV2 power / 10 | DSP pages 0, 2 |
| 21, 22 | grid export energy total | |
| 24 | inverter output power / 10 | |
| 25, 26 | grid export, import energy today | |
| 27, 28 | inverter input energy total | |
| 30 | grid power / 10 | |
| 31 | EPS power / 10 | |
| 32, 33 | grid import energy total | |
| 35 | inverter input energy today | |
| 36, 37 | battery charge, discharge energy today | |
| 38 | the start countdown | DSP page 0 |
| 39, 40 | the 32-bit fault word | DSP header bytes 3-6 and ARM bits 6-11 |
| 41 | heatsink temperature | ARM NTC |
| 42 | load power / 10 | |
| 43 | grid apparent power / 10 | |
| 44 | inverter output energy today | |
| 45, 46 | inverter output energy total | |
| 47, 48 | work time, hours | |
| 49 | system mode: 1 when the DSP has no grid fault | DSP page 0 byte 28 |
| 50 | battery voltage, 0.01 V | DSP page 0 |
| 51 | battery current, 0.01 A | DSP page 0 |
| 52 | battery power | `p_battery` |
| 53, 54 | EPS voltage, frequency | DSP page 1 |
| 55 | charger temperature | ARM NTC |
| 56 | battery temperature: the DSP's (BMS) with HR54 = 1, the ARM's NTC with HR54 = 0 | DSP page 0 or ARM |
| 57 | the warning word; 1 while the meter fault is set | DSP header bytes 7-8 |
| 58 | grid port current (from meter 1 when present: A316 findings item 15) | `i_grid_port` |
| 59 | SOC, % | DSP page 0 (HR54 = 1), or from the voltage |
| 23, 29, 34 | 0 | |

## Input registers 60-119, by slave

| Slave | Content | Function |
|---|---|---|
| 1-8 (meters) | IR60 voltage, IR63 frequency, IR68 active power / 10, IR72 apparent power / 10, IR76 another value / 10, IR80 PF, IR84 current, IR85-88 import and export energy | `modbus_fill_meter_ir` |
| 0x30 (system) | IR99: packs online (high byte), 16 (low byte) | `modbus_fill_slave30_regs` |
| 0x31 (battery unit) | IR62, IR63: the BMS charge and discharge limits in A (DSP page 5) | `modbus_fill_bcu_regs` |
| 0x32-0x36 (packs 1-5) | IR60-75 cells (mV), IR76-79 and IR81 temperatures, IR80 pack voltage, IR82-89 voltage and capacities, IR90-94 status, IR95 current (charge only), IR96 cycles, IR97 cell count, IR98 BMS firmware, IR100 SOC, IR101-104, IR110-114 serial | `modbus_fill_battery_regs`, from the ARM's FC4 pack poll |

The pack fields and their range checks are in [A316 battery.md](../../inverters/gen3-lv/a316/docs/battery.md).

## Input registers 180-299

| IR | Content |
|---|---|
| 217-224 | CF12 to CF19, each with its bytes swapped ([fault-bits.md](fault-bits.md)) |
| 225 onwards | the auto test state, step, values and result (DSP page 4); IR227 = 0x012C |
| 240 | RMS of the ARM's ADC channel 0 |
| 246 | the channel 5 current |
| 247, 248 | total PV power / 10: PV1 + PV2 + meter 2 + meter 3 |
| 258-260, 265-267 | calibration offsets |

## Holding registers the DSP reads that the ARM does not name

Bytes of the settings pages that come from internal ARM values rather than registers (for
example `cfg_changed_flags`, `ct_cycle_count`, `p_eps_sample`, the country table bytes) are in
[arm-dsp-link.md](arm-dsp-link.md).
