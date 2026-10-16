# Notice: what is not under the MIT licence

The MIT licence in [LICENSE](LICENSE) covers my own work here: the tools, the documentation and
notes, the build and configuration files, the map files, and the names, comments and structure I
added to the sources. It does not cover what is listed below. I make no claim to any of it.

## Purpose

This repository exists for interoperability and research: to understand how the GivEnergy Hybrid
Gen3 LV inverter's firmware works, in particular how it talks to and controls its battery, so that
owners can keep using, repairing and connecting to equipment they own. The sources are proved
against the vendor's images, which is what makes them a reliable description of that behaviour.

No firmware images are included. The images the gates compare with are downloaded by
`tools/fetch_firmware.py` from the public archive github.com/DJBenson/giv-firmware and are never
committed. No compilers, libraries or other files of Keil (Arm), Texas Instruments or
STMicroelectronics are included either; each user installs those from the vendor
([docs/development/toolchains.md](docs/development/toolchains.md)).

## The firmware

The decompiled sources under `inverters/gen3-lv/a316/src` and `inverters/gen3-lv/d316/src`
reproduce GivEnergy's A316 and D316 firmware: compiled, they give the vendor's images byte for
byte. The firmware is copyright GivEnergy Ltd (in administration) or its successors in title.
The sources are an independent re-expression of it, written from the images, with my names and
comments; they contain no source code obtained from GivEnergy.

## Third-party code in the trees

What each tree holds, as it is in the files:

### A316 (`inverters/gen3-lv/a316/src`)

- **STMicroelectronics STM32F10x Standard Peripheral Library V3.5.0.**
  - `src/startup/startup_stm32f10x_cl.s` is ST's startup file with two sizes changed; it keeps
    ST's copyright header.
  - `src/cmsis/system_stm32f10x.c` is ST's CMSIS system file, re-expressed from the image with the
    vendor's clock changes.
  - `src/stdperiph/*.c` are twelve short wrappers that `#include` ST's driver sources unchanged
    from `lib/stdperiph35`. ST's library itself is not in the repository; each user downloads it
    from ST.
  - Terms: ST's licence for the library (the V3.5.0 files carry ST's "for guidance only" notice;
    later ST packages of the library are under ST's own licence agreement).
- **STMicroelectronics STM32 USB OTG host library** (about V2.2.0): `src/lib/usb_otg/` and
  `src/lib/usb_host/` (core, MSC and CDC classes) are re-expressed from the image, following ST's
  function names and structure. Terms: ST's licence for that library.
- **FatFs R0.12c by ChaN**: `src/lib/fatfs/ff_p01.c` and `ff_p09.c` are ff.c re-expressed from the
  image, in ChaN's order and shape. Terms: FatFs's own BSD-style licence, whose notice both files
  keep in their header comment:

      Copyright (C) 2017, ChaN, all right reserved.

      FatFs module is an open source software. Redistribution and use of FatFs in
      source and binary forms, with or without modification, are permitted provided
      that the following condition is met:

      1. Redistributions of source code must retain the above copyright notice,
         this condition and the following disclaimer.

      This software is provided by the copyright holder and contributors "AS IS"
      and any warranties related to this software are DISCLAIMED.
      The copyright owner or contributors be NOT LIABLE for any damages caused
      by use of this software.

- **Arm (Keil) C library**: the gate links library members from the user's own MDK install;
  none are in the repository. `src/clib/keil_fpinit.c` and `keil_heapauxi.c` are short C
  stand-ins for two members, written to give the same bytes, not Arm's source.
- **uC/OS-II (Micrium, now under the Apache License 2.0)**: `src/rtos/` is the firmware vendor's
  own small scheduler, written in the shape of uC/OS-II's Cortex-M3 port and using its names
  (OSTCBCur, OS_Sched, OS_CPU_SR_Save and so on). It is not Micrium's code; the names and the
  port's shape are theirs.
- **Modbus CRC-16 tables**: `src/app/comm/modbus_slave.c` holds the standard table-driven
  CRC-16/MODBUS tables, as published in the Modbus over serial line specification.

### D316 (`inverters/gen3-lv/d316/src`)

- **Texas Instruments F2806x device support files** (controlSUITE / C2000Ware header and
  peripheral examples): `src/ti/` (CodeStartBranch.asm, F2806x_CSMPasswords.asm,
  F2806x_usDelay.asm, F2806x_DefaultIsr.c, F2806x_Dma.c, F2806x_MemCopy.c, F2806x_PieVect.c,
  F2806x_SysCtrl.c) and the `F2806x_*.h` register headers in `src/include/`, re-expressed from the
  image under TI's file and function names; some match TI's text. Terms: TI's licence for those
  files (TI's BSD-style licence in current C2000Ware).
- **TI Flash API declarations**: `src/include/Flash2806x_API_Library.h` declares the boot ROM's
  flash API calls the update routine makes, as TI's header does. The flash API itself is in the
  chip's ROM and is not here. Terms: TI's licence for the Flash API.
- **TI C2000 runtime library**: the gate links `rts2800_fpu32.lib` members from the user's own
  CGT install; none are in the repository.
- **uC/OS-II names**: `src/rtos/` is a small custom kernel with uC/OS-like names (OS_CPU_SR save
  and restore, the context switch); not Micrium's code.
- **Modbus CRC-16 tables**: `src/app/comm/crc16.c` holds the standard CRC-16/MODBUS tables (auchCRCHi and
  auchCRCLo, as published in the Modbus over serial line specification).

### Tools

- `inverters/gen3-lv/a316/tools/vendor/rwdata.py` is a copy of a module of
  [Exhume](https://github.com/abedegno/Exhume) (MIT).

## Trademarks

GivEnergy is a trademark of its owner. STM32, Keil, Arm, Texas Instruments, C2000, Modbus and
the other names here belong to their owners. This project is not affiliated with or endorsed by
any of them.
