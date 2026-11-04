# GivEnergy LV battery BMS 3020

This is my reconstruction of the source of the GivEnergy LV (Gen 2) battery's BMS firmware,
version 3020. I built it to match the vendor's `BMS_ARM.bin` byte for byte with Keil armcc 5.06
at `-O1` for a Cortex-M3, following Exhume's armcc5 profile.

## Status

- The whole image rebuilds byte for byte: image, BIN and HEX are identical to the shipped
  package, with **zero original-byte blobs**. I checked this with my verification gate, which
  builds every source at its native address and compares every relocated byte.
- Every function is C, apart from the assembly startup file and the RTOS port's assembly
  routines. The last function to be matched, the ASCII protocol command dispatcher at
  0x0801DF70 (11,746 bytes), is now `ascii_dispatch_command` in `src/app/comm/ascii_protocol.c`
  ([issue #4](https://github.com/abedegno/forgiv/issues/4)). The section below says how it was
  matched.
- The ASCII protocol unit and the serial unit have readable names and comments like the rest
  of the tree. The command and return codes are named in `src/include/bms_ascii_protocol.h`.

This tree has the sources and the battery documentation. The map, the verification tools and
the matching notes are kept outside this publication.

## How the dispatcher was matched

Matching the dispatcher directly at `-O1` failed. The instructions could be reproduced, but
armcc's register choices over the 11,746 bytes could not.

The answer came from older firmware. The older LV BMS versions, 1037 and 3007 to 3018, have the
same dispatcher built without optimisation (`-O0`), where the source is much easier to recover.
I matched it there exactly, registers included, in 1037, 3007 to 3009 and 3014 to 3018; 3012
and 3013 have the same code as 3009. That source, compiled unchanged at `-O1`, is byte-exact against 3019, 3020, 3021 and 3022.

Between 3018 and 3019 the vendor made three changes that affect this file:

- the optimisation level went from `-O0` to `-O1`;
- the version constant changed;
- the charge and discharge current limits changed from `float` to `unsigned int`.

From 3019 to 3022 only the version constant changes. The `match:` comments in
`ascii_protocol.c` mark the spellings that change the bytes, e.g. the command code must be an
`unsigned char`. The method is described for other projects in
[Exhume](https://github.com/abedegno/Exhume)'s armcc5 profile.

## The image

- **Package:** the vendor's update package is in the public archive at
  github.com/DJBenson/giv-firmware. No firmware binary is committed here.
- **Header:** `cc 0b 66 55`, which is version 3020 and magic 0x5566.
- **Size:** 118,790 bytes after the header.
- **Addresses:** the load address is **0x08014000**, the initial SP is 0x20009BB8 and reset is
  0x08027804. The reset instructions and the standard C library's scatter table each establish
  the load address independently.
- **Versions:** firmware 3019, 3021 and 3022 contain the same ASCII dispatcher code as 3020. Gen 3
  firmware 4009 has a changed version of it. The [3022 tree](../bms-3022/README.md) is built
  from this one.

## Layout

- `src/app` is the application: measurement and protection, control, state of charge, settings
  and calibration, and the communication protocols.
- `src/board` is board support. The ST standard peripheral library sources are not included.
- `docs/fingerprint.md` records how I identified the toolchain.
- `docs/map.md` records how the 118,790 bytes divide into code, literals, data and library
  members.

## Protocols

The BMS speaks two protocols:
- Modbus RTU on USART3 to the inverter;
- an ASCII hex protocol on UART4, between paralleled packs. Frames start with `~` and end with
  a carriage return, and every field between them is hex digits. `ascii_dispatch_command`
  answers each command code.

The open analysis at [open-giv/bms-analysis](https://github.com/open-giv/bms-analysis) describes
both from the outside.

Nothing here flashes a device.
