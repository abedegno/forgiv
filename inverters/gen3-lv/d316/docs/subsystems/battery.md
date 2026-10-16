# Battery: the BMS Modbus link

File: `src/app/battery/bms_link.c`; header `bms.h` (the BMS page 0xD500, the protocol constants,
the link's variables). The rest of the battery path is in power_limits.c and op_states.c.

- `task6_bms_link` (RTOS task 6, every 40 ms) is the Modbus RTU master on SCI-B. Every 6th tick
  (240 ms) it sends one request to slave 1: an FC6 write (BMS HR1 wake around HR175, HR2, HR3,
  HR4 for the ARM), a request the ARM forwards, or the FC3 poll (HR0..HR27 with HR109 = 1, else
  HR17..HR25). GPIO57 is the RS485 driver enable.
- The receive side checks byte 0 < 0x10, FC 3 or 4, the exact length and the CRC, then hands FC3
  to `bms_parse_hr` and FC4 to `bms_fc4_reply` (for the ARM).
- `bms_parse_hr` stores HR0, the serial, HR11 capacity, HR13 firmware, HR14, HR15 (full poll
  only) and HR19..HR27 in the page and in `dsp_settings`. Firmware above 3010 uses HR26/HR27 as
  the charge/discharge limits, older firmware HR25 for both. A calibration (HR29) holds SoC to
  floor + 1 .. 99 and the limits to 8 A or more.
- 750 ticks (30 s) without a good reply call `bms_lost`: the reported values go to 0 and CF15 bit
  9 (comms) is set in BMS mode. The first good reply clears it.
- `batt_warning_update` builds the warning byte for the ARM from CF15 bits 2, 5, 6, 9 and CF17
  bit 6.

The full description, what each register drives and what a third-party BMS bridge must provide
is in [../battery.md](../battery.md). Bugs: [../findings-power.md](../findings-power.md) items 2,
3, 6 and 10.
