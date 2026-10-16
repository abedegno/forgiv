# N1: 0x08014000 to 0x08025239

Names are in N1-functions.tsv (157 rows) and N1-globals.tsv (162 rows). Where I say "inference" I have not proved it from the code alone.

## C library and startup

0x08014158 to 0x0801422B is Keil's C library startup (`__scatterload`, `__scatterload_zeroinit`, `__rt_lib_init`, `__rt_lib_shutdown`, `__rt_entry`). I took the names from the gate's library link listing (build/gate/library/link.lst). `__rt_entry` calls `main` at 0x08014242. `main` runs SystemInit, disables interrupts, runs a long delay and `board_init`, then creates eight tasks with 0x0803062E. In my range these tasks are `task_dsp_rx` (prio 1), `task_dsp_tx` (prio 2) and `task_modbus_rs485` (prio 3). The others are at 0x0802735A, 0x0802D41E, 0x08026426, 0x08029AA6 and 0x0802AEF4. `nvic_init` puts the vector table at 0x08014000, so a bootloader sits below it. Writing 1 to HR164 (FC16 only) stops SysTick, masks all IRQs and jumps through the vector at 0x0800002C into that bootloader.

## Interrupt handlers (f0801432C)

I named these from the vector table and the startup file. The USART handlers check TC and RXNE and pass the work on to the UART queue code. `EXTI1_IRQHandler` and `OTG_FS_IRQHandler` follow ST's USB host examples, which gives `USB_Host` (0x20003A24) and `USB_OTG_Core` (0x20003474). `TIM3_IRQHandler` starts an ADC conversion every second tick and toggles PB5 (inference: the buzzer). `SysTick_Handler` releases the RS485 driver-enable pins two ticks after a transmission ends.

## UARTs

All four ports run at 9600 baud. Each has a 9-byte transmit queue and receive queue (`uart_tx_q`, `uart_rx_q`) and a 151-byte buffer.
- USART1: 0x77-framed pages from another MCU (`aux_uart_rx_byte`). There are four pages of 48 words each, with a CRC. Byte 1 carries 2-bit DRM states. 0x080259DE turns these into `drm_power_limit` (2/40/70/100 %) and `drm0_active`, but only for country codes 3, 0x14 and 0x15 with HR41 set. Inference: this is the DRED/DRM interface for the AS/NZS 4777 grid codes. The page data may be a waveform for a cross-check with the ARM's own ADC samples. If the link is lost, fault bit 0x40 is set.
- USART2 (RS485, DE on PA4): the external meter master (0x0802B028, FC4 polls). HR47 switches the port between 8N1 and 8E1.
- USART3 (RS485, DE on PB2): the Modbus RTU slave for the dongle/cloud.
- UART4: the link to the DSP. It has no DE pin.

## DSP link (f08015378): this is where the battery data comes from

I found no BMS protocol in the ARM in this range. The battery data reaches the ARM through the DSP.
- ARM to DSP (`task_dsp_tx`): a 45-byte frame that starts 0x43 and ends with CRC16/MODBUS. It holds the run-mode command, an active power rate in % (HR50, capped by DRM), a state byte and a 16-bit battery power setpoint. The setpoint is HR495 when byte 0x61 of `cfg_block6` is set (inference: HR494 enables it). Otherwise it is -`batt_power_calc`/10. After that comes one of 8 rotating pages of settings (`dsp_build_page`):
  - page 0: grid code, HR60 PV start voltage, battery power mode HR27, battery type HR54, battery capacity HR55, and charger, heatsink and battery temperatures.
  - page 1: battery under- and over-voltage limits HR97/HR98, plus 14 grid limits.
  - page 2: grid port max power HR26. It is forced to 6000 when 0x20001B3F is non-zero.
  - page 4: HR109, HR111/HR112 (battery charge and discharge limits, up to 250), HR115 and `batt_cal_request`.
  - pages 5 and 6: the calibration gains.
- DSP to ARM (`task_dsp_rx`, `dsp_parse_page`): a 45-byte frame that starts 0x44. It carries run mode, charge status, fault and warning words, and one of 7 pages of telemetry.
  - Page 0 holds PV values, battery voltage (0.01 V) and current (0.01 A), and a SOC byte (`batt_soc_dsp`) plus a battery temperature.
  - Page 5 carries raw BMS bytes in five 20-byte sub-pages. 0x08026BA4 unpacks them into `bms_modules`. Modbus serves these as IR60-119 of slaves 0x32-0x36: 16 cell voltages, 4 temperatures, capacities, status and cycles (`modbus_fill_battery_regs`).

So the DSP owns the BMS conversation. The ARM only relays the BMS data and passes the limits through.

## Battery handling seen from here

- Defaults (`cfg_load_defaults`, block 1): battery type 1, capacity 100, under-voltage 43.20 V, over-voltage 57.20 V, grid port max 6000 W, HR110 = 4, HR111/HR112 charge/discharge limit 50 each.
- SOC (0x080258D6, just outside my range, read for context): with battery type 1 (lithium), IR59 SOC and battery temperature come straight from the DSP/BMS. With type 0, the SOC is linear in battery voltage between HR97 and HR98. While charging (status 2 or 3) it uses the full span and caps at 97 %. Otherwise the top of the span is 96 % of HR98 and the cap is 100 %. A DSP state byte of 3 or more forces 0 %, and the "full" byte forces 100 %.
- HR175 (`batt_cal_request`) raises the PV start voltage sent to the DSP from the HR60 value to 155.0 V. It clears itself after 300 frames with battery voltage above 54.0 V and current above 10.00 A. Inference: this is a full-charge or calibration request.
- `led_status_update` blinks LED1 in normal mode when the DSP SOC is at or below HR114 + 1. Inference: a "battery at reserve" indication.
- In the FC6/FC16 handlers HR111 and HR112 accept values up to 250, HR110/114/116 up to 100, and HR97/98 any value up to 0xFFDC. I found no cross-checks between them.

## Modbus slave (f08017FCC, the 45 KB file)

`modbus_slave_process` takes requests from USART3 or from USB CDC (0x080365F8). It accepts its own address, 0, 1-8, 0x11 and 0x30-0x37. Every reply header carries the 10-byte inverter serial.
- FC3 `modbus_fc03_read_holding` and FC4 `modbus_fc04_read_input`. FC4 serves IR0-59 from `ir_regs`, and IR60-119 per slave: meters 1-8, 0x30, 0x31 and batteries 0x32-0x36. It also serves 120-299.
- FC6 `modbus_fc06_write_reg` and FC16 `modbus_fc16_write_regs` share one large switch. It covers about 385 holding registers, from HR1 to HR4385. Writes go into the settings blocks and set `eeprom_dirty`.
- HR23/HR24 select a chip (DSP or ARM) and a peek address.
- HR163 restores factory defaults. HR162 clears the logs.
- FC 0x16 `modbus_fc22_read_meter` returns the meter records.

I mapped IR0-59 to their RAM sources through 0x0802D0EA. They follow the public GivEnergy input register list.

## Settings and EEPROM (f0801756C, f08023154, f080234B4)

The EEPROM is a 24Cxx-style chip at I2C address 0xA0, bit-banged on PB6/PB7 with write protect on PB8. It holds eight blocks, each with a length, a tag and CRC16, and pages are verified after writing.
- Block 1 is the settings block 0x20001AAE (0xD3 bytes).
- Block 2 holds the calibration gains and the fault log.
- Block 3 is the event history. Block 4 holds the energy totals. Blocks 6-8 are more settings.

`cfg_load_all` loads every block at boot and falls back to defaults where a block fails. `grid_load_country_limits` copies the per-country trip limits from the constant tables for the 29 country codes in HR2.

## Faults, LEDs and the panel

- `fault_flags` (0x2000011C) is set and cleared inside critical sections. Any bit other than 0x40 puts the run mode to 3 (fault).
- `fault_code` is the first set bit.
- `fault_log_push` and `event_log_push` keep timestamped histories in EEPROM.
- The LEDs are PB0/PB1 (status) and PB9 (fault blink). The panel indicators are on PB14/15, PC6/7, PC9 and PA8, driven by 0x0802965E: PV present, battery charging or discharging, and charge status.

## Small helpers and dead code

Several functions have no caller and no pointer in the image. armlink kept them because there is no `--split_sections`. They are `min3`, `max3`, `median3`, `clamp`, `div_u16_safe`, `calc_checksum_xor_index`, `hex_nibble_to_ascii`, four empty stubs, `tim4_init`, the PE2 functions, `i2c_write_reg`, `i2c_read_reg`, `fault_debounce_count`, `SystemCoreClockUpdate` and `sys_delay_loops`.
