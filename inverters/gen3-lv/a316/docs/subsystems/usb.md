# USB: dongle link and update stick

Files: `src/app/usb/usb_task.c`, `usbh_usr.c`, `usb_bsp.c`; the ST host library in
`src/lib/usb_otg` and `src/lib/usb_host` (commented by the other pass).

## Architecture

The STM32's OTG FS port runs as a USB host. `task_usb` (priority 7, every tick) starts ST's host
stack 10 s after boot and runs `USBH_Process`. What is plugged in decides the class driver:

- interface class 2 (CDC): the WiFi/LAN dongle. The line coding is 57600 8N1. Each block the
  dongle sends goes to `modbus_slave_process` (the receive callback is set in
  `USBH_USR_EnumerationDone`), and replies go back with `CDC_SendData`. So the dongle, and
  through it the cloud and the app, uses the same Modbus slave as RS485.
- interface class 8 (mass storage): a USB stick with FatFs. `USBH_USR_MSC_Application` mounts
  it and looks for update files.

Files on a stick:

| File | What happens |
|---|---|
| DSPStore.bin | DSP update when its version differs from the DSP's with the same hundreds (or the DSP reports 0): the ARM stops normal work (run mode 4) and sends it over UART4 in 128-byte "D" frames |
| ARMStore.bin, version < 1000 | ARM self-update: the ARM clears the version word at 0x08013FFC; the bootloader takes the image on the next start (inference) |
| ARMStore.bin, version > 1000 | battery image: relayed in 1 KB frames through the DSP to the BMS (inference for the last step), when HR46 is non-zero |
| SN.txt | the first 10 characters become the inverter's serial number |
| MSN.txt | factory mode (inference): the image files are deleted after use |

The battery relay is started by the DSP: the ARM raises `bms_update_pending` on page 4, the
DSP answers with page 4 byte 16, and after 5 passes `task_schedule` sets `dsp_flash_mode`;
`task_dsp_rx` then runs `bms_fw_update_process`.

## Data

- `usb_class_code`, `usb_device_inserted` (HR22), `usb_cdc_timer`, `usb_reset_state`.
- FatFs objects `fil_update`, `fil_arm_image`, `fil_msn`, `fil_tmp`, `usb_fatfs`, the 1 KB
  `usb_file_buf`.
- Update state: `dsp_flash_step`, `dsp_update_*`, `arm_update_state`, `bms_update_*`,
  `bms_update_crc32`; the stored "battery update failed" word at block 2 +0x34.

## Rules found in the code

- The dongle watchdog (`sys_tick_1s`): 60 s without a good request sends a 302-byte block to
  the dongle; 12 minutes without one switch the OTG clock off and restart USB. The host core
  also re-sends the line coding when the dongle's line state has not changed for 1000 calls.
- One update per stick: the DSP update, the ARM update or the battery relay.
- A failed battery relay blocks further relays until the next restart, and inserting a stick
  whose ARMStore.bin is refused clears the block as well.
- The DSP update frames are 0x0A, length, command ("W", "S", "D", "E"), data, 16-bit byte sum;
  the DSP answers 0x0A ... 0x4F. 2000 calls without an answer restart it from "W".
- The battery relay frames are "BMS", 1, "HED", 1024 data bytes, a 16-bit byte sum, "E"; the
  last is "BMS", 1, "END" and a CRC-32. Each data frame waits for "OK" for 300 calls.

## Open questions

- How the DSP passes the battery frames to the BMS, and what the BMS checks. The CRC the ARM
  sends is a constant in this image (findings.md), so either the BMS does not check it or the
  relay never worked with this ARM version.
- What the 302 bytes sent to a silent dongle are meant to be; the buffer is never filled.
- The bootloader's rules for the version word at 0x08013FFC.
