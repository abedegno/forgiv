# forgiv

[![gate](https://github.com/abedegno/forgiv/actions/workflows/gate.yml/badge.svg?branch=main)](https://github.com/abedegno/forgiv/actions/workflows/gate.yml)

Byte-exact decompilations of GivEnergy firmware: C and assembly source that the original
compiler and linker turn back into the shipped firmware image, byte for byte. I made them to
understand how the equipment works, in particular how the inverter treats its battery.

The repository holds the Hybrid Gen3 LV inverter's ARM and DSP firmware at versions 316
and 319, plus the LV battery BMS at versions 3020 and 3022. Each target has its own source and proof.
Inverter version 318 is pinned and surveyed, without a source reconstruction.

## Status

| Firmware | Chip | Toolchain | Byte-exact | From proved source | Original bytes (blobs) | Readable |
|---|---|---|---|---|---|---|
| [Gen3 LV inverter, ARM A316](inverters/gen3-lv/a316/README.md) | STM32F105/107 (Cortex-M3) | Keil armcc 5.06 (MDK 5.21), MDK 5.16 library | yes: `ARMStore.bin` and its HEX form | 143,854 of 145,320 bytes (99.0%); the rest is Keil's C library, the region table and padding made by armlink | 0 | yes: names, headers, comments |
| [Gen3 LV inverter, DSP D316](inverters/gen3-lv/d316/README.md) | TMS320F2806x (C28x with FPU) | TI CGT 16.9.6, 15.12.3 runtime library | yes: `DSPStore.bin`, word for word | 48,555 of 65,536 words (74.1%); 1.9% TI's runtime library, 24.0% erased-flash fill | 0 | yes: names, headers, comments |
| [Gen3 LV inverter, ARM A319](inverters/gen3-lv/a319/README.md) | STM32F105/107 (Cortex-M3) | Keil armcc 5.06, MDK 5.16 library | yes: BIN and HEX, including a licensed whole-image link | 146,030 of 147,536 payload bytes; runtime, region table, compressed initial RAM and alignment account for the rest | 0 | yes: subsystem files, named interfaces and comments |
| [Gen3 LV inverter, DSP D319](inverters/gen3-lv/d319/README.md) | TMS320F2806x (C28x with FPU) | TI CGT 16.9.6, 15.12.3 runtime library | yes: native and Windows linker builds | 48,714 of 65,536 words; runtime, initialization terminator and erased-flash fill account for the rest | 0 | yes: subsystem files, named interfaces and comments |
| [LV battery BMS 3020](batteries/lv/bms-3020/README.md) | Cortex-M3 | Keil armcc 5.06 / armasm | yes: BIN and HEX, including a licensed whole-image link | 112,628 of 118,790 bytes; the rest is Keil's C library, the region table and padding | 0 | yes, apart from one ASCII dispatcher in assembler pending its C match |
| [LV battery BMS 3022](batteries/lv/bms-3022/README.md) | Cortex-M3 | Keil armcc 5.06 / armasm | yes: BIN and HEX, including a licensed whole-image link | 113,720 of 119,814 bytes; the rest is Keil's C library, the region table and padding | 0 | yes, apart from the same ASCII dispatcher in assembler |

"Byte-exact" means each tree's gate (`tools/gate.py`) compiles every source, links the whole
image with the original vendor's tools and compares it with the vendor's file, and says
IDENTICAL. "Original bytes 0" means no uncovered flash ranges are copied in as opaque blobs.
The gates account separately for source, toolchain libraries, linker output and initial RAM.
This proves reconstruction, not that every register's meaning is known.

## Quick start

You need the firmware images (fetched below), the toolchains
([docs/development/toolchains.md](docs/development/toolchains.md)) and, for the ARM targets, a checkout of
[Exhume](https://github.com/abedegno/Exhume), the toolkit the matching was done with. Python 3.11
or later. [docs/development/building.md](docs/development/building.md) has the details.

```sh
python3 tools/fetch_firmware.py          # downloads the package, checks the sha256s, fills firmware/

# A316 (ARM)
cd inverters/gen3-lv/a316
export EXHUME=/path/to/Exhume MDK_TOOLCHAIN=/path/to/mdk-homes
$EXHUME/.venv/bin/python tools/gate.py   # about 1 minute; ends with "-- gate: PASS"

# D316 (DSP)
cd ../d316
export C2000_WINEPREFIX=/path/to/wine-prefix C2000_CGT=/path/to/ti-cgt-c2000_16.9.6.LTS \
       C2000_RTSLIB=/path/to/c2000_15.12.3.LTS/lib/rts2800_fpu32.lib \
       C2000_NATIVE=/path/to/ti-cgt-c2000_18.12.8.LTS/bin
python3 tools/gate.py                    # about 2 minutes; ends with "IDENTICAL" and "PASS"
```

Instead of environment variables, each tree takes a `toolchain.local.toml` (gitignored; copy
its `toolchain.example.toml`). Each tree's README lists its settings and their defaults.

`python3 tools/repocheck.py` runs the checks that need neither toolchain nor images (CI runs it).

For 319, use the [319 build and source guide](inverters/gen3-lv/docs/319-readability.md).
Its acceptance includes the licensed ARM whole link and both DSP linkers. The commands above
remain the 316 examples; choose a target explicitly before fetching or rebuilding.

## Layout

    README.md, LICENSE, NOTICE.md     this page, the licence of my own work, what is not mine
    CONTRIBUTING.md                   how to add a firmware
    firmware/manifest.toml            where each image comes from, with its sha256
    tools/fetch_firmware.py           fetches and checks the images, puts them in firmware/
    tools/repocheck.py                repository checks for CI
    docs/                             the documentation across both chips (below)
    docs/development/toolchains.md    installing the toolchains
    docs/development/method.md        how byte-exact matching works here
    inverters/gen3-lv/                the Hybrid Gen3 LV inverter (its README: the two chips)
      a316/                           the ARM firmware A316: src, map, tools, docs
      d316/                           the DSP firmware D316: src, map, tools, docs
      a319/, d319/                    the version 319 reconstructions and proof manifests
      docs/                           version survey, 319 handover and source guide
    batteries/lv/bms-3020/, bms-3022/  the LV battery BMS 3020 and 3022: sources and docs; one
                                      dispatcher remains assembler

Each firmware directory is self-contained: its own sources (`src/`), map of the image (`map/`),
tools (`tools/`, with the gate), notes (`docs/`) and toolchain configuration. The battery
trees publish their sources and documentation only; their maps and gates are not included.

## Documentation

Start at [docs/README.md](docs/README.md), which has reading paths for owners, developers and
researchers. The cross-chip operation pages describe the 316 baseline; the
[319 handover](inverters/gen3-lv/docs/319-handover.md) records the recovered changes.
Each baseline rule names the function
and file it comes from:

- **system**: [architecture](docs/system/architecture.md), [hardware](docs/system/hardware.md),
  [update packages](docs/system/update-packages.md);
- **operation**: [start-up](docs/operation/startup.md), [states](docs/operation/states.md),
  [energy modes](docs/operation/energy-modes.md), [battery](docs/operation/battery.md),
  [grid](docs/operation/grid.md), [control](docs/operation/control.md),
  [faults](docs/operation/faults.md), [communication](docs/operation/comms.md);
- **reference**: [registers](docs/reference/registers.md),
  [the ARM-DSP link byte by byte](docs/reference/arm-dsp-link.md),
  [fault bits](docs/reference/fault-bits.md);
- **[findings](docs/findings.md)** across both chips, by confidence;
- **development**: [building](docs/development/building.md),
  [modding](docs/development/modding.md), [method](docs/development/method.md),
  [conventions](docs/development/conventions.md),
  [adding a firmware](docs/development/adding-a-firmware.md).

```mermaid
flowchart LR
    app["App, cloud"] --> dongle["Dongle"]
    dongle -- "USB" --> arm["ARM A316<br/>settings, schedules,<br/>Modbus, meters"]
    arm -- "RS485" --> meters["Meters"]
    arm <-- "UART, 45-byte frames" --> dsp["DSP D316<br/>power stage, grid,<br/>protection"]
    dsp -- "RS485 Modbus" --> bms["Battery BMS"]
    dsp -- "PWM" --> power["PV, battery,<br/>grid and EPS stages"]
```

The ARM decides when to charge, discharge or export; the DSP runs the power stage, applies every
limit and is the only chip on the battery bus. Each tree also has its own notes:
[inverters/gen3-lv/README.md](inverters/gen3-lv/README.md) for the pair,
[a316](inverters/gen3-lv/a316/README.md) and [d316](inverters/gen3-lv/d316/README.md) for
each chip, with their `docs/battery.md` and `docs/findings*.md`.

## Safety

Nothing here is meant to be flashed to an inverter. The Gen3 LV is a grid-connected, certified
device; a changed firmware could damage it, the battery or the installation, and would not meet
the grid code the original was certified against. This repository rebuilds the original image
to prove that the source is right, and that is all I have tested. See [SECURITY.md](SECURITY.md).

## No firmware images

The repository holds no firmware images and no toolchain files. The images come from the public
archive [DJBenson/giv-firmware](https://github.com/DJBenson/giv-firmware);
`tools/fetch_firmware.py` downloads them and checks them against `firmware/manifest.toml`. The
compilers come from Keil (Arm) and TI, from their public download servers.

## Links

- [Exhume](https://github.com/abedegno/Exhume): the byte-matching toolkit and its Claude Code
  skills, with the armcc5 and ti-c2000 profiles these trees were matched with.
- [DJBenson/giv-firmware](https://github.com/DJBenson/giv-firmware): the firmware archive.
- [open-giv/bms-analysis](https://github.com/open-giv/bms-analysis): an open analysis of the
  GivEnergy LV battery and its BMS protocol (Ghidra and emulation), cited in the A316 notes.

## Licence

My own work here (tools, notes, build files, the structure and comments of the sources) is under
the MIT licence ([LICENSE](LICENSE)). The decompiled sources reproduce GivEnergy's firmware and
include code derived from third-party libraries, which are not mine to license:
[NOTICE.md](NOTICE.md) says what and under which terms.
