# Communication: DSP link, meters, Modbus slave

Files: `src/app/comm/dsp_link.c`, `src/app/comm/meter.c`, `src/app/comm/modbus_slave.c`.

## Architecture

Three links, each with its own task:

```
 DSP (TI C2000) <--UART4 0x43/0x44 frames--> task_dsp_tx / task_dsp_rx
 meters 1-8, device 0x11 <--USART2 RS485, ARM is master--> task_meter
 app, cloud, GivTCP --USART3 RS485--> task_modbus_rs485 --+
                    --USB CDC dongle--> CDC_ReceiveData --+--> modbus_slave_process
```

### DSP link (dsp_link.c)

Both directions are 45-byte frames with a CRC16/MODBUS over the first 43 bytes, sent high byte
first. The ARM sends a 0x43 frame every 60 ms: run mode, fault bits, the active power rate,
a charge/discharge state, a battery power setpoint, and one of eight settings pages in turn
(so each page goes out every 480 ms). The DSP sends 0x44 frames: run mode, charge status, a
32-bit fault word, a warning word, and one of seven telemetry pages. The byte layouts are in
the comments of `dsp_build_page` and `dsp_parse_page`.

The battery reaches the ARM only through the DSP. Page 0 carries the BMS SOC, battery voltage,
current, power and temperature; page 2 the BMS capacity, a "full" byte and the BMS versions;
page 4 a BMS-present word and the DSP's fault words; page 5 the raw reply to the ARM's own FC4 pack
poll, in five 20-byte sub-pages. The ARM's request for that poll travels on its page 6.
docs/battery.md covers this in detail.

The same task and port also carry the firmware updates (usb.md): the DSP update with 0x0A
frames, and the battery relay with 1034-byte "BMS" frames.

### Meters (meter.c)

The ARM polls meters at addresses 1-8 (HR7 says which exist) with function 4 (or 3 for the
raw words of the HR47 = 1 type), one request every 200 ms: 9 float registers and 7 raw words
per meter, then two exchanges with the device at 0x11, then the next meter. While a grid-side
meter exists, every regular request is followed by fresh active-power reads of meters 1 and
5-8. Replies are matched to the request by the step that sent it, not by the register in the
reply. The same task rebuilds the Modbus images HR0-59 and IR0-59 every fifth pass (25 ms).

### Modbus slave (modbus_slave.c)

One handler, `modbus_slave_process`, serves both transports. Requests are standard 8-byte
frames, except that function 16 has no byte count. Replies carry a 16-byte header: slave,
function, the 10-character inverter serial, the start and count of the request. A read is
limited to 60 registers and may not cross a multiple of 60. The register map is in the file
header and in the comments of the read and write handlers.

Slaves answered: the inverter (HR30), 0, 1-8 (the meters' views), 0x11, 0x30 (system), 0x31
(battery unit), 0x32-0x36 (battery packs 1-5), 0x37.

## Data

- `dsp_tx_frame`, `dsp_rx_frame`, `dsp_tx_page`; the DSP values (`dsp_link.h`, `battery.h`,
  `measure.h`); `dsp_bms_raw`, the mailbox for the pack poll.
- `meter_data[8]` (`meter_t`), `dev11_data`, the poll state.
- `hr_regs[60]`, `ir_regs[60]`, `modbus_reg_buf`, `modbus_reply`, and through the handlers
  every settings block.

## Rules found in the code

DSP link:
- The setpoint the DSP gets in bytes 5-6 is HR495 when HR494 is exactly 1; otherwise it is
  minus the CT or grid meter power / 10 (a self-consumption target, inference).
- The state byte is 2 under HR329 forcing, else 1 when charging or under HR175, else 2 when
  discharging, else 0.
- HR50 is capped by the DRM limit for grid codes 2 and 3 with HR41; HR53 is replaced by DRM0
  for grid codes 2, 3, 20 and 21.
- HR26 goes out as 6000 while a calibration (HR29) runs; HR60 goes out as 155.0 V while HR175
  is 1; HR175 clears itself after 300 page-4 frames with batt_voltage > 540 and batt_current >
  1000.
- 50 bad frames, or 50 wake-ups without a frame, set fault bit 0x80; any good page clears it.
- The calibration (HR29) is advanced by the DSP's warning bits: 0x800 sets bit 1, else 0x400
  or 0x002 sets bit 2, and 7 ends it.

Meters:
- A meter silent for 2000 passes (10 s) sets the meter fault: IR0 reads 2, IR57 reads 1, the
  meter values are cleared, and the DSP gets the flag on page 4.
- HR48/HR49 reverse a meter: its active power is negated and its import and export energies
  swap. Changing them swaps the stored day-start counters too.
- The day's energy from a meter is its counter minus the counter at the start of the day.
- The device at 0x11 gets a power command every cycle: 36000 while HR230 = 1 (the default),
  else the export power when HR27 = 1, export is above 50 W and no slot is active, else 0.

Modbus slave:
- Over USB the 8-byte requests carry their CRC low byte first (standard) and function 16 high
  byte first; over RS485 every request must carry it high byte first. Replies are always low
  byte first.
- Writes to slave 0 over RS485 get no reply.
- A write outside the register's range is refused with an exception reply, and in a
  function 16 write the registers before the refused one stay written.
- Calibration gains change only through 1.0 (0x400), offsets only through 0.
- HR481-490 are RAM-only copies of ten control registers; HR166 = 1 makes the schedule and
  battery registers RAM-only too. The half-hour tables HR360-467 and HR175, HR200, HR201,
  HR329, HR495 and HR4000 are never saved.
- HR163 restores factory defaults (1, 2), the calibration (2, 3); HR162 clears the counters
  and logs; HR164 = 1 enters the bootloader.
- HR25 reads any RAM halfword when HR23 = 1 (ARM): the address is 0x20000000 + HR24.

## Open questions

- Whether the RS485 CRC order is meant: a standard Modbus master sends the low byte first and
  would be refused on RS485, while the dongle's frames over USB pass. I have not seen what
  GivEnergy connects to the RS485 slave port.
- Which DSP page fields the `dsp_pN_*` names stand for; what `g_200001D2` (frame byte 9) is
  beyond "0 keeps the output off".
- The meter types: "115" and "418" are from the register names; which register of each meter
  is current and which is frequency is not certain (findings.md).
- What the device at 0x11 is (an export limiter, a diverter or a charger), and why its
  command defaults to 36000.
- What HR200, HR201 and HR4000 make the DSP do.
