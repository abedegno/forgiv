# GivEnergy Hybrid Gen3 LV inverter: DSP firmware D316

This is a byte-exact decompilation of the inverter's DSP firmware D316 (`DSPStore.bin` from the
package `A316-D316.zip`, the same package as the ARM side in [../a316](../a316/README.md);
`tools/fetch_firmware.py` at the repository root puts it at `firmware/gen3-lv/D316_DSPStore.bin`).
The DSP is a TI C2000 part of the TMS320F2806x family with the FPU. The aim is the same as for
A316: C source that TI's own compiler and linker turn back into the image word for word, so that I
can read how the inverter treats its battery. Nothing here is ever flashed to an inverter.

The image is 131,072 bytes: 64K 16-bit words, big-endian in the file, at word addresses
0x3E8000 to 0x3F7FFF. Word 0 is the version, 0x013C (316). The flash entry at 0x3F7FF6 is
`LB 0x3F5226`, a small boot loader, which starts the application at 0x3E8004 when the word at
0x3F3FFC is 0xAA. docs/notes.md has the whole layout.

## Status: byte-exact, every word from source

`tools/gate.py` compiles everything under `src/` (C, plus six assembly files that were assembly in
the original), links the whole image with TI's linker and gets `D316_DSPStore.bin` back word for
word, with no blobs of original words.

| Supplied by | Words | Share |
|---|---|---|
| Proved sources (src/) | 48555 | 74.1% |
| Runtime library members | 1240 | 1.9% |
| Made by the linker (end of .cinit) | 2 | 0.0% |
| Fill 0xFFFF (holes) | 15739 | 24.0% |
| Original words (blobs) | 0 | 0.0% |

The assembly sources are TI's CodeStartBranch, usDelay and CSM password files, the RTOS context
switch and its critical-section helpers, and the boot loader's entry `boot_main` (it writes
registers before it has a stack frame and has no epilogue, which no C compiler produces). Every
other function is C. docs/notes.md has what matching taught, docs/hard.md how each hard function
was matched.

## Reading the source

- [docs/battery.md](docs/battery.md): how the DSP handles the battery, from the matched C: the BMS
  link, every current limit and taper with its formula, SoC floor and forced charge, voltage
  trips, soft start, calibration, and what a third-party BMS bridge must provide.
- [docs/findings.md](docs/findings.md): likely bugs, dead code, mismatches with the ARM, open
  questions.
- docs/subsystems/: one page per area (arm_link, battery, boot, control, drivers, grid, meas,
  power, rcmu, rtos).
- [docs/README.md](../../../docs/README.md) at the repository root: the view across both chips
  (architecture, start-up, states, energy modes, the registers, the ARM-DSP link byte by byte).
- docs/readability.md: how the matched tree was made readable without changing a word: names
  (map/names.tsv), headers, constants, file layout (map/filenames.tsv), comments.
- Every file has a header comment and every function a comment; `match:` notes explain code
  shapes kept for byte matching, `name:` notes the evidence for a name.

The compiler is CGT 16.9.6 for every source (16.9.5 gives the same words); the runtime library members
are 15.12.0-15.12.3's. toolchain.toml has the evidence. docs/notes.md has what matching taught.

The map (`map/functions.tsv`, `map/data.tsv`) covers every word of the image:

    python3 tools/coverage.py

| Functions | Count | Words |
|---|---|---|
| c | 398 | 38,790 |
| ramfunc (copied to RAM) | 50 | 6,618 |
| rts (library members) | 21 | 1,184 |
| asm | 6 | 63 |

| Data | Words |
|---|---|
| .cinit (414 records and the end words) | 2,006 |
| constants (CRC tables, version, app valid flag) | 514 |
| PieVectTableInit | 256 |
| switch tables (4) | 178 |
| CSM reserved words and passwords | 126 |
| function pointer tables (2) | 34 |
| library constants (expf) | 28 |
| fill 0xFFFF | 15,739 |

data.tsv also lists 1,325 RAM variables and 356 peripheral registers found from direct-page and
address references, with sizes where the code shows them.

## Toolchain

- **Compiler**: TI C2000 code generation tools 16.9.6.LTS (16.9.5 gives the same words), COFF
  ABI, `-v28 -ml -mt --float_support=fpu32 -O1`, the Windows build under Wine (Wine Devel on a
  Mac). The tree was first matched with 15.12.x and moved to 16.9.6 later; toolchain.toml has the
  evidence and docs/notes.md the history.
- **Runtime library**: `rts2800_fpu32.lib` of 15.12.0 to 15.12.3 (15.12.4 changed `expf`), so the
  15.12.3.LTS Windows install is needed too, for its library only.
- **Native tools**: the macOS build of 18.12.8.LTS for ofd2000, dis2000, hex2000, and the linker
  that match.py uses (it gives the same image as 15.12's).
- **Python** 3.11 or later (tomllib). Nothing else.
- **For tools/emudiff.py only**: Ghidra 12 with the
  [ghidra-tms320c28x](https://github.com/outlandnish/ghidra-tms320c28x) extension installed, and
  a JDK 21 (`GHIDRA_INSTALL_DIR` and `JAVA_HOME`; the defaults are Homebrew's).

[docs/development/toolchains.md](../../../docs/development/toolchains.md) at the repository root
says where to get and how to install each.
`toolchain.toml` holds the default paths, the flags and the Wine settings. Point the tools at
your installs with the environment variables `C2000_CGT`, `C2000_RTSLIB`, `C2000_NATIVE`,
`C2000_WINEPREFIX`, `WINE_BIN` (and `C2000_OTHERS` for `match.py --all-cgt`), or with a
`toolchain.local.toml` beside it (gitignored; start from `toolchain.example.toml`). The tools find
Wine through it, so nothing has to be on `PATH`. Each tool run takes a Wine prefix of its own
(`build/wine/jobN`, an APFS clone of the master prefix (`cp -Rc`, so macOS), held with a lock), so tools can run side
by side. The gate stops the Wine servers when it ends; servers that match.py starts exit by
themselves a minute after their last use, or with `wineserver -k`.

## Tools

    python3 tools/dis.py 3EAC5D +20          # dis2000 on any word range (or a map name)
    python3 tools/dis.py InitFlash           # a ramfunc is shown at its run address
    python3 tools/match.py crc16 [FUNC]      # compile and compare each function with the image
                                             # (a path, or a source's name or stem under src/)
    python3 tools/match.py SRC --all-cgt     # the same with every installed compiler
    python3 tools/gate.py                    # prove src/, link the whole image, compare (about 10 s)
    python3 tools/gate.py --with try/x.c     # the gate with try/x.c in place of the src/ file x.c
    python3 tools/coverage.py                # the map covers every word once
    python3 tools/libfind.py                 # where the runtime library's members are
    python3 tools/mkmap.py                   # rebuild the map (keeps names edited by hand)
    python3 tools/modbuild.py                # the modding build: a working DSPStore.bin that may differ
    python3 tools/emudiff.py [IMAGE]         # run pure functions of a mod image and the original side by side

`match.py` compiles with `--gen_func_subsections=on`, so each function is a section of its own,
and links the object with each section at its map address. Every symbol the file uses and does
not define is set to its map address; a name the map does not have resolves only if it carries
its address (`sub_3EAC5D`, `var_D8E2`, `c_3F3E78`). So relocated words are filled in by the
linker, not masked, and must come out as the image's words. It prints EXACT, or the first
differences with both disassemblies side by side, and UNRESOLVED for a symbol it cannot place.
A file's static variables are placed where the image's words put them, with a note.

## The modding build

The gate proves the sources; `tools/modbuild.py` lets them change. It compiles every source and
links the image with TI's linker from an ordinary command file, so functions may change size
and new ones may be added, and it writes `build/mod/DSPStore.bin` in the package's format. It
does not compare the result with the original. Run the gate first: the mod build compares its
objects with the gate's.

    python3 tools/modbuild.py                          # src/ as it is
    python3 tools/modbuild.py --patch FILE             # FILE applied to a copy of src/ (build/mod/tree)
    python3 tools/modbuild.py --src DIR                # another copy of src/
    python3 tools/modbuild.py --shift 0x41             # everything that floats moved up 0x41 words
    python3 tools/modbuild.py --out PATH               # write the image elsewhere
    python3 tools/emudiff.py build/mod/DSPStore.bin    # 105 calls of 18 functions in both images

**Fixed** (the boot loader and the update path depend on them): the version word at 0x3E8000,
which the ARM compares with the DSP's version; `app_entry` at 0x3E8004, where `boot_main` jumps;
`app_valid` = 0xAA at 0x3F3FFC, without which `boot_main` does not start the application; and all
of sector A (0x3F4000 to 0x3F7FFF: the boot loader with its entry `boot_main` and the update
routine it runs at 0x0400, `codestart`, the CSM words), which the update never writes, so the
build checks it is the original's word for word. RAM stays as the original has it: each file's
`.ebss`, the library's data, the stack at 0xE390, the registers, `PieVectTable` at 0x0D00.

**Floating**: `.text`, the ramfuncs' load image, `.cinit` and `.econst` with the switch tables, in
that order, in 0x3E8008 to 0x3F3FFB (sectors D, C, B), each in the original's order with anything
new at the end. The ramfuncs run in 0x8000 to 0xBFFF; the linker defines `RamfuncsLoadStart`,
`RamfuncsLoadSize` and `RamfuncsRunStart` for `init_system`'s copy. A new source file's
variables go in 0x10000 to 0x13FFF. A file already in the image must keep its variables where
they are, so new variables go in a new file.

**Output**: `build/mod/DSPStore.bin`, 131,072 bytes: 0x3E8000 to 0x3F7FFF as 16-bit words, high
byte first, holes 0xFFFF, the form of the file in `A316-D316.zip`. Beside it, the linker's map and
XML (DSPStore.map, DSPStore.xml). The summary gives the free space of each region: the
application flash has only 199 words free.

**Checks**, on every build: no unresolved symbol and every map function present; the fixed words;
each object section against the gate's (identical, or listed with the words that changed); every
unchanged section against the original image, where a word may differ only in a relocated field
that points to the same target moved; and every call, long branch, `MOVL XARn,#` and table pointer
into what floats is a relocated field, so no address is written as a number. Built from an
unchanged tree the image is the original word for word. With `--shift 0x41`, 47,372 words differ
and every check passes, and emudiff finds every call agrees. The example in
[examples/mod-example](examples/mod-example/rcmu-limit.patch) makes the residual current limit
stricter (0.3 A to 0.25 A) and sets version 317.
[docs/development/modding.md](../../../docs/development/modding.md) has the details.

**Safety.** A changed image is no longer the certified firmware, and the DSP's protections are
the only ones between the power stage and the grid or the battery. Nothing here has been run on
hardware, and nothing here flashes anything. The modding page's "Safety first" applies to every
image this build writes.

## How to match a function

1. Look it up in `map/functions.tsv` and read it: `python3 tools/dis.py NAME`.
2. Write C in a file under `src/` (or anywhere while trying). Call other functions and use
   variables by their map names; rename a map row when its meaning is known (match.py and the
   gate read names from the map, so a rename needs no other change). A ramfunc needs
   `#pragma CODE_SECTION(f, "ramfuncs")`. A line `// cflags: ...` adds compiler flags for the file.
3. `python3 tools/match.py FILE` until every function is EXACT.
4. Move it into `src/` and run `python3 tools/gate.py`: it must say IDENTICAL and PASS.

## Layout

    toolchain.toml the flags, the image's path and the default tool paths
    map/           functions.tsv and data.tsv: addr (hex), size (words), name, kind, notes
    src/           proved sources, by subsystem (below); src/include for headers
    tools/         the tools; d316.py is the shared code; tools/emu: emudiff.py's Ghidra script
    docs/notes.md  flags, sections, linker behaviour, the image layout, pitfalls
    build/         everything the tools write (safe to delete); build/mod: the modding build
    examples/      a sample mod for tools/modbuild.py --patch

The sources are in directories by subsystem. map/filenames.tsv lists every file with its old
name (from the matching agents), its new one and the evidence for the name.

    src/ti/            TI's F2806x support files: CodeStartBranch.asm, F2806x_CSMPasswords.asm,
                       F2806x_usDelay.asm, F2806x_DefaultIsr.c, F2806x_Dma.c, F2806x_MemCopy.c,
                       F2806x_PieVect.c, F2806x_SysCtrl.c (InitFlash)
    src/boot/          the boot loader and the application header words it reads
    src/rtos/          the RTOS: scheduler and main, tasks, events, tick, context switch
    src/drivers/       device_init.c (init_system and the peripheral set-up), the SCI drivers,
                       the PWM forcing
    src/app/grid/      grid_support.c: task 4, grid protection and the grid-code responses
    src/app/power/     power_limits.c (task 2) and the MPPT
    src/app/battery/   bms_link.c: task 6, the BMS Modbus master
    src/app/comm/      arm_link.c (task 5), the page 6/7 handlers, the Modbus CRC
    src/app/meas/      task 3 and the DC measurements, task 1 and the AC measurements, the
                       ramfunc ADC scaling, averages and RMS sums
    src/app/rcmu/      the residual current monitor and its self test
    src/app/state/     task 0, the operating states, the application start-up
    src/app/settings/  the settings defaults and the rating presets
    src/app/control/   the converter control module (control_isr) and its ramfunc loops
    src/app/util/      the -Ooff helpers, rate_limit, the IQ constants
                       (tools/ebss_solve.py now works from markers in the sources)

Each object is named after its file's stem, so no two sources share a stem (the gate checks).

The function kinds are `c`, `rts` (a runtime library member), `asm` (hand assembly) and
`ramfunc` (loaded in flash, run from RAM; the notes give `run=0x....`). The names come from my
earlier Ghidra research notes (not published) where they described a function, from TI's F2806x
support files where the code is clearly theirs, and, since the readability pass, from what the
code does (map/names.tsv has every name and its evidence). TI's default interrupt handlers are
named after their slot in PieVectTableInit.
