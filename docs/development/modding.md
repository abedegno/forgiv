# Modding: changing what the firmware does

The trees are built to prove that the sources equal the shipped firmware. A change that alters
one byte fails the gate, by design. This page is about the other kind of build: one that is meant
to differ. It says what each chip's toolchain allows, what a changed image has to keep, how the
update files are laid out, and what the boot loaders check. It is not a procedure for putting a
changed image on an inverter, and the repository has no tool that does.

## Safety first

- The Gen3 LV is grid-connected, certified equipment. Its firmware holds the grid protection
  that the certification (G98/G99 in the UK, and the equivalent elsewhere) was granted for, and
  the battery protection the battery relies on. A changed firmware is no longer the certified
  firmware, and running it on the grid may breach the connection agreement and the grid code.
- The DSP's protections are the only ones between the power stage and the grid or the battery.
  [findings.md](../findings.md) shows how few of them there are in places: with a BMS the DSP
  does not regulate the battery voltage, and battery temperature has no effect.
- Never flash an inverter you do not own, and never flash one you cannot recover. On the DSP an
  interrupted update leaves the boot loader waiting for the ARM; whether the ARM side can be
  recovered after a bad ARM image depends on its boot loader, which is not in this repository.
- Nothing in this repository has been run on hardware, changed or unchanged. Nothing here flashes
  anything. [SECURITY.md](../../SECURITY.md) and [CONTRIBUTING.md](../../CONTRIBUTING.md) set the
  rules for contributions: no flashing instructions, no claims that a change is safe to flash.

## The DSP (D316)

### What the toolchain allows

TI's C2000 code generation tools are a free download with no code-size limit, so the whole DSP
image can be compiled and linked from the sources in one `lnk2000` run. That is what the gate
does already ([building.md](building.md)).

### Why the gate cannot take a change

The gate links every section at the address the map gives it, places the runtime library members
where the image has them, and defines every symbol a source uses but does not define from the
map ([D316 gate](../../inverters/gen3-lv/d316/tools/gate.py)). A function that grows overlaps the
next one; a new function has no address. So a changed build needs the linker to place code
itself.

### The mod build

`tools/modbuild.py` is the D316 mod build. It compiles every source with the same compiler and
flags as the gate and links the whole image with TI's linker from an ordinary command file
(MEMORY and SECTIONS), so functions can change size and new ones can be added. It does not compare
the result with the original; the gate stays the proof that the sources are the original. Run
from `inverters/gen3-lv/d316`:

    python3 tools/gate.py                       # first: the proof, and the objects the mod build compares with
    python3 tools/modbuild.py                   # build src/ as it is: build/mod/DSPStore.bin
    python3 tools/modbuild.py --patch FILE      # apply FILE to a copy of src/ (build/mod/tree) and build that
    python3 tools/modbuild.py --src DIR         # build another copy of src/
    python3 tools/modbuild.py --shift 0x41      # the same, with the application moved up 0x41 words (an audit)
    python3 tools/emudiff.py [IMAGE]            # run functions of IMAGE and the original side by side

`--patch` and `--src` never touch `src/`, so the gate keeps passing while I work on a mod. A
failed check stops the build, and no image is written.

#### What is fixed and why

| Address | What | Why it is fixed |
|---|---|---|
| 0x3E8000 | the version word (`app_header.c`) | the ARM compares the file's first word with the DSP's version before an update |
| 0x3E8004 | `app_entry` | `boot_main` jumps there with an absolute `LB` |
| 0x3F3FFC | `app_valid` = 0x00AA | `boot_main` starts the application only when it reads 0xAA |
| 0x3F4000 .. 0x3F7FFF | sector A: the boot loader (`boot_main`, `boot_update`, `CsmUnlock`, `MemCopy`, the update routine loaded at 0x3F5004 and run at 0x0400), `codestart`, the CSM reserved words and passwords | the update never writes sector A, so a mod cannot change it through an update; the build checks that every word of it is the original's |
| RAM | each file's `.ebss` at its original address, the library's data, the stack at 0xE390, the peripheral registers, `PieVectTable` at 0x0D00 | many sources reach RAM through fixed addresses (`ram_view.h`, and the names the map defines) |

The ramfuncs' regions are fixed too, but not the functions in them: they load in the application
flash and run in L0 to L4 (0x8000 to 0xBFFF). `init_system` copies them with
`memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, &RamfuncsLoadSize)`, and the gate defined those
three names from the map. The mod build has the linker define them (`LOAD_START`, `LOAD_SIZE`,
`RUN_START`), so they follow the ramfuncs wherever they go. The boot loader's copy of its update
routine uses `BootRamfuncsLoadStart`, `BootRamfuncsLoadEnd` and `BootRamfuncsRunStart` the same
way. `PieVectTableInit`, the table `InitPieVectTable` copies to 0x0D00, is a constant like any
other and moves with `.econst`; the code reaches it by name.

#### What floats

- **0x3E8008 to 0x3F3FFB** (sectors D, C and B): `.text`, the ramfuncs' load image, `.cinit`,
  then `.econst` with the switch tables, in that order. Inside each group the sections keep the
  original's order, and anything new goes at the end of its group. The linker builds the end of
  `.cinit` and sets `cinit` itself (`--rom_model`), as in the gate.
- **0x10000 to 0x13FFF** (L7 and L8): the variables of a new source file. A file that is already
  in the image keeps its variables where they are, and the build stops if its `.ebss` grows or a
  variable moves within it. Adding a function or changing a header can reorder a file's
  variables (docs/notes.md, "A file's own variables"), so the build checks every variable against
  its map address. New variables therefore go in a new file.

The linker links in two passes. The first puts the four groups in one GROUP to find where each
starts; the second places them at those addresses and gives the ramfuncs their run address in
RAM, which a member of a GROUP cannot have.

#### How it is checked

Every build checks these, and a failure stops it:

1. **Link.** No unresolved symbol, and no symbol defined from the map at an address that floats
   (such a name is left to the linker). Every function of `map/functions.tsv` is in the image.
2. **Fixed words.** The version word, 0xAA at 0x3F3FFC, sector A word for word the original's,
   the ramfunc copy symbols agreeing with where the linker put the ramfuncs, every RAM variable
   at its map address.
3. **Objects.** Each section of each object against the same file's object from the last gate
   run (`build/gate/src`): identical, or listed as changed. For a changed section the build
   prints the words that changed, or the new size if it grew or shrank.
4. **Moved, not changed.** Every section the mod did not change is compared with the original
   image at the original's address. A word may differ only inside a relocated field, and that
   field must hold the original's target moved by exactly as much as the section that holds the
   target moved.
5. **Addresses written as numbers.** Every call, long branch and `MOVL XARn,#` in the original's
   code that holds an address in a region that floats must be a relocated field, or it would not
   follow a move. The same goes for every 32-bit table entry that is the address of a map row.
   All 691 are.

Built from an unchanged tree, the mod build gives the original image word for word, because it
keeps the original's order. That shows the layout is complete. It does not show the build copes
with things moving, so `--shift 0x41` moves the application flash and the ramfuncs' run
addresses up by an odd number of words. That build passes every check: 47,372 words differ from
the original, and 933 relocated fields follow their targets.

#### Running the code: emudiff.py

`tools/emudiff.py` runs 105 calls of 18 pure functions in both the original image and a mod
image, in Ghidra's p-code emulator with the
[ghidra-tms320c28x](https://github.com/outlandnish/ghidra-tms320c28x) processor module (Ghidra
12 with the extension installed, and a JDK 21). The functions are `CRC16` (which reads the CRC
tables in `.econst`), the min and max and clamp helpers in `util.c`, `median3ul` (arguments on
the stack), `isqrt_div` and `udiv32_16`, the ramfunc `rate_limit` (copied to its run address
first, as `init_system` does), and the runtime library's `FS$$DIV`, `sqrtf` and `expf` (which
reads its constants in `.econst`). Every call must give the same result in both images; 78 of the
answers are also checked against Python. The emulator has the CPU and memory only, with no
peripherals or interrupts, so it can only test functions like these.

On the shifted build and on the sample mod below, all 105 calls agree, and all 78 Python answers
agree. The two divisions by `RPT || SUBCUL` (`udiv32_16`, `isqrt_div`) give the same wrong
quotient in both images, because the module's emulator gets that instruction wrong, so they are
compared between the images only. To check that the test can fail, I changed one word of
`auchCRCLo` in a copy of the shifted image, and the CRC16 call that reads it failed.

#### A sample mod

[examples/mod-example/rcmu-limit.patch](../../inverters/gen3-lv/d316/examples/mod-example/rcmu-limit.patch)
lowers the residual current watch's continuous limit from 0.3 A to 0.25 A. That makes the watch
trip sooner, so the change is in the safe direction. It also sets the version to 317, on both
the version word and the page 1 constant the ARM shows as HR19, because the ARM only takes a file
whose version differs from the DSP's:

    python3 tools/modbuild.py --patch examples/mod-example/rcmu-limit.patch --out build/mod/example/DSPStore.bin

The build passes every check and reports what changed:

    3. objects: 494 sections identical to the gate's objects, 3 changed
       changed: app_header: version
         version+0 at 0x3E8000: 0x013D, original 0x013C
       changed: arm_link: .text:_arm_link_frame
         .text:_arm_link_frame+1047 at 0x3EB08A: 0x3D9C, original 0x3C9C
       changed: rcmu: .text:_rcmu_watch_step
         .text:_rcmu_watch_step: 61 words, 63 in the original (what follows it moves)

0.25 is 0x3E800000, whose low half is 0, so the compiler loads it with one `MOVIZ` where 0.3
took `MOVIZ` and `MOVXI`. `rcmu_watch_step` is two words shorter, and everything after it in
`.text`, the ramfuncs' load image, `.cinit` and `.econst` moves down two words. 15,047 words differ
from the original in all; check 4 accounts for every one of them outside the three changed
sections.

### Free space

D316 fills the application area almost completely. The mod build's summary gives the free space
of each region; for the unchanged tree:

| Region | Range | Used | Free (words) |
|---|---|---|---|
| application flash (sectors D, C, B) | 0x3E8008 .. 0x3F3FFB | 48,941 | 199 |
| ramfunc run RAM (L0 .. L4) | 0x8000 .. 0xBFFF | 6,072 | 10,312 |
| RAM for new files' variables (L7, L8) | 0x10000 .. 0x13FFF | 0 | 16,384 |
| sector A (boot loader) | 0x3F4000 .. 0x3F7F7F | 723 | 15,533, but the update cannot write it |

So a mod can grow the code and constants by about 200 words, ramfuncs included, since their load
image is in the same flash. Anything larger has to save space elsewhere first. The update's erase
covers sectors B to H. If the part is a 128K-word F28068 or F28069, sectors E to H lie below
0x3E8000 and are unused by D316 (inference from TI's sector map; the image does not name the
part). A `DSPStore.bin` starts at 0x3E8000, though, and the ARM reads its version from the first
word, so the mod build does not use them.

### Packaging as DSPStore.bin

A `DSPStore.bin` is the whole range 0x3E8000 to 0x3F7FFF: 65,536 words, 131,072 bytes, each word
high byte first, holes 0xFFFF. The mod build writes `build/mod/DSPStore.bin` in exactly this form
with hex2000 (`-romwidth 16 -order MS -fill 0xFFFF`, ROMS origin 0x7D0000 and length 0x20000 in
byte addresses), with the linker's map and XML beside it. For the boot loader to start it, the
word at 0x3F3FFC must be 0x00AA. For the ARM to take it, the version word must differ from the
version the DSP reports on page 3 and have the same hundreds (3xx for D316). Nothing else is
checked ([update-packages.md](../system/update-packages.md#what-is-checked-and-what-is-not)).

## The ARM (A316)

### What the toolchain allows

A316 was built with armcc 5.06 at -O0. Byte-exact rebuilding needs that compiler, which comes
with Keil MDK. MDK-Lite, which the gate uses, refuses to link more than 32 KB of code and data,
so the gate links the 145 KB image in pieces of about 28 KB and proves the 45 KB Modbus file by
placing armcc's own object ([A316 gate.md](../../inverters/gen3-lv/a316/docs/gate.md)). That
leaves two ways to change the image:

- **A full relink with a licensed MDK.** `tools/modbuild.py` compiles every source and links the
  whole image in one armlink run with an ordinary scatter file, so code can grow, shrink and
  move. It needs a licensed MDK 5.21 or later (any edition without the 32 KB limit).
- **A patch mod with MDK-Lite.** Compile only the changed functions, place them in free flash
  and re-point their callers, leaving every other byte of the original where it is (below).

The same licensed MDK also runs `tools/gate.py --whole`, which links the byte-exact image in one
armlink run and compares it ([building.md](building.md#with-a-licensed-mdk-the-whole-link)).

### The licensed MDK

The tools find it through a setting, never through anything in the repository:
`MDK_LICENSED_HOME` (the MDK home, the directory that holds `ARM/ARMCC`), or
`mdk_licensed_home` in the gitignored `toolchain.local.toml`. Under Wine the licence check reads
the registry of the prefix MDK was installed in, so its tools run in that prefix
(`MDK_LICENSED_PREFIX`, by default the directory above `drive_c` in the home's path). Unset, both
licensed modes print SKIPPED and exit 0. The licence itself is never printed, logged or copied:
the tools drop every line of tool output that names it.

### The mod build

Run from `inverters/gen3-lv/a316`, after the gate (the mod build compares its objects with the
gate's):

    $EXHUME/.venv/bin/python tools/gate.py                       # first: the proof, and the objects the mod build compares with
    $EXHUME/.venv/bin/python tools/modbuild.py                   # build src/ as it is: build/mod/ARMStore.bin
    $EXHUME/.venv/bin/python tools/modbuild.py --patch FILE      # apply FILE to a copy of src/ and build that
    $EXHUME/.venv/bin/python tools/modbuild.py --src DIR         # build another copy of src/
    $EXHUME/.venv/bin/python tools/modbuild.py --shift 0x44      # the same, with the floating code 0x44 bytes later (an audit)
    $EXHUME/.venv/bin/python tools/modbuild.py --version 317     # the version in the file's header (default 316)
    $EXHUME/.venv/bin/python tools/emudiff.py [--image AXF]      # run functions of a mod build and the original side by side

The build always works on a copy of the sources in `build/mod/tree`, so `src/` is never changed
and the gate keeps passing while I work on a mod. A failed check stops the build, and no file is
written. The compiler is the gate's own (the compile home, MDK 5.21; MDK-Lite is enough to
compile); only the link needs the licensed MDK. The two C stand-ins for library members in
`src/clib` are left out, and the members are linked from the library, as the original was.

#### What is fixed and why

| Address | What | Why it is fixed |
|---|---|---|
| 0x08014000 .. 0x0801414F | the vector table (`startup_stm32f10x_cl.s`, `RESET`) | the boot loader below 0x08014000 starts the application from it, and `nvic_init` points VTOR there (`NVIC_SetVectorTable(0x8000000, 0x14000)`) |
| 0x08013FFC | the version word | it belongs to the boot loader's area, outside the application: the build does not write it. The running ARM compares the file's header version with it and clears it before an update ([update-packages.md](../system/update-packages.md#the-arm-self-update)) |
| 0x08000000 .. 0x08013FFF | the boot loader | not in this image or this build. The application reaches it by number (HR164 in `modbus_slave.c`: `NVIC_SetVectorTable(0x8000000, 0)`, then a call through the word at 0x0000002C, the boot area's vector 11), which is right because it does not move |
| RAM | the RW and ZI layout of the original: 0x20000000 .. 0x2000A597 | the sources reach almost every variable through the map's names, which the link defines at fixed addresses; only `stm32f10x_rcc`'s `.data` and the startup file's `HEAP` and `STACK` belong to sources |

RAM is laid out as the whole-link gate lays it out: the image's own initial RAM in blobs around
`stm32f10x_rcc`'s `.data`, a zero-init blob up to the library's `__libspace`, the library's
`.bss`, then `HEAP` and `STACK`, so the region table has the original's two entries. A mod
therefore cannot change the initial value of a variable that no source owns; it changes it in
code, or adds a variable of its own.

#### What floats

- **0x08014150 to 0x0803FFFF**, the rest of the flash: the C library's start-up code, every
  `.text`, the library members, the `.constdata`, the region table and the compressed RW data,
  laid out by armlink (`*(InRoot$$Sections)` and `.ANY (+RO)`). armlink orders a region by
  attribute, then section name, then input order, and the build gives the objects in the
  original's order (by each one's `.text`), so an unchanged tree comes out at the original's
  addresses. Anything new goes in with the rest.
- **0x2000A598 to 0x2000FFFF**, above the original's stack: the variables of a mod
  (`RW_IRAM2`, `.ANY (+RW +ZI)`). They get entries of their own in the region table, so the
  start-up code copies or clears them like the original's.

The flash is 256 KB: the startup file is the connectivity line's (`startup_stm32f10x_cl.s`,
STM32F105/107), whose parts have 64, 128 or 256 KB of flash and 64 KB of RAM, and the image runs
to 0x080377A8, past the 128 KB mark (0x08020000), so the part is a 256 KB one (inference from the
image; no code reads the flash size).

#### Names for numbers

The gate's sources write nine function addresses as numbers: `main` creates its tasks with
`OSTaskCreate(0x802735b, ...)` for `task_measure` and seven more like it (OSTaskCreate takes an
int), and `USBH_USR_EnumerationDone` stores `0x8022c51` (`modbus_slave_process`) as the CDC
receive callback. Such a number compiles to the same literal word as the name but with no
relocation, so it would not follow its function when the code moves. In its copy of the sources
the build replaces each such number (outside comments and strings, with bit 0 set, a named
function's address + 1 in the floating flash) with `((int)name)` and declares the name. That gives
the same bytes with a relocation. `src/` keeps the numbers, which are what the gate proves.

#### How it is checked

Every build checks these, and a failure stops it:

1. **Link.** No unresolved symbol, and no flash address given as a number: a placeholder alias
   such as `sub_08030450` is resolved to the function's name with a steering file, and only RAM
   and peripheral names come from the map as symdefs. Every named function of
   `map/functions.tsv` is in the image.
2. **Fixed.** The vector table at 0x08014000 with the original's initial SP; every RAM blob,
   `stm32f10x_rcc`'s `.data`, `__libspace`, `HEAP` and `STACK` at its original address; the region
   table's RW and ZI entries as the original's; the image inside the application flash.
3. **Objects.** Each section of each object against the same source's object from the last gate
   run (`build/gate/verify`): identical, different only in words that became relocated fields
   (names for numbers), or changed. For a changed section the build prints the halfwords that
   changed, or the new size if it grew or shrank.
4. **Moved, not changed.** Every section the mod did not change, the library members included
   (their relocations read from the library), is compared with the original image at the
   original's address. Every relocated field must hold the original's target moved by exactly as
   much as the section that holds the target, or, for a named target inside a changed section,
   by as much as the name moved; every other byte must be the same. The region table's
   addresses must follow too, the compressed RW data must be the same bytes, and the padding
   between sections must be zero.
5. **Numbers.** In every section of the build's objects and of the library members, every
   4-byte word of the literal pools and constant data that holds an address in the floating
   flash, and every MOVW/MOVT pair that makes one, must be a relocated field. The gate's objects
   are audited the same way and their numbers listed: the nine above.

Built from the unchanged tree, the mod build gives the original `ARMStore.bin` byte for byte,
and the HEX equals `A316.hex`: the floating layout reproduces the original's placement exactly.
That shows the layout is complete. It does not show the build copes with things moving, so
`--shift 0x44` starts the floating code 0x44 bytes later (68 bytes, not a multiple of 8). That
build passes every check: 133,137 bytes of the image differ from the original; within the 126
sections the build shares with it (145,300 bytes), 159 bytes differ, every one inside a relocated
field, and 1,801 of the 3,080 relocated fields hold a moved target that moved by exactly as much
as its section. The 68 bytes before the moved code are zero. In the original's sources check 5
finds the nine numbers above and nothing else; in the build, 94 relocated words hold floating
addresses and no word holds one as a number.

#### Running the code: emudiff.py

`tools/emudiff.py` runs functions of a mod build and of the original side by side with Exhume's
differential tester ([firmware-testing.md](https://github.com/abedegno/Exhume/blob/master/docs/firmware-testing.md),
Unicorn, Cortex-M). Functions are matched by name, so the mod build may be laid out anywhere; its
ELF gives its addresses, and the RAM names come from `map/data.tsv` (check 2 shows they hold).
The default list is the pure functions: `crc16_modbus` (which reads its tables in `.constdata`),
`calc_checksum_xor_index`, the integer helpers of `util.c`, `isqrt_div`, `counter_wrap_inc`,
`fault_set` and `fault_clear`, 200 random inputs each. On the unchanged build, the shifted build
and the sample mod, all 11 behave the same. With `--mutate crc16_modbus=0x10:0x00` (one byte of
the shifted build changed) `crc16_modbus` differs (`r0 0xb442 != 0x0` on its sixth input), so the
test can fail.

`--all` runs every function the C sources define. On the shifted build, 444 of 446 behave the
same with 20 inputs each. The other two are artefacts of the random state: `os_flags_accept`
reads through `OSTCBCur`, which the tester set to 0x080304FF, into code that moved; and
`grid_load_country_limits` ran with `grid_country` 52 and `grid_table_set` 95 (the firmware
keeps `grid_country` below 29 and `grid_table_set` at 0), so it read outside its tables, in
bytes that moved. The tester has
no peripherals or interrupts, so it can only show that functions like these compute the same
thing.

#### A sample mod

[examples/mod-example/arm-version.patch](../../inverters/gen3-lv/a316/examples/mod-example/arm-version.patch)
changes HR21, the ARM version the inverter reports on Modbus, from 316 to 317 (`modbus_hr_update`
in `meter.c` fills it with a constant). The running ARM only takes an `ARMStore.bin` whose header
version differs from its own, so the build is given the same version for the header:

    $EXHUME/.venv/bin/python tools/modbuild.py --patch examples/mod-example/arm-version.patch --version 317 --out build/mod/example/ARMStore.bin

The build passes every check and reports what changed:

    3. objects: 63 sections identical to the gate's objects, 2 different only in words that became relocated fields (names for numbers), 1 changed
       changed: meter: .text
         .text+0x1F72 at 0x0802CF46: f240, original f44f (halfwords)
         .text+0x1F74 at 0x0802CF48: 103d, original 709e (halfwords)
    wrote build/mod/example/ARMStore.bin: header version 317 + 0x5566, then 0x08014000-0x080377A8 (145320 bytes); 4 bytes of the image differ from the original

0x13C is a Thumb-2 modified immediate and 0x13D is not, so armcc loads 317 with a 4-byte `MOVW`
where 316 took a 4-byte `MOV.W`. Nothing moves, and the 4 bytes of that instruction are the only
difference in the image. The header is `3d 01 66 55`. `emudiff.py modbus_hr_update` on this build
shows the change: `hr_regs+0x2a: 0x3c != 0x3d`.

A change that grows code works the same way: a test with three more lines in
`modbus_hr_update` made `meter.c`'s `.text` 28 bytes longer, everything after it moved, and every
check passed, with the call and task pointers into the moved code following their targets.

### Free space

From the mod build's summary, for the unchanged tree:

| Region | Range | Used | Free (bytes) |
|---|---|---|---|
| vector table (fixed) | 0x08014000 .. 0x0801414F | 336 | 0 |
| application flash (floats) | 0x08014150 .. 0x0803FFFF | 144,984 | 34,904 |
| RAM, the original's RW and ZI (fixed) | 0x20000000 .. 0x2000A597 | 42,392 | 0 |
| RAM for new variables | 0x2000A598 .. 0x2000FFFF | 0 | 23,144 |

So a mod can add about 34 KB of code and constants and about 22 KB of variables. Whether the ARM
boot loader copies an image that long, and how it erases, is not known: it is not in this
repository.

### Patch mods under MDK-Lite

What MDK-Lite allows is a patch: compile the changed functions, place them in free flash, and
re-point their callers, leaving every other byte of the original in place. Exhume's
[modding-build](https://github.com/abedegno/Exhume/tree/master/skills/modding-build) skill covers
this for armcc5 firmware: new code in free flash, and a proof that the original's bytes changed
only where the mod says. The free flash is the same 0x080377A8 to 0x0803FFFF.

### A GCC build

A build of the A316 sources with GCC, free of armcc and of MDK-Lite's limit, is planned for v1.1.
It would not be byte-exact; its use is a changed image that is built whole, checked against the
byte-exact tree function by function.

### Packaging as ARMStore.bin

An `ARMStore.bin` is a 4-byte header and the image as it sits in flash from 0x08014000:

| Bytes | Content |
|---|---|
| 0-1 | the version, little-endian |
| 2-3 | 0x5566, little-endian (`66 55`) |
| 4 .. | the image |

The mod build writes `build/mod/ARMStore.bin` in exactly this form (`--version` sets bytes
0-1), with `ARMStore.hex` (the image as Intel HEX, in the layout of `A316.hex`), the linked ELF
`ARMStore.axf` and armlink's listing `ARMStore.map` beside it. The running ARM takes the file when
bytes 2-3 are 0x5566 and the version is below 1000, differs from the word at 0x08013FFC and has
the same hundreds; it then clears that word and leaves the rest to its boot loader
([update-packages.md](../system/update-packages.md#the-arm-self-update)). What that boot loader
checks (a length, a checksum) is not in this repository. A version above 1000 is not an ARM
image: it is relayed to the battery.

## What the boot loaders check, in short

| Image | Checked | Not checked |
|---|---|---|
| `DSPStore.bin` | the version word (ARM), each frame's 16-bit sum and each block below sector A (DSP loader), 0xAA at 0x3F3FFC (at reset) | any CRC or signature |
| `ARMStore.bin` | the header (ARM application) | everything after the header in A316; the ARM boot loader is not in this repository |

## Where to start

- [operation/](../README.md#map-of-the-pages): what the code does now, with the function for each
  rule.
- [reference/arm-dsp-link.md](../reference/arm-dsp-link.md): most behaviour changes touch the
  settings the two chips exchange.
- [conventions.md](conventions.md): the `match:` notes mark code written in a shape the compiler
  needed; a mod build is free to write it plainly.
