# How A316 handles the battery

This is a read of the Hybrid Gen3 LV ARM firmware A316 (byte-exact sources in `src/`) for one
question: what does the ARM do with the battery, and what must a replacement BMS satisfy as far as
the ARM is concerned. I wrote it before the DSP firmware D316 (TI C2000) was decompiled; the
DSP's side is now in [../../d316/docs/battery.md](../../d316/docs/battery.md). Where I rely on
the DSP analysis in open-giv `bms-analysis` (docs/05-inverter-firmware.md, Ghidra and emulation,
not byte-exact), I say so. Where I infer rather than read, I say "inference".

Addresses are flash addresses of the A316 image (loaded at 0x08014000) or RAM addresses. Settings
offsets are from `cfg_settings` at 0x20001AAE (EEPROM block 1).

## 1. The data path, end to end

```
 BMS (RS485, 9600 8N1, Modbus RTU, GivEnergy FC4 framing)
   |   the DSP is the bus master: FC3 HR poll of device 1 every 240 ms (DSP's own),
   |   plus the FC4 reads the ARM asks for (devices 1..5)
   v
 DSP (D316)  -- 0x44 frame, 45 bytes, UART4, 9600 -->  ARM (A316)
   ^            page 0: battery V/I/power/SOC/temp          |
   |            page 2: "full" flag, BMS capacity, versions |
   |            page 4: BMS-present word, DSP fault words   |
   |            page 5: raw FC4 reply, 5 x 20-byte sub-pages|
   |                                                        |
   +--- 0x43 frame, 45 bytes, UART4 <-- ARM: run mode, power rate, charge/discharge state,
                                            battery power setpoint, 8 rotating settings pages
                                            (page 6 carries the ARM's FC4 request)
 ARM -> Modbus RTU slave (RS485 USART3, and USB CDC to the dongle at 57600)
        IR0-59 (battery V, I, P, temperature, SOC), IR60-119 of slaves 0x32-0x36 (per pack),
        HR registers (settings)
```

### BMS to DSP

The ARM never touches the BMS bus. It has no UART for it: USART1 is the auxiliary MCU, USART2 the
meter bus, USART3 the Modbus slave, UART4 the DSP (N1 notes). All BMS frames on the wire come from
the DSP. The DSP's own FC3 poll of HR0-HR27 (or HR17-HR25 when HR109 is not 1) is invisible in this
image apart from the values the DSP passes on (bms-analysis 05).

### DSP to ARM (`task_dsp_rx` 0x080171DA, `dsp_parse_page` 0x080153CC)

`task_dsp_rx` collects a 45-byte frame that starts 0x44, checks the CRC16/MODBUS over 43 bytes,
then takes:

- byte 1 run mode (`inv_run_mode`), byte 2 charge status (`charge_status`), bytes 3-6 a fault word,
  bytes 7-8 a warning word (`dsp_warning_word`, served as IR57);
- one telemetry page from byte 12 (`dsp_parse_page`).

The battery fields per page:

| Page | Field | Global | Use in the ARM |
|---|---|---|---|
| 0 | SOC byte | `batt_soc_dsp` 0x200002E3 | IR59 when HR54 = 1 |
| 0 | battery voltage, 0.01 V | `batt_voltage` 0x200002DC | IR50 |
| 0 | battery current, 0.01 A | `batt_current` 0x200002DE | IR51 |
| 0 | battery power /10 | `p_battery_dsp` 0x200002E0 | energy counters |
| 0 | battery temperature | `batt_temp_dsp` 0x200002E6 | IR56 when HR54 = 1 |
| 2 | "full" byte | `batt_full_flag` 0x20000313 | forces SOC 100 in lead-acid mode |
| 2 | capacity word | 0x200002E4 | HR55 reads back this when non-zero (inference: the BMS's HR11) |
| 2 | two version words | `bms_chip_version` 0x2000039A (HR46), `bms_fw_version` 0x200003A2 (HR18) | battery firmware relay gate |
| 3 | battery state byte | `batt_state_dsp` 0x200002DB | >= 3 forces SOC 0 in lead-acid mode |
| 4 | BMS-present word | `bms_present` 0x200003AE | only read by dead code (below) |
| 4 | DSP fault words CF12-CF19, 16 bytes (10 here, 6 in page 6) | `dsp_fault_words` 0x2000236A | IR217-224 |
| 5 | raw FC4 reply | `dsp_bms_raw` + 2 = 0x2000070C | parsed by `bms_poll` |
| 5 | bytes 0x23/0x24: the BMS charge and discharge current limits (BMS HR26/HR27) in A | `bms_charge_limit_a`, `bms_discharge_limit_a` | IR62/IR63 of slave 0x31 |

Page 5: byte 1 is a "BMS data valid" byte; when it is 1, byte 2 is a sub-page number 0..4 and the
next 20 bytes go to `dsp_bms_raw + 2 + 20 * subpage`. Sub-page 4 sets the "reply ready" byte
(`dsp_bms_raw[0x99]`, 0x200007A3). So the DSP hands over up to 100 bytes of the raw BMS reply,
without its own interpretation. The ARM does not check the CRC of the BMS reply; the DSP has done
that (bms-analysis 05: the DSP accepts an FC4 reply only when it is `(count + 3) * 2` bytes long
with a good CRC).

A correction to the N1 notes: the page 5 bytes are unpacked by `bms_poll` (0x08026C40), not by
0x08026BA4, which is `inv_output_enable_ctrl`.

### ARM to DSP (`task_dsp_tx` 0x08016FDC, `dsp_build_page` 0x08015D02)

Each 0x43 frame carries:

- byte 1: run mode (or 0x0A in one grid-code case);
- byte 2: `fault_flags >> 4`;
- byte 3: active power rate HR50, capped by the DRM limit for Australian codes;
- byte 4: a state, 2 if HR329 forcing is active, else 1 if `charge_active` or HR175, else 2 if
  `discharge_active`, else 0;
- bytes 5-6: the battery power setpoint, HR495 when `cfg_block6[0x61]` (HR494) is exactly 1, else
  `-batt_power_calc / 10`, where `batt_power_calc` is the CT or meter power (inference: grid
  power, so the default is a self-consumption setpoint);
- from byte 7: one of eight rotating pages, then CRC16 (high byte first).

The battery settings on the pages:

| Page | Battery-relevant content |
|---|---|
| 0 | HR60 PV start voltage (155.0 V instead while HR175 is 1), HR54 battery type, HR55 capacity, HR114, `charge_active`, `discharge_active`, HR27 (or 1 while `hold_active`), charger, heatsink and battery temperatures |
| 1 | HR97 and HR98 (battery low and high voltage limits, 0.01 V) |
| 2 | HR26 (6000 instead while HR29 is non-zero), HR29 (calibration stage) |
| 4 | HR109, HR176, HR111, HR112, the discharge stop SOC now (4 instead while HR329 forcing is active), HR115, HR177, HR175, `bms_update_pending` |
| 6 | the ARM's FC4 request (7 bytes and a "request" flag) |
| 7 | the charge target SOC now, HR116 |

### ARM to Modbus and the cloud

The Modbus slave (`modbus_slave_process` 0x08022C50) serves the same tables on RS485 (USART3) and
on the USB CDC link to the dongle (N3 notes), so the app, the cloud and GivTCP all read the same
registers. Battery data appears in IR0-59 (`modbus_ir_update` 0x0802D0EA), in IR60-119 of slaves
0x32-0x36 (`modbus_fill_battery_regs` 0x0801FA04, one per pack, slave = 0x31 + device), in IR60-119
of slave 0x31 (`modbus_fill_bcu_regs`: IR62/IR63 the BMS charge and discharge limits in A, from DSP page 5) and slave 0x30 (IR99 high byte =
`bms_packs_online`, low byte = 16).

## 2. The pack poll (`bms_poll`, 0x08026C40)

### Where it runs

`bms_poll` is called once per pass of `task_measure` (0x0802735A), the priority-0 task. That task
wakes on its 25-tick period or on event flag 2, which the ADC interrupt posts at the end of each
mains cycle (0x0801432C area). With a 1 ms SysTick (`SysTick_Config(SystemCoreClock / 1000)`),
that is about every 20 ms on a 50 Hz grid (inference from the wake sources). A request goes out on
every 500th call, so about every 10 s, which matches the captures (one FC4 poll about every 10 s).

### The request

The poll state (initial RAM values, read from the image's RW data): `bms_poll_timer` = 0,
`bms_poll_addr` = 1, `bms_poll_step` = 0. On each 500th call the step advances:

| step | start | count | then |
|---|---|---|---|
| 0 | 0 (0x00) | 21 | step 1 |
| 1 | 21 (0x15) | 19 | step 2 |
| 2 | 40 (0x28) | 20 | step 3 |
| 3 | 0 (0x00) | 21 | address + 1 (5 wraps to 1), step 0 |

So after the first pass each device gets block 1 twice, then blocks 2 and 3: 0, 0, 21, 40, about
10 s apart, then the next device. A full sweep of five devices is 20 polls, about 200 s. This is
exactly the order seen on the wire (bms-analysis 03).

The request is staged in `dsp_bms_raw` (0x2000070A): bytes +0x84..+0x89 (0x2000078E..93) hold
device, 4, start (big-endian), count (big-endian); +0x8A is never written by `bms_poll` and is 0.
The flag at +0x98 (0x200007A2) is set to 1. Only if that flag is already 0: a request that has not
been sent yet is not overwritten, but the step still advances, so that block is skipped.
`dsp_build_page` page 6 copies the flag and the 7 bytes into the 0x43 frame and clears the flag.
So one request is sent once, inside one in every eight ARM-to-DSP frames. The ARM adds no CRC;
the DSP builds the Modbus frame (inference; bms-analysis 05 says the DSP sends the request and caps
FC4 counts at 26).

### The reply

When the reply-ready byte is 1, `bms_poll` clears it and reads the 0x82-byte receive area
`rx` = 0x2000070C:

- `rx[0]` must be 1..5. The pack index is `rx[0] - 1`. The ARM does not check that this matches
  the device it asked, and it does not check `rx[1]` (the function code).
- `rx[2..3]` (big-endian) is taken as the start register: this is GivEnergy's non-standard FC4
  reply, which echoes the start address where Modbus has a byte count. Values 0, 21 and 40 are
  parsed; anything else is ignored.
- Data start at `rx[4]`. After parsing, the 0x82 bytes are cleared.

Each pack's record is 0x83 bytes at `bms_modules` + 0x83 * index (0x200007A4). A value outside its
range is not stored; the record keeps the last good value. No fault, no count, no log.

Block 1 (start 0, 21 registers, 42 data bytes):

| Data offset | Field | Check | Record | Served as (slave 0x31 + device) |
|---|---|---|---|---|
| 0-20 | serial (ASCII) | none | +0x02, 21 bytes | IR110-114: the first 10 bytes |
| 22-23 | temperature 1, 0.1 degrees C signed | -300 < v < 700 | +0x79 | IR76 |
| 24-25 | temperature 2 | same | +0x7B | IR77 |
| 26-27 | temperature 3 | same | +0x7D | IR78 |
| 28-29 | temperature 4 | same | +0x7F | IR79 |
| 30-31 | temperature 5 (BMS) | same | +0x81 | IR81 |
| 32-33 | status word | none | +0x00 | bit 0 counts the pack in `bms_packs_online` |
| 34-41 | | ignored | | |

Block 2 (start 21, 19 registers, 38 data bytes; the one-byte cell count shifts later fields by a
byte):

| Data offset | Field | Check | Record | Served as |
|---|---|---|---|---|
| 0 | cell count | none | +0x2C | IR97 |
| 1-2 | cycles | none | +0x2D | IR96 |
| 3-6 | u32 (a voltage in mV) | 0 <= v < 60000 | +0x2F | IR82/83 |
| 7-8 | pack voltage, mV | v < 60000 | +0x33 | IR80 |
| 9-12 | current, mA, s32 | 0 <= v < 80000 | +0x35 | IR95 = v / 10 (0.01 A) |
| 13-16 | capacity (calibrated), 0.01 Ah | v < 200000 | +0x39 | IR84/85 |
| 17-20 | design capacity, 0.01 Ah | v < 200000 | +0x3D | IR86/87 and again IR101/102 |
| 21-24 | remaining capacity, 0.01 Ah | v < 200000 | +0x41 | IR88/89 |
| 25 | SOC, % | v <= 100 | +0x45 | IR100 |
| 26-34 | status bytes | none | +0x46..+0x4E | IR90-94 (IR93 is byte 32 alone) |
| 35-36 | BMS firmware version | none | +0x77 | IR98 |

The current check rejects every negative value, so a discharge current is never stored: IR95 shows
the last charge current (or 0) while the battery discharges. Charge currents of 80 A or more are
also dropped. This is the inverter's behaviour with a real GivEnergy battery too.

Block 3 (start 40, 20 registers, 40 data bytes):

| Data offset | Field | Check | Record | Served as |
|---|---|---|---|---|
| 0-31 | 16 cell voltages, mV | 2200 < v < 3700 | +0x53, 16 words | IR60-75 |
| 32-33 | highest temperature, 0.1 degrees C | -300 < v < 700 | +0x73 | IR103 |
| 34-35 | lowest temperature, 0.1 degrees C | **-200** < v < 700 | +0x75 | IR104 |
| 36-37 | highest cell, mV | 0 <= v < **700** | +0x4F | not served |
| 38-39 | lowest cell, mV | 0 <= v < 700 | +0x51 | not served |

Two oddities in block 3: the lowest temperature has a floor of -20.0 degrees C where every other
temperature has -30.0 degrees C, and the highest and lowest cell voltages are checked against 700, a
temperature range, so a real millivolt value never passes. Neither field is served or used, so this
looks like a copy-paste slip with no effect (inference).

The cell loop is fixed at 16. There is no handling of another cell count in the ARM.

### Timeouts and no answer

There is none in the ARM. If no reply arrives, nothing happens: the next request goes out 500 calls
later, the record keeps its old values, and `bms_packs_online` keeps counting a pack whose last
block 1 had status bit 0 set. Records start at zero after reset (`bms_modules` is zero-init RAM).
Battery-lost handling (about 30 s without a valid HR reply: limits and SOC to zero, comms fault)
is in the DSP (bms-analysis 05).

An absent pack answered by a real battery with its "empty slot" pattern gives temperatures of
-273.0 degrees C (dropped by the range check), zero cells (dropped), zeros for the rest of block 2
(stored) and a status word of 0 (not counted). A pack that is not answered at all keeps a zeroed
record.

### What the ARM does with the pack data

Nothing beyond showing it. `bms_modules` is read only by `bms_poll` (the online count) and
`modbus_fill_battery_regs`. No control decision in the ARM uses a cell voltage, a pack temperature,
the block 2 SOC or a capacity from the pack poll. Control uses the DSP's page 0 values (from the
DSP's HR poll).

The full IR60-119 map of slaves 0x32-0x36 (`modbus_fill_battery_regs`): IR60-75 cells, IR76-79
temperatures 1-4, IR80 pack voltage (mV), IR81 temperature 5, IR82/83 the block 2 u32 voltage,
IR84/85 calibrated capacity, IR86/87 design capacity, IR88/89 remaining capacity, IR90-94 status
bytes, IR95 current (0.01 A, charge only), IR96 cycles, IR97 cell count, IR98 BMS firmware, IR99
0, IR100 SOC, IR101/102 design capacity, IR103/104 highest and lowest temperature, IR105/106 two
words of `energy_totals` (not from the BMS), IR107-109 0, IR110-114 serial (10 characters),
IR115-119 0.

## 3. Battery registers the ARM handles

Holding registers, from `modbus_fc06_write_reg` (0x0801C160; `modbus_fc16_write_regs` at
0x08018816 has the same checks) and defaults from `cfg_load_defaults` (0x08024064). A write
outside the range is refused with an error reply. Names in quotes are the givenergy-modbus names
where I am confident of them; otherwise I describe what the code does.

| HR | Store | Accepted | Default | What the ARM does with it |
|---|---|---|---|---|
| 26 | +0x7B | 0..60000 | 6000 | to the DSP, page 2; replaced by 6000 while HR29 is non-zero (N1: grid port max power, W) |
| 27 | +0x7D | 0..1 | 1 | "battery_power_mode"; page 0 (sent as 1 while `hold_active`); gates the export command to the address-0x11 device |
| 29 | +0x91 | 0..7, only when HR54 = 1 | 0 | calibration stage, bits ORed in; see section 5 |
| 46 | (from DSP) | read only | | BMS chip version (likely); a battery image is relayed only when non-zero |
| 54 | +0x78 | 0..1 | 1 | "battery_type", 1 lithium (BMS), 0 lead acid. Writing 0 also clears HR29 and HR58 and sets HR111 to 20; writing 1 sets HR111 to 50. Page 0 |
| 55 | +0x79 | 0..10000 | 100 | "battery_capacity", Ah; page 0. Reads back as the DSP's capacity word when that is non-zero (`modbus_hr_update` 0x0802CE2E) |
| 56/57, 44/45 | +0x83.. | HHMM <= 2400 | 0 | discharge slots 1 and 2 (more in `cfg_block6`) |
| 58 | +0xA3 | 0..1 | 1 | "enable_auto_judge_battery_type". The function that reads it, `batt_auto_judge_type` (0x08029A3A), has no caller and no pointer in the image: dead code. So the type never changes by itself |
| 59 | +0x82 | 0..1 | 0 | enable discharge slots |
| 60 | +0x3B | 800..10000 | 1300 | PV start voltage, 0.1 V; replaced by 1550 while HR175 = 1 |
| 94/95 | +0x8C/+0x8E | HHMM | 0 | charge slot 1 |
| 96 | +0x8B | 0..1 | 0 | enable charge slots |
| 97 | +0x7E | 0..0xFFDC | 4320 | battery low voltage limit, 0.01 V; page 1; lead-acid SOC zero point; night recovery threshold (lead acid). No clamp in the ARM |
| 98 | +0x80 | 0..0xFFDC | 5720 | battery high voltage limit, 0.01 V; page 1; lead-acid SOC full point. No clamp in the ARM |
| 108 | +0x97 | 0..100 | 6 | night recovery charge length (minutes, inference from the 120-pass multiplier in a 500 ms task) |
| 109 | +0x98 | 0..100 | 1 | page 4; on the DSP side, 1 selects the full HR0-27 poll (bms-analysis 05) |
| 110 | +0x96 | 0..99 | 4 | "battery_soc_reserve": the discharge stop SOC outside discharge slots (`sched_slot_soc_targets` 0x08027A9E), sent on page 4 |
| 111 | +0x99 | 0..250 | 50 | "battery_charge_limit", %; page 4 |
| 112 | +0x9A | 0..250 | 50 | "battery_discharge_limit", %; page 4 |
| 113 | +0x77 | 0..1 | 0 | not battery as far as I can see |
| 114 | +0x9C | 0..100 | 4 | page 0; the LED "reserve" blink (`led_status_update`, SOC <= HR114 + 1); the lithium night-recovery threshold; the discharge stop SOC in remote mode (HR494 = 1) |
| 115 | +0x9D | 0..1 | 1 | page 4 (meaning on the DSP side unknown) |
| 116 | +0xA5 | 0..100 | 4 | page 7 (meaning on the DSP side unknown) |
| 175 | `batt_cal_request` (RAM) | 0..1 | 0 | see section 5 |
| 176 | +0x9B | 0..255 | 7 | page 4; bit 3 follows HR7 bit 0 |
| 177 | +0x9E | 0..1 | 0 | page 4 |
| 310 | +0xBA | 20..100 | 100 | charge target SOC outside charge slots, sent on page 7 |
| 332 | +0xB1, +0xD2 | 0..1 | | 1 switches to lead acid (HR54 = 0, HR55 = 125 Ah); 0 to lithium |
| 494 | `cfg_block6` +0x61 | any | 0 | stored as written; remote mode only when exactly 1 |
| 495 | `batt_power_remote` (RAM) | any | 0 | the battery power setpoint in remote mode, signed 16-bit, same unit as the default setpoint |

I found no cross-checks between HR97 and HR98, between HR110 and HR114, or between HR111/HR112 and
the capacity. The "clamped to 54.0-63.0 V and 20.0-48.0 V" behaviour bms-analysis reports for
HR98/HR97 is not in this ARM image: the FC6 handler stores any value up to 0xFFDC and page 1 sends
it as stored. So the clamp, if it exists, is in the DSP.

But a written HR97/HR98 does not survive a restart. At boot, `cfg_load_all` sets HR97 = 43.20 V and
HR98 = 58.50 V whenever the settings version byte (+0xA6) is at most 0x14, and it sets that byte to
0x14 itself. No Modbus write or other code in this image raises it, so the reset runs at every boot.
The exception is an EEPROM whose byte another firmware version set higher (docs/findings.md, bug 7).
For a BMS swap: the inverter starts each boot with 43.20/58.50 V, whatever was written before.

Input registers (`modbus_ir_update`): IR49 system mode, IR50 battery voltage (0.01 V, from the
DSP), IR51 battery current (0.01 A, from the DSP), IR52 battery power (`p_battery`), IR56 battery
temperature (the DSP's value with HR54 = 1, the inverter's own NTC with HR54 = 0), IR57 the DSP
warning word (but forced to 1 while the byte at 0x20000396 is non-zero; it is named
`board_init_done`, yet the meter code also sets and clears it, so I do not know how often that
override applies), IR59 SOC. The battery charge and
discharge energy counters come from `energy_accumulate` (0x0802752C), which integrates
`p_battery_dsp`.

## 4. SOC

There are two SOCs from the battery, and the ARM uses only one of them.

- **The DSP's SOC** (`batt_soc_dsp`, page 0). With HR54 = 1, `task_monitor` (0x08026426) copies it
  to `batt_soc` (IR59) every 20 ms, and the battery temperature from page 0 to IR56. Per
  bms-analysis 05 this is the low byte of the BMS's HR21. This SOC drives everything SOC-based on
  the ARM side: the LED reserve blink, night recovery, and the app's SOC.
- **The pack poll's block 2 SOC** (offset 25). Stored per pack and served as IR100 of slave 0x32+.
  Display only.

With HR54 = 0 (lead acid), `batt_soc_from_voltage` (0x080258D6): SOC = (V - HR97) * 100 /
(top - HR97), where top is HR98 while charge status is 2 or 3 and 96 % of HR98 otherwise; capped
at 97 % while charging and 100 % otherwise; 0 below HR97 or when the DSP's battery state byte is
3 or more; 100 when the DSP's "full" byte is 1. With charge status 7 and HR54 = 0, SOC and
temperature are 0.

The SOC targets the ARM sends to the DSP (`sched_slot_soc_targets`, every 500 ms): the charge
target is the active charge slot's SOC, else HR310 (default 100); the discharge stop SOC is the
active discharge slot's SOC, else HR110 (default 4). In remote mode (HR494 = 1) they are HR310
and HR114. The SOC floor itself (block discharge, forced charge) is applied by the DSP
(bms-analysis 05).

## 5. Protections and limits: ARM versus DSP

What the ARM does itself:

- **Calibration sequencing** (HR29). In `sched_charge_discharge_eval` (0x080299D0), with HR29
  non-zero and the system running: bit 1 clear means `discharge_active`; bit 1 set and bit 2 clear
  means `charge_active`. In `task_dsp_rx`, on each good frame while HR29 is non-zero: DSP warning
  bit 0x800 sets HR29 bit 1 (the "empty" end, start charging); otherwise bit 0x400 or bit 0x002
  sets bit 2 (the "full" end); HR29 >= 7 clears it to 0 (done). Each change is saved to EEPROM.
  Inference, consistent with bms-analysis: 0x800 is the BMS's HR20 bit 3 ("empty"), 0x400 is
  HR20 bit 2 ("full"), and 0x002 is the DSP's battery over-voltage trip. Nothing in the ARM times
  a calibration out. HR29 is only accepted while HR54 = 1. While HR29 is non-zero, HR26 is sent
  as 6000 and night recovery is off.
- **Night recovery charge** (`batt_night_recovery_charge` 0x08029548). Between 01:00 and 04:59,
  with the system running and no charge active: lithium, if SOC is 0 or SOC + 10 <= HR114, for 60
  passes; lead acid, if V < HR97 - 5.00 V. It then raises `charge_active` for HR108 x 120 passes,
  and for lithium stops as soon as SOC >= HR114. With the defaults (HR114 = 4) this only fires at
  a reported SOC of 0.
- **HR175** (`batt_cal_request`). While 1: PV start voltage sent as 155.0 V, and the frame's
  state byte says "charge". It clears itself in `dsp_build_page` page 4 after more than 300
  consecutive page-4 frames with `batt_voltage` > 540 and `batt_current` > 1000. In the units of
  those globals (0.01 V and 0.01 A) that is 5.40 V and 10.00 A, so in practice "charging at more
  than 10 A for 300 page-4 frames". (The N1 and N2 notes read the voltage as 54.0 V; with
  `batt_voltage` in 0.01 V it is 5.40 V.)
- **The pack poll's range checks** (section 2): they only filter what is displayed.
- **Write ranges** on HR registers (section 3).

What the ARM leaves to the DSP (it only passes settings on): current limits (HR111/HR112 and the
BMS's HR25/26/27), the voltage trips (HR97/HR98), the SOC floor and forced charge, the charge
taper by SOC, the "battery full" block, the meaning of HR19/HR20 bits, battery-lost detection, the
start permit, and the battery-to-DSP voltage mismatch check. None of these is in this image; for
their behaviour see bms-analysis 05.

## 6. What a replacement BMS must satisfy, as far as the ARM is concerned

For the pack poll (all of it is display and cloud data):

1. Answer FC4 on RS485 at 9600 8N1 as device 1, for start 0 count 21, start 21 count 19 and start
   40 count 20, with GivEnergy's reply header `[device][0x04][start hi][start lo]`, then
   `count * 2` data bytes, then the CRC. The ARM keys the parse on the echoed start (0, 21, 40)
   and the device byte (1..5).
2. Block 1 data offset 32-33: a word with bit 0 set, or the pack is not counted in
   `bms_packs_online` (slave 0x30 IR99).
3. Temperatures in 0.1 degrees C, signed, strictly between -30.0 and 70.0 degrees C (block 3 lowest temperature:
   strictly above -20.0 degrees C).
4. Exactly 16 cell voltages in mV, strictly between 2200 and 3700.
5. Capacities in 0.01 Ah as u32 below 200000; SOC 0..100; the two voltage fields below 60000 mV.
6. Current in mA as s32. Only 0 to 79999 is stored; negative values are dropped by the ARM
   whatever the battery does, so this is not something a BMS can fix.
7. Devices 2..5: either the real battery's empty-slot pattern, or no answer. The ARM copes with
   both. The empty pattern stores zeros and leaves the pack uncounted.
8. The serial in block 1 offset 0: the ARM shows the first 10 characters.

For control (through the DSP):

9. The ARM takes its SOC (IR59) and battery temperature (IR56) from the DSP's page 0, that is from
   the DSP's HR poll, not from block 2. Keep HR21 and the block 2 SOC equal so the app's two
   views agree.
10. HR54 must stay 1. HR58 auto-judge is dead code, so a BMS that drops out does not flip the
    inverter to lead-acid mode; HR332 = 1 would.
11. HR20 bits 2 and 3 (per bms-analysis) end a calibration. The ARM never times one out.

What I cannot tell from the ARM, because it lives in the DSP:

- how the DSP builds the FC4 frame from the ARM's 7 bytes, its reply timeout and turnaround
  window, and whether it retries;
- the whole FC3 HR0-HR27 handling: which HR becomes `batt_soc_dsp`, `batt_temp_dsp` (and its
  unit), the "full" byte, the battery state byte, the BMS-present word and the capacity word;
- which HR20/HR19 bits become warning bits 0x800, 0x400 and 0x002;
- current limit and voltage trip logic, SOC floor, battery-lost timeout;
- what pages 0, 1, 2, 4 and 7 mean on the DSP side for HR114, HR115, HR116, HR176, HR177;
- whether the battery firmware relay frames (`"BMS",1,"HED"` ... `"BMS",1,"END"`, sent by
  `bms_fw_update_process` 0x0802A726 on UART4, the DSP link) are passed to the BMS bus. Inference:
  they are, since the ARM has no other path to the battery. They are only sent when a USB stick
  holds an ARMStore.bin with version > 1000 and HR46 is non-zero.

bms-analysis docs/05 has answers for most of these from Ghidra emulation of D316. They are not
byte-exact and I have not re-checked them here.
