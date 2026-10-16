# Findings from the comment pass

While commenting `src/app` and `src/include` I read every application function. This file
lists what looks wrong, what is dead, the rules of the program that the code shows, and what I
could not settle. Each item names the file and function. I re-read each bug in the source
before listing it, and I dropped four that did not hold (at the end).

Confidence labels:
- **confirmed**: the faulty behaviour follows from the code alone and is not plausibly meant;
- **likely**: the code certainly does this, and it is probably not what was meant;
- **possible**: the code does this; whether it is a fault depends on things outside the ARM
  image (the DSP, the hardware, the other end of a link) or on intent I cannot see.

Effects on the inverter are my inference unless I say otherwise.

## Likely bugs

### Confirmed

1. **The CRC-32 sent with a battery firmware image is a constant.**
   `control/schedule_p3.c: crc32_table_init` has no caller and no pointer to it anywhere in the
   image, so `crc32_table` (zero-initialised RAM) stays all zero. `crc32_update` then reduces to
   `crc = crc >> 8`, which is 0 after four bytes whatever the data. `usb/usbh_usr.c:
   bms_fw_update_process` therefore sends ~0 = 0xFFFFFFFF as the image CRC in its END frame.
   Effect: a BMS that checks the CRC would refuse every image relayed by this ARM version; one
   that does not check it gets no protection.

2. **The second END frame of the battery relay carries the CRC inverted back.**
   `usb/usbh_usr.c: bms_fw_update_process`, state 0xA2, inverts `bms_update_crc32` in place each
   time it sends the END frame, at the 3rd and the 7th call. The first frame has ~crc, the second
   crc. With item 1 that is 0xFFFFFFFF and then 0. The rest of each END frame (bytes 11-1033) is
   whatever the last data frame left in `dsp_tx_frame`.

3. **Reads outside the served ranges use an uninitialised length.**
   `comm/modbus_slave.c: modbus_fc03_read_holding` (start 540-4079, 4200-4339, 4400 and up:
   `reply_idx`), `modbus_fc04_read_input` (start 300 and up: `j`) and `modbus_fc22_read_meter` (start
   120 and up: `j`) have no branch that sets the reply index, then compute the CRC over that
   many bytes and store the two CRC bytes at `modbus_reply + index`. The value is whatever the
   stack slot held. Effect: a wrong or no reply, two bytes written somewhere within 64 KB above
   `modbus_reply` (0x20001276), which can corrupt RAM, and with a large value a CRC read past
   the end of RAM, which would end in a bus fault and a reset. Any Modbus master, including the
   cloud through the dongle, can trigger it with one read.

4. **Setting the serial number from a stick overwrites HR138.**
   `usb/usbh_usr.c: USBH_USR_MSC_Application` reads 10 bytes of SN.txt with `f_read` (no
   terminator) and copies them with `strcpy` into the serial at settings +0x13. `strcpy` stops at
   the first zero after the 10 bytes, so it writes at least one byte more, onto settings +0x1D,
   the low byte of HR138 (default 2484, a grid limit sent to the DSP on page 3), and more bytes
   if `usb_file_buf[10..]` holds non-zero data left by an earlier read. The settings are then
   saved.

5. **An uninitialised stack buffer is sent to the dongle.**
   `control/schedule_p3.c: sys_tick_1s` sends `local_4` (304 bytes on the stack, never written)
   with `CDC_SendData(local_4, 302)` when the dongle has been silent for 60 s. Effect: 302 bytes
   of stale stack go to the dongle, perhaps meant as a wake-up. Harmless to the inverter as far
   as I can see, but the content is arbitrary.

6. **The header-copy load uses an uninitialised retry counter.**
   `drivers/eeprom.c: eeprom_load` with block 5 (the 32-byte header copy) loops `while (attempt++ <
   tries)` without setting `attempt` to 0 in that branch. Effect: the header copy, used only when block
   1 failed at boot, may be tried fewer than five times or not at all, and then the model and
   Modbus address fall back to 3/0x32 and 0x11.

### Likely

7. **HR97 and HR98 are reset at every boot.**
   `board.c: cfg_load_all` sets HR97 = 43.20 V and HR98 = 58.50 V, and marks block 1 for saving,
   whenever the layout byte at settings +0xA6 is at most 0x14. The same code sets that byte to
   0x14 and nothing in the image ever sets it higher, so this runs on every start. Effect: battery
   voltage limits written over Modbus last only until the next restart, the HR98 default is in
   practice 58.50 V (not the 57.20 V of `cfg_load_defaults`), and block 1 is rewritten at every
   boot, unless the EEPROM already holds a higher byte written by another firmware version (no
   Modbus write in this image reaches +0xA6). Probably a version number that was meant to be bumped. (The DSP, not the ARM, acts on
   these limits; docs/battery.md, section 3.)

8. **A stray UART4 byte clears the first byte of the Modbus RS485 buffer.**
   `drivers/uart.c: uart_rx_isr`, port 3: when the first byte of a would-be DSP frame is not 0x44
   it sets `uart_rx_buf[2][0] = 0`, which is USART3's buffer, not UART4's (`[3]`). Effect: a
   Modbus request arriving at that moment may lose its address byte and be dropped.

9. **The wait before a USB reply is one tick, not twenty.**
   `comm/modbus_slave.c: modbus_slave_process` sets `modbus_usb_wait = 20` and busy-waits for 0;
   `stm32f10x_it.c: SysTick_Handler` sets it to 0 at the next tick instead of counting it down.

10. **A battery image block of only 0xFF bytes ends the relay in a timeout.**
    `usb/usbh_usr.c: bms_fw_update_process`, state 1, does not send a 1 KB block whose bytes are
    all 0xFF but still waits for the "OK" answer (state 0xA1). No answer comes, and after 300
    calls the relay is marked failed. The frames carry no block number, so the receiver could not
    place the following blocks anyway. Images padded with 0xFF are common.

11. **A reversed meter's apparent power saturates.**
    `comm/meter.c: meter_parse_fc04`, step 5: for a meter whose HR48/HR49 bit is set the apparent
    power is multiplied by 4294967296.0 before the conversion to an integer, so it saturates (or
    becomes 0 for a negative value). Likely meant as a sign change like the active power. Shown
    as IR72 of the meter's slave.

12. **The half-hour tables are lost at a restart, and table mode then charges all day.**
    `comm/modbus_slave.c: modbus_fc06_write_reg` (HR360-467) stores both tables in
    zero-initialised RAM outside every EEPROM block, while the table mode switch HR322 and the
    thresholds HR323-325 are saved (block 6). After a restart `control/schedule_p3.c:
    sched_halfhour_table_eval` reads 0 for every half hour, and with HR324 > 0 it sets
    `charge_active` for the whole day until the tables are written again.

13. **A function 16 write of more than 73 registers repeats one byte.**
    `comm/modbus_slave.c: modbus_fc16_write_regs` caps the data index at byte 150, so registers
    from the 74th on all get the value `byte150 << 8 | byte150`. The RS485 frame limit (300 bytes)
    and the 100-register cap in `modbus_slave_process` do not prevent it.

### Possible

14. **The RS485 slave expects the CRC high byte first.**
    `comm/modbus_slave.c: modbus_slave_process`, `modbus_src == 2`: both checks compare the CRC's
    low byte with the frame's last byte, so a request must carry its CRC high byte first, while
    standard Modbus sends the low byte first and the slave's own replies do too. Over USB the
    8-byte requests are checked in the standard order. Effect: a standard Modbus master on the
    RS485 port is refused, unless GivEnergy's own RS485 masters send the other order.

15. **With a grid meter, IR13 and IR58 may be swapped.**
    `control/monitor.c: power_flow_update` puts meter 1's `current` into `f_ac1` (IR13, grid
    frequency) and its `frequency` into `i_grid_port` (IR58, grid current). The field names come
    from `meter_parse_fc04` (register 0x4E current, 0x52 frequency), and `modbus_fill_meter_ir`
    serves them under those names as IR84 and IR63. Either this assignment or the field names are
    swapped; which one depends on the meters' register maps.

16. **The DRM country lists disagree.**
    `control/monitor.c: drm_mode_eval` handles grid codes 3, 20, 21 and 2, 8, 12;
    `stm32f10x_it.c: aux_uart_rx_byte` and `task_monitor` call it only for 2, 3, 8, 20, 21, so
    code 12 is never evaluated; `comm/dsp_link.c: task_dsp_tx` and `dsp_build_page` cap the power
    rate only for 2 and 3, and send DRM0 for 2, 3, 20, 21. For codes 2, 8 and 12 DRM0 is always
    on once evaluated.

17. **FC6 and FC16 accept different values for two registers.**
    `comm/modbus_slave.c`: HR51 (reactive power rate) is accepted up to 60 by
    `modbus_fc06_write_reg` and up to 50 by `modbus_fc16_write_regs`; HR167 (settings +0x73) can
    only be written with FC16.

18. **The ARM peek can read past the end of RAM.**
    `comm/meter.c: modbus_hr_update` serves HR25 as the halfword at `0x20000000 + HR24` when HR23
    = 1. HR24 = 0xFFFF reads across 0x20010000, the end of the STM32F105/107's 64 KB RAM, which
    would give a bus fault and, through `HardFault_Handler`, a reset. Its range test (`<= 0x10000`)
    is always true for a 16-bit value.

19. **A meter reply's byte count is not bounded.**
    `comm/meter.c: task_meter` takes the reply length from byte 2 (+5) without a limit, and
    `meter_rx_buf` is 150 bytes; a count above 145 writes into `meter_tx_buf` behind it. Only a
    corrupt or hostile reply on the meter bus does this.

20. **The EEPROM write-protect line stays active after a failed write.**
    `drivers/eeprom.c: eeprom_write_block` returns at a failed page with PB8 still set to 1
    (writing allowed on this board). The next write sets it again, so the effect is only that the
    chip stays writable.

21. **EEPROM reads acknowledge the last byte.**
    `drivers/eeprom.c: eeprom_read_block` sends an ACK after every byte, the last one too, before
    the STOP; I2C expects a NACK there. `i2c_bus_recover` runs before each access, which hides a
    slave that holds SDA.

22. **PC9 is driven by two functions.**
    `control/schedule_p3.c: sys_tick_1s` calls `gpio_pc9_set(1)` on every pass after 3 s, and
    `status_outputs_update` sets the same pin from the load power right after. While the load is
    below 50 W the pin pulses high for a moment every 500 ms. One of the two names is probably
    wrong about what PC9 is.

23. **The lead-acid SOC divides by a bad span.**
    `control/monitor.c: batt_soc_from_voltage` divides by `0.96 x HR98 - HR97` (or `HR98 - HR97`
    while charging) without a check. With HR97 at or above that, the divisor is 0 or wraps to a
    huge unsigned number, and the SOC reads 0.

24. **HR494 is stored in a byte and not saved.**
    `comm/modbus_slave.c: modbus_fc06_write_reg` stores HR494 into a byte (256 reads as 0) and,
    unlike the other block 6 registers, does not mark block 6 dirty, so remote mode survives a
    restart only if another block 6 write saves it.

25. **The weekday is computed for the wrong century.**
    `drivers/rtc.c: RTC_Get` passes years since 2000 to `RTC_Get_Week`, whose century test then
    never applies the 2000s correction. Nothing reads `rtc_time.week`, so there is no effect.

## Dead code

None of these has a caller or a pointer in the image (I searched the image for each function's
address); armlink kept them because the project was linked without `--split_sections`.

| Function | File | Note |
|---|---|---|
| `batt_auto_judge_type` | control/schedule_p3.c | the only reader of HR58 and `bms_present`; so HR58 "auto judge battery type" does nothing |
| `crc32_table_init` | control/schedule_p3.c | see bug 1 |
| `fault_log_push`, `event_log_push` | control/monitor.c | so the fault and event logs are never written; they are only cleared, loaded and saved |
| `fault_test`, `fault_none`, `counter_wrap_inc` | control/monitor.c | |
| `fault_debounce_count` | comm/dsp_link.c | would set fault bit 0x100 |
| `calib_apply_q10` | control/measure.c | |
| `max3_u32` | control/schedule_p3.c | |
| `bin_to_bcd` | comm/meter.c | |
| `usb_disk_getfree` | usb/usbh_usr.c | |
| `RTC_NVIC_Config`, `RTC_Alarm_Set` | drivers/rtc.c | so `RTC_IRQHandler` never runs |
| `tim4_init` | board.c | so `TIM4_IRQHandler` never runs |
| `hex_nibble_to_ascii` | board.c | |
| `gpio_pe2_init`, `gpio_pe2_set` | drivers/panel_led.c | |
| `i2c_write_reg`, `i2c_read_reg` | drivers/i2c.c | |
| `min3`, `max3`, `median3`, `clamp`, `div_u16_safe`, `calc_checksum_xor_index` | util.c | the whole file |
| `stub_noop_1` .. `stub_noop_4` | stubs.c | the whole file |
| `sys_delay_loops` | main_p4.c | the whole file |

Dead branches and data:
- `board.c: cfg_load_all` clears `f09` and then tests it for 1: HR53 is always set to 1 at boot.
- `comm/modbus_slave.c`, HR48/HR49 in both write handlers: the `== 0xAA` branch sits in the
  `else` of `<= 0xFF`, so it is never reached.
- `stm32f10x_it.c: ADC1_2_IRQHandler`: the `/ 1` averaging and the phase counter that always
  completes on the first sample.
- `g_200019E4` (130 bytes, served as IR120-171 by `modbus_fc04_read_input`) is never written, so
  those input registers always read 0.
- `ntc_stub` is called but empty.
- `SystemCoreClockUpdate` (cmsis, not my part) has no caller either.

## Recovered rules

The rules each subsystem follows are in docs/subsystems/ (main.md, drivers.md, comm.md,
control.md, usb.md) and the battery ones in docs/battery.md. The ones I found most useful:

- The ARM decides only flags and targets (charge, discharge, hold, two SOC targets, a power
  setpoint, the settings pages); the DSP runs the power stage and applies every limit.
- Write rules of the Modbus slave: a refused register ends an FC16 write with the earlier ones
  kept; gains change only through 1.0 and offsets only through 0; HR481-490 are RAM-only copies
  of HR27, 29, 56, 57, 59, 94, 95, 96, 50, 53; HR166 = 1 keeps the schedule and battery
  registers out of the EEPROM; HR175, 200, 201, 329, 360-467, 495 and 4000 are never saved.
- FC16 requests have no byte count; replies have a 16-byte header with the inverter serial;
  reads are limited to 60 registers within one block of 60.
- HR97/HR98 are reset at boot (bug 7); HR53 is forced to 1 at boot; calibration gains outside
  0.70-1.30 are reset to 1.0 at boot.
- Time slots: HHMM, end minute included, start = end off, start > end wraps; 10 charge and 10
  discharge slots; a charge cancels a discharge; nothing runs off grid or at charge status 7.
- Night recovery charge between 01:00 and 04:59, once a night, for HR108 minutes.
- HR329 forcing ends after 2 hours with SOC below 10; HR175 ends after 300 page-4 frames of
  charging current; a calibration (HR29) has no timeout.
- The EEPROM stores each byte inverted, with length, tag and CRC per block; a lost block 1 is
  rebuilt from a 32-byte header copy; a failed boot load is retried at 2 s and 4 s before
  defaults are written; a save that fails six times is dropped.
- A failed battery firmware relay blocks the next one only until the next restart, or until a
  stick with a refused ARMStore.bin is inserted.
- The dongle watchdog sends a frame after 60 s of silence and restarts USB after 12 minutes.
- A hard fault resets the ARM.

## Open questions

- Whether the RS485 slave's CRC order (bug 14) is meant for GivEnergy's own tools.
- Which meter register is current and which is frequency (bug 15), and which meters the "115"
  and "418" types are.
- What the device at Modbus address 0x11 is, and why its default command is 36000.
- The DSP side of every page: which `dsp_pN_*` field is what, what the DSP does with HR109,
  HR114-116, HR176, HR177, HR200, HR201 and HR4000 (docs/battery.md lists the battery ones).
- Whether the battery relay ever worked with this ARM version (bugs 1, 2, 10), or the BMS
  ignores the frames' CRC.
- What PC3, PC9, PB5 (buzzer or drive) and PD2 (mains sync) are on the board.
- What `sys_mode` means exactly; I follow the public register list (IR49: 1 on grid).
- The units of the CT power and of `g_200002B8`, and what `dsp_p2_d17` is; both feed energy
  counters.
- The bootloader's rule for the version word at 0x08013FFC.
- About 50 `g_` globals with one or two uses whose purpose the code does not show.

## Checked and dropped

- `calib_table_defaults` leaves the stored "battery update failed" word at 0x400; harmless,
  because `bms_update_fail_flag_load` only treats 1 as "failed".
- `task_dsp_rx` checks the CRC on every flag-8 wake-up; flag 8 is only set when 45 bytes of a
  0x44 frame are in, so it does not check partial frames.
- `bms_poll` counts six records though only five packs are polled; `bms_modules` has six
  records and the sixth stays zero.
- The USB code changes the NVIC priority grouping after `nvic_init`; the application's six
  interrupts keep preemption level 0 under every grouping it uses.
