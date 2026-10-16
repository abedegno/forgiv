# N2 notes (0x0802523A to 0x0803240B)

Plain notes per subsystem. Where I guess, I say so.

## Scheduler (0x08030298 to 0x080307CA)

A small scheduler shaped like uC/OS-II but not Micrium's code. Eleven TCBs at 0x20007C6C, a
16-bit ready mask, and per-task event flags instead of semaphores or queues. `OSTaskCreate`
(0x0803062E) takes (task, arg, stack words, prio) and carves the stack from a 16 KB pool at
0x20003DEC. `OSTimeTick` (from SysTick) counts each task's period and posts flag 1 when it runs
out, so every task is a loop that sets its own period and then calls `os_flags_pend(0)`.
`os_task_delay(prio, first, period)` sleeps by borrowing the period counter. `OSTaskStkInit` is the
Micrium Cortex-M3 port's stack frame word for word. The tasks main creates:

| prio | entry | what (my name) |
|---|---|---|
| 0 | 0x0802735A | task_measure (ADC interrupt posts flag 2; RMS values, output enable, BMS pack poll) |
| 1 | 0x080171DA | (outside my range) |
| 2 | 0x08016FDC | (outside my range) |
| 3 | 0x08022F7A | (outside my range) |
| 4 | 0x0802D41E | task_meter (RS485 Modbus master on port 1) |
| 5 | 0x08026426 | task_monitor (20 ms: LEDs, temperatures, SOC, CT, power flow, fault logging) |
| 6 | 0x08029AA6 | task_schedule (500 ms: RTC, charge/discharge scheduling) |
| 7 | 0x0802AEF4 | task_usb (USB host) |

## Battery

What the ARM side does with the battery, as far as my range shows it:

- **SOC.** With `battery_type` (HR54, settings +0x78) = 1 (lithium), IR59 is the SOC byte the DSP
  sends in its frame (0x200002E3), and IR56 is the battery temperature from the same frame. So the
  BMS SOC reaches the ARM through the DSP, not through the ARM's own pack poll. That is my reading
  of the writers. With `battery_type` 0 (lead acid), `batt_soc_from_voltage` (0x080258D6) works
  out SOC as a straight line between two voltage settings (settings +0x7E empty, +0x80 full). It
  caps at 97% while charge_status is 2 or 3, scales full to 96% otherwise, gives 0 when the DSP's
  battery state is 3 or more, and gives 100 when the DSP sets a full flag. The battery NTC
  (`ntc_battery_temp_update`) is only used in that case.
- **Battery pack poll** (`bms_poll`, 0x08026C40, in the highest-priority task). Every 500 passes
  it asks pack 1..5 with Modbus function 4 for registers 0 (21 regs), 21 (19 regs) and 40 (20 regs).
  It keeps each value only if it is in range: temperatures -30.0 to 70.0 C, 32-bit capacities
  under 60,000, 80,000 and 200,000, SOC 0..100, and **16 cell voltages 2200..3700 mV** (16S LFP).
  The records are 0x83 bytes each at 0x200007A4. Status bit 0 of each pack counts toward
  `bms_packs_online`. Which UART this uses, and where the CRC is added, I did not trace.
- **Auto battery type** (HR58, `batt_auto_judge_type`). When the DSP reports no BMS, the inverter
  switches itself to lead acid with 125 Ah and a setting of 20 at +0x99. When a BMS appears, it
  switches to lithium and sets +0x99 to 50. As an inference, +0x99 is a charge or discharge current
  limit in A, but I have not seen it used.
- **Charge and discharge scheduling.** There are 10 charge slots and 10 discharge slots, each with
  an HHMM start, an end and a SOC. Charge slot 1 is HR94/95 and enable_charge is at settings +0x8B.
  Discharge slot 1 is HR56/57, slot 2 is HR44/45, and enable_discharge is HR59. The others live in
  the second settings block at 0x20001C82. A slot may wrap past midnight. HR29 (`soc_force_adjust`)
  bits override the slots with a forced discharge or a forced charge. Charge always wins over
  discharge. There is also a "table mode" (settings_ext[0] = 1) with two 48-entry half-hour
  tables, where a value over 10000 means a negative number, compared against three thresholds. I
  infer this is a tariff (price) mode.
- **Night recovery charge** (`batt_night_recovery_charge`). Between 01:00 and 04:59, if the battery
  has been flat for 60 s, it forces a grid charge for N minutes (setting +0x97). For lead acid,
  flat means the voltage is more than 500 units under the empty setting. For lithium, it means SOC
  0 or SOC 10 points under the reserve (+0x9C). The ARM only raises `charge_active`; the DSP
  applies the actual current and voltage limits. I found no HR25/26/27-style charge or discharge
  current limit used in my range. HR27 here is battery_power_mode, which only gates the export
  command to the device at Modbus address 0x11.
- **Battery firmware relay.** If 0:ARMStore.bin on a USB stick has the 0x5566 header and a version
  over 1000, and the BMS chip version (HR46) is nonzero, the ARM does not flash itself. Instead
  `bms_fw_update_process` streams the file on port 3 in 1 KB frames. Each frame starts with
  `"BMS",1,"HED"`, carries the data and a byte sum, and ends with 'E'. The last frame is
  `"BMS",1,"END"` plus the inverted CRC-32 of the image. A failure is remembered in a stored byte at
  0x20001BB5 (loaded at boot by `bms_update_fail_flag_load`), and that blocks further tries. Port 3 also carries the DSP update, so as an inference the battery link may run
  through the DSP or share that bus.

## Meters (f0802B028, f0802C94C)

This is an RS485 Modbus RTU master on port 1, and up to 8 meters at addresses 1..8. HR7
(enable_ammeter) is a bitmask of which ones exist. HR47 is meter_type: 1 means EM115 (values times
10), otherwise EM418 (kW times 10000). HR48/49 reverse a meter's direction bit by bit. Requests are
function 4 at registers 0x10, 0x4E, 0x52, 0x92, 0xD2, 0x112, 0x152, 0x160, 0x166, 0xFF00..0xFF06,
0x524/5, and the replies are big-endian floats. Meter 1 is the grid. Meters 2 and 3 add to PV
energy, so I take them to be AC PV meters. Meters 5..8 are subtracted from the grid power, which I
infer means more AC sources. After 2000 passes with no reply, `meter_comm_fault` is set: IR0 then
shows 2 (warning) and the red LED lights. The same bus also polls a device at address 0x11 (fn 3
from 0xCA, 38 regs; fn 6 to reg 0xE7, with the CRC bytes in the other order). The command written
is the export power (when HR27 = 1 and no slot is active) or 36000. As an inference, this is an
export diverter or EV charger.

## Modbus register images

`modbus_hr_update` and `modbus_ir_update` copy the live variables into HR0..59 (0x20000682) and
IR0..59 (0x2000060A). The positions match the public GivEnergy register names (HR21 = 316, the
ARM version; HR34 = 140, the Modbus version; HR35-40 the clock; IR59 SOC), and that is where most
of my "likely" global names come from. HR25 is a debug peek: it returns the RAM word at
0x20000000 + HR24.

## Measurements

- The NTC curves for the heatsink (IR41), charger (IR55) and battery (IR56) are the same 14-piece
  table, each averaged over 128 samples.
- CT clamp: 191 voltage and current samples give P = sum(v*i)/0x776 and I_rms = isqrt(sum(i*i)/191).
  The last word of the sample block carries either the battery NTC or the DRM input bits.
- DRM (AS/NZS 4777) only applies for Australian grid codes and HR41 enable_drm_rj45. It limits
  power to 2/40/70/100% or requests DRM0.
- `energy_accumulate` integrates every 0.5 s into 0.1 kWh day and total counters for grid
  import/export, PV1/PV2, inverter in/out, battery charge/discharge and load. Day counters reset
  at midnight from the RTC.

## USB, firmware update, FatFs

- ST USB host library V2.x: `usb_bsp.c` (TIM2 delays) and the 18 `USBH_USR_*` callbacks, named
  from the slot order of `USR_cb` at 0x200003E0. A mass-storage stick (class 8) gets FatFs. A CDC
  device (class 2) gets a 57600 8N1 link. As an inference, that is the WiFi/LAN dongle.
- Stick files: `0:DSPStore.bin` (sent to the DSP on port 3 with a 0x0A frame protocol: W, S, D, E,
  ack 0x0A 0x4F), `0:ARMStore.bin` (a self-update: it writes 0 into the version word at
  0x08013FFC for the bootloader; it only accepts the same major version), `0:SN.txt` (rewrites the
  inverter serial) and `0:MSN.txt` (factory: deletes the images after use).
- FatFs R0.12c `ff.c` from 0x0803083C. I named every function in source order. The sizes and the
  callers (`f_unlink("0:ARMStore.bin")`, `f_getfree` in the USB code, `f_close` calling `f_sync`)
  agree with that order.

## RTC

This is the common STM32 RTC calendar code (`RTC_Init`, `RTC_Get`, `RTC_Set`, `RTC_Get_Week`,
`Is_Leap_Year`, `RTC_Alarm_Set`, with `table_week` and `mon_table`). It uses BKP DR4 = 0xA5A5 or
0x5050 as the "set" marker, defaults to 2017-03-01 02:03:04, and stores the year as an offset
from 2000.

## Clashes and loose ends

- map/data.tsv already names some globals I looked at (USBH_MSC_cb, CDC_cb, CDC_SetLineCode,
  USB_OTG_Core, USB_Host, UserCb, dongle_link_timer at 0x20000310). My reading agrees with
  them, and I left them out of my globals file.
- `sched_window_value` (0x0802993E) sets 0x2000038C from settings +0xC6 inside a time window. I
  did not find what reads it.
