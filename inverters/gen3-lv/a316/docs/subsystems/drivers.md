# Drivers

Files: `src/app/board.c`, `src/app/drivers/uart.c`, `eeprom.c`, `i2c.c`, `nvic.c`,
`panel_led.c`, `rtc.c`.

## Architecture

`board.c` brings the board up from `board_init`: the panel, LED, buzzer and I2C pins, then the
settings from the EEPROM (`cfg_load_all`), TIM3, the ADC with DMA, PC9, the four UARTs and the
run-time variables. It also holds the factory defaults (`cfg_load_defaults`) and the
per-country grid limit tables.

The UARTs all run at 9600 baud:

| Port | Queue | Pins | Use |
|---|---|---|---|
| USART1 | 0 | PA9/PA10 | the auxiliary MCU (CT waveform, DRM inputs, battery NTC) |
| USART2 | 1 | PA2/PA3, DE PA4 | RS485 master: meters 1-8 and the device at 0x11; 8E1 when HR47 = 0 |
| USART3 | 2 | PB10/PB11, DE PB4 | RS485 Modbus slave |
| UART4 | 3 | PC10/PC11 | the DSP |

`uart.c` keeps a transmit and a receive queue per port. Transmission is one byte per
transmit-complete interrupt. Reception for ports 1 and 2 is a linear buffer the task drains
with `uart_rx_getc`; UART4 has its own rules (frame start 0x44, update replies, "OK" in the
battery relay); USART1 bypasses the queues (`aux_uart_rx_byte`).

The EEPROM is a 24Cxx-type part at I2C address 0xA0 with 16-bit addressing, driven by
`i2c.c` (bit-banged, PB6 SCL, PB7 SDA) and `eeprom.c`. Each settings block lives at
block number x 0x100 and is stored as `[length][tag][data][CRC16]`, every byte inverted. The
table is in the header of `eeprom.c`. Blocks are written in 8-byte pages with a verify pass.

`rtc.c` is the common STM32 RTC calendar code: seconds since 1970 in the RTC counter, LSE
clock, BKP DR4 as the "set" mark. `nvic.c` sets the vector table and the priorities.
`panel_led.c` drives the front panel indicators.

## Data

- `cfg_settings` (block 1, 0xD3 bytes), `cal_gains` (block 2), `fault_log`, `event_log`
  (block 3), `energy_totals` (block 4), `cfg_block6/7/8`: defined in board.c's data, typed in
  `settings.h` and `energy.h`.
- `eeprom_dirty`: one bit per block to save (`eeprom.h`).
- `grid_limits_tbl`, `tbl2`, `tbl3`, `tbl4`: 29 countries' trip limits and times (flash).
- `uart_tx_q`, `uart_rx_q`, `uart_rx_buf[4][151]`.
- `rtc_time` (years since 2000), `rtc_daycnt`.

## Rules found in the code

- Boot loads each block up to five times; a block that fails gets its defaults and is marked
  dirty. When anything failed, `task_monitor` re-reads the dirty blocks 2 s and 4 s later before
  writing defaults over the chip (`eeprom_reload_flagged`), so a passing I2C error does not wipe
  the settings. Block 1 is not retried that way.
- When block 1 is lost, the 32-byte header copy at 0x1E0 restores the model (HR1, HR2) and the
  Modbus address; without it the model becomes 3/0x32 and the address 0x11.
- A block 1 stored with another length (an older layout) is loaded as far as it goes and then
  rewritten at today's length.
- At every boot: HR53 is set to 1 (the "off" branch is dead), the calibration gains outside
  0.70..1.30 are reset to 1.0, and while the layout byte +0xA6 is at most 0x14 (always, in this
  image) HR97 and HR98 are set to 43.20 V and 58.50 V.
- The grid limits come from the country code in HR2's high byte (0-28). HR28 = 1 adds 10.00 Hz
  to every frequency limit. The voltage and frequency limits written over Modbus go through
  `cfg_limit_check`, which accepts everything.
- An EEPROM save that fails six times is dropped (task_monitor).
- The clock is kept in the RTC as seconds since 1970; years before 1970 or after 2099 are
  refused. Without the backup mark the clock starts at 2017-03-01 02:03:04.

## Open questions

- What PC3 and PC9 drive. PC3 is set to 1 on every `task_measure` pass. PC9 is set by both
  `gpio_pc9_set` (1 after 3 s) and `panel_led_pc9_set` (load present), so one of the two names is
  probably wrong.
- Whether PB5 is a buzzer. TIM3 toggles it at about 6.4 kHz while `output_enabled` is 1, which
  fits a buzzer but also a charge-pump or relay drive.
- The write-protect polarity: the code sets PB8 to 1 for writing.
- What the `g_` bytes that `sys_runtime_vars_init` clears are for; most have one or two uses.
