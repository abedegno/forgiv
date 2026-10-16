# The gate

The scripts this page names were removed after v1.0.0, when the work was finished; they are in the
`v1.0.0` tag, and their reusable forms are in [Exhume](https://github.com/abedegno/Exhume).

`tools/gate.py` proves what is proved so far and rebuilds the whole image by linking. It passes
only when every proved source matches on its own and the rebuilt image, with its 4-byte header,
is identical to `firmware/gen3-lv/A316_ARMStore.bin`. It grows by itself: every source under `src/`
(not `src/include`) is a proved source, so a file that lands anywhere under `src/` is checked on the
next run. Sources are found by walking `src/` and placed by their at: lines and the map, never by
a directory name, so a source moves or is renamed with `mv` alone (stems stay unique across
`src/`; `tools/srcfiles.py` does the same lookups for the other tools).

## Running it

```sh
cd inverters/gen3-lv/a316
export PATH="/Applications/Wine Devel.app/Contents/Resources/wine/bin:$PATH"   # the gate sets EXHUME_ARMCC_RUNNER=wine itself
PY=$EXHUME/.venv/bin/python
$PY tools/gate.py              # everything from scratch: about 30 s
$PY tools/gate.py --fast       # only what changed since the last run: under a second when nothing did
$PY tools/gate.py --verify-only
$PY tools/gate.py --fast --libhome toolchain/MDK536   # the library link from another MDK
$PY tools/gate.py --fast --place schedule_p3,misc  # prove these sources by object placement (or: all)
$PY tools/gate.py --fast --with build/gate/test/modbus_slave.c   # a test source, in place of src/'s file of that stem
```

`GATE_PLACE` does the same as `--place`. `--with` is for tests: the file it names replaces the
source with the same stem (or is added), so I can try a changed copy without touching `src/`.

The exit status is 0 for PASS and 1 for FAIL. What it prints is also in
`build/gate/report.txt`; `build/gate/coverage.tsv` lists every byte range with what supplied it.
The rebuilt files are `build/gate/A316.img` (the image), `build/gate/A316_ARMStore.bin` (with the
header) and `build/gate/A316.hex` (Intel HEX in the original's layout).

The settings are in `exhume.toml` under `[gate]`.

## What it does

### 1. Verify

Each source goes through Exhume's own match (`armmatch.match`): compiled with the options on its
`/* opts: */` line, each section placed at the address its `at:` lines and the map give, linked
with a symdefs file for everything else, every byte compared with the image (RAM data with the
image's initial RAM). A source that is not a whole match fails the gate. The work directories are
`build/gate/verify/<stem>`.

### 2. The library link

The standard C library is linked from the toolchain's own libraries, not copied:

- `__main` to the end of `__rt_exit` (0x08014150 to 0x0801422C): `__main`, `__scatterload`
  (`__scatterload_rt2`), `__decompress1`, the zero-init handler and the `.ARM.Collect$$` pieces
  of `__rt_entry`, `__rt_lib_init`, `__rt_lib_shutdown` and `__rt_exit`;
- the members after the application code (0x0803675C to 0x08036B40): strcpy, the memclr
  helpers, strcmp, heapauxi, `__user_setup_stackheap`, exit, libspace, `_sys_exit`,
  use_no_semi and the float helpers from `fz_ws.l`;
- the region table at 0x080376B0 and the compressed RW data after it, to the end of the image;
- the 2 zero bytes at 0x080376AE, between the last proved `.constdata` (FatFs's `ExCvt`) and the
  region table (below).

A stub asks for the entry points the application and the startup file reference
(`lib_requests`), so armlink loads the members in the same resolution passes as the original
link and lays them out in the same order. The RW region is the image's own initial RAM (read by
the vendored `tools/vendor/rwdata.py`, which understands the standard library's
`__scatterload_rt2` and `__decompress1`), so armlink compresses it the same way and builds the
same two-entry region table. The zero-init region is a blob up to the library's `__libspace`
(0x20008834), then the library's own `.bss`, then a blob for the startup file's heap and stack, so
`__user_libspace` holds the original's address. What the library needs from the application
comes from `lib_symdefs`: `main`, `__user_initial_stackheap` (in the startup file at 0x0802D6C4)
and `__ARM_use_no_argv` (armcc defines it with `main(void)`; without it armlink adds the argv
set-up).

#### The padding before the region table

In the original link the region table is the last thing in the RO region, right after the
application's last `.constdata`, which ends at 0x080376AE. `Region$$Table` holds words, so armlink
puts it on a 4-byte boundary and pads. The library link cannot start a region at 0x080376AE
(armlink stops with L6244E, "not aligned on a 4 byte boundary"), so the table's region starts at
the boundary below, 0x080376AC, with a byte-aligned area of 2 bytes first (`gate_lead.s`), which
stands in for the end of the `.constdata` and is left out of the image. armlink then places
`Region$$Table` at 0x080376B0 and fills 0x080376AE and 0x080376AF itself. I tried leads of 1, 2, 3
and 4 bytes: armlink put the table at 0x080376B0 every time, with a PAD of 3, 2, 1 and no bytes,
and the table and compressed RW data came out the same.

The gate does this only when the gap between the end of the last proved flash section and the
region table is 1 to 3 bytes and they are zero in the image (`table_pad`). After the link it
checks that the listing has `Region$$Table` at the table's address and a PAD row of exactly that
many bytes from the end of the proved section, and that armlink's padding bytes are zero and equal
to the image's. The piece that holds the section makes no blob for those bytes. Anything else
fails the gate.

I did not use the library members `tools/lib_search.py` listed in the middle of the application
code (`__alloca` at 0x08014DAC, abort at 0x08023226, a math helper at 0x08024D46, the heap escrow
at 0x0803043E). They are coincidences: short application functions whose bytes, with the call
targets masked, look like a library member. The real members are only in the two library ranges.

### 3. Pieces

MDK-Lite refuses a link of more than 32 KB of RO and RW data (ZI does not count: I checked with a
40 KB blob, which failed, and 60 KB of ZI, which linked). So the rest of the flash image, from
0x08014000 to the region table, is linked in pieces of about 28 KB, on a fixed grid moved down to
the nearest 4-byte boundary outside every proved section, so a newly proved file moves no cut but
its own.

In each piece:

- every section of a proved source that starts in the piece goes in a FIXED region at its address;
- the source's other sections (its data elsewhere in flash or in RAM) go FIXED at their own
  addresses, so references to them resolve to the real addresses;
- every gap that no proved source and no library range covers is a blob of the original's bytes
  (an assembler area with `INCBIN`);
- sections that meet off a 4-byte boundary share a region, first `+FIRST`, last `+LAST`
  (armlink orders a region by attribute and name, not by the scatter file); same-named sections
  between those two keep the input order, so a longer group is allowed when its middle sections
  are same-named sections of proved objects in name order (the `.constdata` sections at
  0x08036D40);
- every other symbol comes from symdefs: the map's names and placeholders, names that sources
  give with `at:` lines, and the library's symbols from the library link.

After the link I check from armlink's map listing that each proved section sits at its address,
and compare its bytes and the blobs' bytes with the image. A source that failed verify is still
linked, so its wrong bytes also show in the image.

Where 1 to 3 zero bytes lie between two proved sections and end where the second starts, on that
section's alignment, they are armlink's alignment padding: the two sections share a region (the
first `+FIRST`), armlink inserts the padding, and the bytes are compared like any other. No cut
falls between such padding and the section after it. The section after the padding must have that
alignment in its object (armcc gives a `.text` 4-byte alignment when it holds a literal pool, else
2), so padding before a 2-aligned section is not counted: it stays a blob and shows in the
coverage. The padding before the region table is made by the library link (above).

### Sources too large for armlink: object placement

MDK-Lite's armlink stops with L6050U when one link holds more than 32 KB of RO and RW data. One
source file is bigger than that: 0x08017FCC to 0x08023143 (45,432 bytes, 15 functions,
`src/app/comm/modbus_slave.c`, then `src/auto/f08017FCC.c`). armmatch links each source on its own, so it cannot prove that file even when
every byte is right. `tools/objplace.py` does armlink's job for one object instead:

- the object is the one verify compiled, with the source's own options;
- `fromelf --text -v -r -s` lists its sections with their file offsets, its symbols and its
  relocations. MDK-Lite's objects are not plain ELF (the header starts `\xf7ORC`, and fromelf
  refuses `--elf` under the Lite licence), but the section contents are stored plainly, so I read
  each allocatable section's bytes at the offset fromelf gives. The number of relocations read
  must equal the relocation section's size divided by 8;
- each section goes where `armmatch.place` puts it (the map and the at: lines), as in the armlink
  route;
- every relocation is applied with the addend already in the field (REL): `R_ARM_ABS32`
  (S + A) | T, `R_ARM_REL32`, `R_ARM_THM_CALL` and `R_ARM_THM_JUMP24` (the BL/B.W immediate,
  range checked), `R_ARM_THM_JUMP19`, the Thumb MOVW/MOVT pair, and `R_ARM_NONE`. armcc only
  emits THM_CALL and ABS32 in this project (1,555 and 921 over all 115 sources). A symbol the
  object defines resolves to its section's address plus its value (bit 0 marks Thumb code); a
  weak definition gives way to a symdefs one, as armlink does with the startup file's handlers;
  every other symbol comes from the same symdefs the armlink route uses (the map with its
  placeholder aliases, the at: lines, and in the image stage the library link's symbols, as the
  pieces get them);
- the mapping symbols must agree: a branch relocation must sit in `$t` code, a literal word in
  `$d` data;
- an unresolved symbol, an unknown relocation type, a branch to a target that is not Thumb code
  or out of range is an error, and the source fails. Nothing is skipped or masked;
- the placed bytes are compared with the image exactly as armmatch compares armlink's bytes:
  every byte, the map rows' sizes with the same alignment padding rule, RAM data against the
  image's initial RAM.

The gate uses it on its own when armmatch's link stops with L6050U (verify then reports
`-- too large for armlink: proved by object placement`), and for any source named with `--place`.
Such a source is left out of the pieces: the pieces run around it, and its bytes in the image
come from the placement, linked again in the image stage with the pieces' symbols and compared
there too. The report says `placed SRC: ... MATCHES`, and the coverage line and `coverage.tsv`
(kind `placed`) count its bytes as "proved by object placement". Alignment padding next to a
placed section is made by the placement, as armlink would make it.

#### Checking the method against armlink

`objplace.py check SRC...` runs both routes on the same object, armmatch with armlink and the
placement, and compares the two results section by section. Over all 115 proved sources (the
101 files then in `src/auto`, from 8 to 10,212 bytes, the 12 StdPeriph files, the startup file with its
vector table and the context switch) both give the same bytes, 2,476 relocations in all, and both
call every source whole. As a whole-image check, `gate.py --place all` proves every source by
placement and leaves armlink only the blobs and the library: the image, bin and hex are
IDENTICAL.

#### Proving it can fail

I copied `src/auto/f080274A0.c` (now `src/app/control/schedule_p3.c`) to `build/gate/test/`, changed one constant (`BMS.r4day = 0x7fbc`
to `0x7fbd`) and ran `gate.py --fast --place f080274A0 --with build/gate/test/f080274A0.c`:

```
FAIL build/gate/test/f080274A0.c: not a whole match
    sub_080274A0                 0x080274A0  2 bytes differ, first at +0x154
    -- 0/10212 bytes matched, 134 relocations resolved (object placement)
placed build/gate/test/f080274A0.c: 1 sections, 10212 bytes in flash, 134 relocations resolved: DIFFERS
    f080274A0(.text) 0x080274A0: 2 bytes differ, first at 0x080275F4
image: build/gate/A316.img (0x08014000-0x080377A8): 2 bytes differ
-- gate: FAIL in 6s (--fast)
```

A changed relocation addend (`g_200002D2 + 1` for `+ 0`) failed with 1 byte different in a
literal word, and a call to a function nothing defines failed with `unresolved symbol
no_such_function`.

#### The 45 KB file now

`build/gate/test/f08017FCC.c` is automerge's merged unit of the best sources
(`automerge.py run --all --whole --only 08017FCC`), with `sub_08022C50` and `sub_08022F7A` renamed
so that their at: lines do not pull the section two ways (the build is 20 bytes longer than the
image, so those two land late). With `--with` the gate takes the oversized route on its own
(armlink stops with L6050U) and fails as it should: 4,816 bytes differ, the first at 0x0801FD35.
`objplace.py show build/automatch/files/f08017FCC.c .text=0x08017FCC` (the merged unit with its
own names; `.text=` places the section by hand, since its at: lines disagree) lists the
differences function by function, comparing each function at its map address with every
relocated field recomputed for that address:

| function | result |
|---|---|
| sub_08017FCC to sub_0801C160 (7) | same bytes |
| sub_0801FA04 | 36 bytes in 5 places: `(0xffff & (hi << 8)) + lo` with r3 and r4 swapped |
| sub_0802007C to sub_08020A32 (4) | same bytes |
| sub_08021464 | 16 bytes longer: `mov r0, r1` before byte-pair stores in other places, and literal loads where the image reuses a register (22 places) |
| sub_08022C50 | 4 bytes longer: one more literal pool word (0x200001DC, which only sub_08021464 uses) |
| sub_08022F7A | same bytes at its map address (moved 20 bytes) |

So the placement shows the differences the matching agents know of and nothing else. When the
file is whole, it goes into `src/` and the gate proves it by placement with no change (it is now
`src/app/comm/modbus_slave.c`).

### 4. The image and the file

The image is put together from the pieces and the library link. Every byte must come from a link;
where a proved source and the library both cover a byte (the three 2-byte `bx lr` functions at
0x08036868 to 0x0803686E and the one at 0x0803692C, which automatch proved as C but which are
library members) they must agree. The image is compared with the original, then the 4-byte header
is added and the file is compared with `firmware/gen3-lv/A316_ARMStore.bin`.

## The header

The four bytes before the image are two little-endian halfwords: the firmware version, then the
constant 0x5566. A316 has `3c 01 66 55`: 0x013C is 316. It is not a length or a checksum: I
checked byte, halfword and word sums, CRC-16 in four variants and CRC-32, and none gives 0x5566.
Every ARMStore.bin in the giv-firmware archive has the same layout: A187 (`c5 01`, 453, the
folder's A453), A212 (`d4 00`), A214 (`d6 00`), A316, A318 (`3e 01`), A319 (`3f 01`), A920 to
A922 (`98 03` to `9a 03`), each followed by `66 55`. The DSP files have their version big-endian
(`01 3c ff ff`). The gate writes `struct.pack('<HH', version, 0x5566)` with `version = 316` from
`[gate]`.

## Coverage now (2026-10-15)

```
coverage of the 0x237A8 (145320) bytes of the image:
  proved sources (C and assembly)         143854   99.0%
    linked by armlink                      97910   67.4%  src/app 59762, src/clib 8, src/cmsis 640, src/lib 24476, src/rtos 1444, src/startup 400, src/stdperiph 11180
    proved by object placement             45944   31.6%  src/app/comm/modbus_slave.c
  alignment padding (linker)                  10    0.0%
  C library members (linked from c_w.l, fz_ws.l)    1208    0.8%
  region table + compressed RW (armlink)     248    0.2%
  original bytes (blobs)                       0    0.0%
```

Every byte of the image now comes from a link of proved sources or of the library: no blob is
left. The 60 proved sources are 46 decompiled C files (`src/app`, `src/cmsis`, `src/rtos`, `src/lib`,
`src/clib`; map/filenames.tsv), the 12 StdPeriph files, the startup file and the context switch.

### The last 2-byte gaps

Five 2-byte gaps lay between proved files. Four are armlink's alignment padding, each 2 zero
bytes before something with 4-byte alignment:

- 0x0802DC12 (before `stm32f10x_bkp`), 0x0802FE8E (before `stm32f10x_usart`), 0x0803325A (before
  `usb_hcd_int.c`) and 0x08034E2A (before `usbh_stdreq.c`): made in the pieces;
- 0x080376AE, before the region table: made by the library link.

0x08034E2A was a blob until I found the file was split wrongly: f08034E2C.c (now usbh_stdreq.c) held only
`USBH_ParseDevDesc`, whose `.text` has no literal pool and so only 2-byte alignment. It is the
start of `usbh_stdreq.c`, which runs to 0x08035288; the whole file has a literal pool (in
`USBH_ParseCfgDesc`), so its `.text` is 4-aligned and armlink pads before it (docs/matching.md,
"Round 5"). The other gap, 0x0803043E (`bx lr`), was an empty function at the end of
f080303A4.c (now the start of `src/rtos/os_cpu_c.c`).

### Assembly sources

A `.s` under `src/` is assembled with armasm (its `; opts:` line, else `--cpu Cortex-M3`) and
proved like a C source; `; at: NAME 0xADDR` lines work as in C. The startup file's default
handlers are weak, and the application's real handlers are map rows that mostly still have
placeholder names (in address ranges other agents own), so the startup source names them on
`; at:` lines. Exhume's `armmatch.place` would let such an annotated weak name decide where the
section goes; `tools/gate.py` wraps it (`place_weak`) so that an annotated weak symbol neither
places its section nor counts as defined by the source, and the symdefs then define it at the
real handler's address. The gate imports the wrapper before any match, so verify and the pieces
both use it. An assembler `NOINIT` area (the startup file's `STACK` and `HEAP`) counts as
zero-init in a piece's MDK-Lite size check, like `.bss`.

The library row leaves out the 10 bytes the proved C files cover inside the library range. The
RW data's 0x588 bytes of initial values are the original's (they are what the compressed data is
made from); RAM rows that a proved file owns are checked by verify.

## Fast mode

`--fast` keeps, under `build/gate/cache`, each source's verify result keyed on the source, every
file it includes with `"..."` (searched next to it and in its `-I` directories), the map and the
toolchain; and each link's result keyed on its scatter file, symdefs, blobs and the objects it
links. A source whose key is unchanged is not compiled again, and a link whose inputs are
unchanged is not run again. When nothing changed no Wine starts at all. A full run without
`--fast` rebuilds everything and refreshes the cache.

## Proving it can fail

I changed one constant in a proved source: `AIRCR_VECTKEY_MASK` in StdPeriph's `misc.c` (which
`src/stdperiph/misc.c` includes), 0x05FA0000 to 0x05FB0000. `gate.py --fast` verified only that
file and relinked only its piece, and failed three ways:

```
FAIL src/stdperiph/misc.c: not a whole match
    SysTick_CLKSourceConfig      0x0802D6E8  1 bytes differ, first at +0xCA
piece 0x08029000-0x0802FE90 (10 proved sections, 6 blobs): DIFFERS
    misc(.text) 0x0802D6E8: 1 bytes differ, first at 0x0802D7B2
image: build/gate/A316.img (0x08014000-0x080377A8): 1 bytes differ
bin: build/gate/A316_ARMStore.bin (header version 316, magic 0x5566): DIFFERS from ../../../firmware/gen3-lv/A316_ARMStore.bin
-- gate: FAIL in 3s (--fast)
```

I put the constant back and the gate passed again.

## Which MDK the library comes from

The library link fixes the library's release more tightly than the fingerprint did. With
`--libhome`, MDK 5.16, 5.17, 5.20, 5.21, 5.22, 5.24, 5.25 and 5.36 all link these bytes; 5.11,
5.12, 5.14 and 5.15 do not. Their `exit` has no `push {r4,lr}` and ends with a `bl` to
`__rt_exit` (12 bytes), where the image has the 18-byte form that pushes, pops and branches. So the
library is from MDK 5.16 or later. The README's "5.11 to 5.15" came from the 16-byte abort match
at 0x08023226, which is application code. `[gate] libhome` is MDK516, the earliest that fits. The
C sources are compiled with `[toolchain] home`, now MDK521: the image's `subs rd, rn, #0` rules
out 5.11 to 5.15 and a switch dispatch in meter.c (then f0802B028.c) rules out 5.16 to 5.20 (docs/matching.md).

## Wine

The gate runs at most 4 worker processes (`[gate] jobs`, or `GATE_JOBS`), each with its own Wine
prefix under `build/gate/wine/jobN` and a persistent wineserver, and stops those servers at the
end. One thing cost me a while: the first program run under a new wineserver starts Wine's
services (services.exe, winedevice and so on), and they inherit that program's output pipes and
live as long as the server. If that first program is a Keil tool run with captured output, its
output never reaches end of file and the call never returns. Each worker therefore runs
`wine cmd /c exit` with no pipes before any tool. A stage that still hangs fails after 30 minutes
(`TIMEOUT`) instead of waiting forever. `GATE_DEBUG=N` makes the gate dump its stack and exit
after N seconds; each worker also dumps its stack into `build/gate/wine/jobN.log` on SIGUSR1.

## What is not covered yet

- No byte of the image is the original's now, but one source, modbus_slave.c, is proved by object
  placement rather than by armlink (it is over MDK-Lite's 32 KB link limit).
- Object placement reads the object's section bytes from the file at the offsets fromelf gives,
  since fromelf cannot write the Lite object as plain ELF. The cross-check against armlink over
  all 115 sources is what tells me that reading is right.
- The library link still makes the heap and stack's zero-init space with a blob of its own; the
  startup file's `STACK` and `HEAP` areas are placed and checked in verify and in the pieces, not
  in the library link (zero-init space has no bytes in the image either way).
- RW data that proved files own is checked by verify, not by the library link: the library link
  always uses the original's initial RAM. That is enough to rebuild the image, but a data-owning
  file's `.data` and `.bss` are not yet laid out by armlink in the link that makes the compressed
  RW data.
- `GATE_EXTRA_SYMDEFS` (a symdefs file added to every piece link, for a modded build) is
  there for that, but I have not tried it here.
