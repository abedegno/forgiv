# GivEnergy LV battery BMS 3022

This is my reconstruction of the source of the GivEnergy LV (Gen 2) battery's BMS firmware,
version 3022. It is built from the [3020 tree](../bms-3020/README.md), with 3021 used as a
stepping stone between the two, and matches the vendor's `BMS_ARM.bin` byte for byte with
Keil armcc 5.06 at `-O1` for a Cortex-M3.

## Status

- The whole image rebuilds byte for byte: image, BIN and HEX are identical to the shipped
  package, with **zero original-byte blobs**. I checked this with my verification gate: all 79
  compilation units prove at their native addresses, and a licensed whole-image link of
  113,720 source bytes passes too.
- As in 3020, one function, the ASCII protocol command dispatcher, is still in assembler
  (`src/app/comm/ascii_dispatcher.inc`), pending a byte-exact C version
  ([issue #4](https://github.com/abedegno/forgiv/issues/4)). The nine functions around it are
  C, in `src/app/comm/ascii_protocol.c`. Everything else is C.

This tree has the sources and the battery documentation. The map, the verification tools and
the matching notes are kept outside this publication.

## The image

- **Package:** the vendor's update package is in the public archive at
  github.com/DJBenson/giv-firmware. No firmware binary is committed here.
- **Size:** `BMS_ARM.bin` is 119,818 bytes: a four-byte version and magic header (version
  3022, magic 0x5566), then 119,814 bytes loaded at **0x08014000**.
- **Addresses:** the initial SP is 0x20009BD0 and reset is 0x08027C48 (Thumb entry
  0x08027C49).
- **RAM:** initialised RAM is 1,716 bytes, 24 more than 3020. Zero-initialised RAM is
  38,172 bytes, unchanged.

## What changed from 3020

The vendor's release notes describe 3021 adding battery heating and changing calibration and
cold-current handling. They describe 3022 changing state-of-charge behaviour at minor
overvoltage, and stopping balancing during a cell-detection fault. The code shows:

- **Staged battery heating.** Holding register 1 bit 2 enables it, and the stage bits are
  reported in holding register 15. It works from the minimum cell temperature, the state of
  charge and the signed pack current. Temperatures are stored in tenths of a kelvin, so 2730
  is 0 degC. Heating restarts below 2750 (2 degC) and is complete at 2780 (5 degC). The interface
  is `src/include/bms_heating.h` and the controller is in `src/app/control/control_tasks.c`.
  The stage names follow the polarity of the measured current.
- **Secondary current:** a timed substitution of the secondary current measurement, with two
  new counters and two status bytes.
- **Cell consistency:** a debounce that counts failing samples and clears on a good one.
- **Balancing:** both balancing loops skip cells while the cell-detection fault bit is set.
- **Protection:** the forced protection service now runs every 40 task passes instead of
  every 2,000.

## Layout

- `src/app` is the application: measurement and protection, control and heating, state of
  charge, settings and calibration, and the communication protocols.
- `src/board` is board support. The ST standard peripheral library sources are not included;
  the files in `src/stdperiph` only include the external library.

The names are descriptive names I chose for the recovered roles, not the vendor's own.

Nothing here flashes a device.
