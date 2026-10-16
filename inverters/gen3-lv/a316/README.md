# GivEnergy Hybrid Gen3 LV inverter: ARM firmware A316

Reading the G3 LV inverter's ARM firmware A316 (`ARMStore.bin` from the public giv-firmware
archive, `Firmware/Inverter/Hybrid Gen3 LV/A316-D316.zip`) with the byte-matching method and tools
of Exhume's armcc5 profile, for understanding: how the inverter treats its battery. Nothing here
is flashed; the aim is not a modified inverter (grid-tied, certified).

The image: a 4-byte header `3c 01 66 55` (version 0x013C = 316), then a Cortex-M3 image loaded at
0x08014000 (SP 0x2000A598, reset 0x0802D6A9). The DSP firmware D316 from the same package is TI
C2000 and is in [../d316](../d316/README.md).

## Status: byte-exact

`tools/gate.py` rebuilds `A316_ARMStore.bin` and `A316.hex` byte for byte from the sources in
`src/` and Keil's own libraries; no original bytes are copied in. `tools/fetch_firmware.py` at the
repository root puts both originals in `firmware/gen3-lv/` (it makes the HEX file, the image less
its header, from ARMStore.bin), where `exhume.toml` points.

| Supplied by | Bytes | Share |
|---|---|---|
| Proved sources (C, plus the ST startup file and the RTOS context switch in assembly) | 143,854 | 99.0% |
| Keil C library members, linked from MDK's libraries | 1,208 | 0.8% |
| Region table and compressed RW data, made by armlink | 248 | 0.2% |
| Alignment padding made by armlink and checked | 10 | 0.0% |
| Original bytes | 0 | 0% |

The sources, under the original file names where they are known (`map/filenames.tsv` gives
each file's old name, its kind of name and the evidence):

    src/app/            main, stm32f10x_it, board, stubs, util
    src/app/drivers/    uart, eeprom, i2c, nvic, panel_led, rtc
    src/app/comm/       dsp_link, modbus_slave, meter
    src/app/control/    monitor, measure, schedule
    src/app/usb/        usb_bsp, usbh_usr, usb_task
    src/cmsis/          system_stm32f10x
    src/rtos/           a uC/OS-II style scheduler: os_core, os_cpu_c, os_cpu_a.s, os_flag, os_init, os_task, os_time
    src/lib/fatfs/      FatFs R0.12c ff.c
    src/lib/usb_otg/    ST's USB OTG driver: usb_core, usb_hcd, usb_hcd_int
    src/lib/usb_host/   ST's USB host library: core/, msc/, cdc/
    src/clib/           two Keil C library members also built from C
    src/stdperiph/      12 StdPeriph V3.5.0 files (rcc built with `HSE_VALUE` 8000000)
    src/startup/        the ST startup file

Where one original file compiles to different bytes as one unit than the image holds, it stays in
parts named `<name>_pN.c`, N the first part it holds (main, schedule, ff; the reasons are in
map/filenames.tsv). Every tool finds a source by walking `src/`, by its stem (unique across `src/`)
or by an address on its at: lines (`tools/srcfiles.py`), so a source can be renamed or moved
with `mv` alone.
One 45 KB file is over MDK-Lite's 32 KB link limit and is proved by placing armcc's own object at
its address (`tools/objplace.py`, docs/gate.md). The sources started machine-shaped (drafts from
Ghidra and from an -O0 translator, whose reusable form is in Exhume's armcc5 profile); functions
and most globals now have names, shared headers, structs and named constants (docs/readability.md);
comments come next. How each part was matched: docs/matching.md.
Bugs, dead code, recovered rules and open questions from the comment pass: docs/findings.md.
The view across both chips (architecture, start-up, energy modes, the registers, the ARM-DSP link
byte by byte) is in [docs/README.md](../../../docs/README.md) at the repository root.

    $EXHUME/.venv/bin/python tools/gate.py   # about 1 minute, PASS

## Running it

What is not in this repository, and where the tools look for it (tools/a316env.py; each is an
environment variable, or a key in `toolchain.local.toml` here, which starts from
`toolchain.example.toml`):

| Setting | What | Default |
|---|---|---|
| `EXHUME` | an [Exhume](https://github.com/abedegno/Exhume) checkout, with its venv | `../exhume` or `../Exhume` beside the repository |
| `MDK_TOOLCHAIN` | a directory of Keil MDK homes named `MDKnnn` | `toolchain/` here (a directory or a link) |
| `MDK_HOME` | the compile home: MDK 5.21 or later (armcc 5.06) | `MDK521` in `MDK_TOOLCHAIN` |
| `MDK_LIBHOME` | the library home: MDK 5.16 or later | `MDK516` in `MDK_TOOLCHAIN` |
| `WINE_BIN` | the directory with `wine` and `wineserver` | `wine` on `PATH`, else Wine Devel's on macOS |

ST's StdPeriph library V3.5.0 goes at `lib/stdperiph35` (unpacked, or a link to it): the twelve
`src/stdperiph` files include ST's driver sources from there unchanged.
[docs/development/toolchains.md](../../../docs/development/toolchains.md) at the repository root
says where to get each piece.

```sh
python3 ../../../tools/fetch_firmware.py            # once: the images
export EXHUME=/path/to/Exhume MDK_TOOLCHAIN=/path/to/mdk-homes
$EXHUME/.venv/bin/python tools/gate.py               # the whole gate; --fast reuses what is unchanged
```

## Fingerprint

From scripted searches made during the matching work, no hand matching (the scripts were
removed after v1.0.0 and are in the `v1.0.0` tag; Exhume's armcc5 profile has the reusable
fingerprint tools):

| Question | Answer | How |
|---|---|---|
| Compiler | Keil armcc 5, standard C library (not MicroLIB) | `__main` runs armcc's `__scatterload` and `__decompress`; a library search finds 17-18 members of `c_w.l`/`h_w.l` exactly |
| Optimisation | `-O0` | a search of the ST library under each build: 145 functions exact at -O0, 11-12 at -O1..-O3 |
| ST library | StdPeriph V3.5.0 | 145 functions of V3.5.0 exact |
| Release | compiler MDK 5.21 or later (armcc 5.06); library MDK 5.16 or later | The compiler: armcc 5.05 (MDK 5.11 to 5.15) writes a flag-setting copy as `movs rd, rn` where the image has `subs rd, rn, #0` (7 places, docs/matching.md, round 4 range C), and MDK 5.16 to 5.20 add a `cmp` to a switch dispatch in meter.c (then f0802B028.c) that 5.21 and later do not (range B), so the compile home is MDK521. The library: its startup and exit code (`exit.o`) as linked is MDK 5.16 to 5.36's; 5.11 to 5.15's `exit` is shorter (tools/gate.py, `--libhome`), so the gate links it from MDK516; the later releases link the same bytes. MDK 4.x match 131 of the 145 ST functions. An earlier "5.11-5.15" rested on a 16-byte `abort` match at 0x08023226 that is application code |

The searches took every `MDKnnn` home in `MDK_TOOLCHAIN`. tools/a316env.py sets
`EXHUME_ARMCC_RUNNER=wine` unless it is set already.
