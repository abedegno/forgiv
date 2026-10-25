# GivEnergy LV battery BMS 3020

This is my reconstruction of the source of the GivEnergy LV (Gen 2) battery's BMS firmware,
version 3020. I built it to match the vendor's `BMS_ARM.bin` byte for byte with Keil armcc 5.06
at `-O1` for a Cortex-M3, following Exhume's armcc5 profile.

## Status

- The whole image rebuilds byte for byte: image, BIN and HEX are identical to the shipped
  package, with **zero original-byte blobs**. I checked this with my verification gate, which
  builds every source at its native address and compares every relocated byte.
- One function, the ASCII protocol command dispatcher `sub_0801DF70` (11,746 bytes), is still in
  assembler (`src/app/comm/ascii_dispatcher.inc`), pending a byte-exact C version
  ([issue #4](https://github.com/abedegno/forgiv/issues/4)). The other
  nine functions in that unit are C, in `src/app/comm/ascii_protocol.c`. The build compiles
  that C to assembly and combines it with the dispatcher in one code area, using the layout in
  `ascii_layout.json`.
- Everything else is C.

This tree has the sources and the battery documentation. The map, the verification tools and
the matching notes are kept outside this publication.

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
- an ASCII hex protocol in the Pylontech v2.5 family on UART4, between paralleled packs. Its
  CID2 dispatcher is the one function still in assembler.

The open analysis at [open-giv/bms-analysis](https://github.com/open-giv/bms-analysis) describes
both from the outside.

Nothing here flashes a device.
