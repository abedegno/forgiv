# Fingerprint checkpoint

I searched the shipped 3020 image, not the 3022 addresses from the earlier analysis.
The input is 118,794 bytes, SHA-256
`d8d06d7a6e758abeec7724e2fdd6cca3ef4e956fc1eecac2e66e7150a2a24456`.
The manifest pins the archive commit, package, member and derived HEX.

## Corrected load address

The loaded image starts at **0x08014000**, after the four-byte update header. Its first
words are SP 0x20009BB8 and reset 0x08027805 (Thumb bit set). At reset 0x08027804,
file offset 0x13808, the instructions are `ldr r0; blx r0; ldr r0; bx r0`, followed by
the startup handlers. Loading at 0x08010000 instead puts that vector inside application
code, with no prologue and a return through registers reset has not initialised.

The standard library independently confirms this. `__scatterload` is at 0x08014158;
`__decompress1` is at 0x0801418C and the zero-init handler at 0x080141E8. The region
table is at 0x08030CD4-0x08030CF4. Its compressed region loads at 0x08030CF4 and
executes at 0x20000000 (0x69C bytes); the zero region starts at 0x2000069C and has
0x951C bytes. Its end is the vector's SP, 0x20009BB8. At the earlier assumed base,
the table's load addresses are outside the image and the handlers point into arithmetic.

The file also has a trailer: FF fill after the compressed stream, then `BMS2.0` at
file offset 0x1D000, then four bytes. I have not yet established how those last four
bytes are made. They must be covered by the final packaging gate.

## Compiler and options

The reproducible working compiler is **ARM Compiler 5.06 update 3 (build 300)**,
MDK521. I also proved all thirteen complete StdPeriph units with MDK511 (5.04 update 1,
build 49). The two links compare every code byte and RCC's twenty initial RAM bytes,
with every relocation resolved by armlink. This establishes a working build, not a
unique historical release: every installed MDK 5.11-5.36 home produces the same thirteen
relocation-masked whole units. MDK 4.60 produces five and 4.70/4.71A/4.74 produce six.
I will repeat the compiler scan on the completed application before accepting the gate.
The exact executable hashes and version strings are in `compiler-builds.tsv`.

For these driver files the flags are `--cpu Cortex-M3 -O1`, no `-Otime`, no
`--split_sections`, `STM32F10X_CL`, `USE_STDPERIPH_DRIVER`, `HSE_VALUE=8000000`.
The CL RCC file is 928 bytes and SPI is 768 bytes; HD and the CL default 25 MHz clock
fail those whole-file searches. The full thirteen-file link checks prove the 8 MHz
configuration. Shared pools and two-aligned consecutive functions prove whole-file layout.

I scored every installed MDK home at -O0/-O1/-O2/-O3 with and without -Otime against
StdPeriph 3.5.0 and 3.1.2. For MDK 5.11 onward the original split-function search finds
153 exact masked 3.5.0 functions at -O1, versus 14 at -O0; 3.1.2 gives 139 at -O1.
Those scores are candidate counts, include aliases, and are not coverage. The complete
3.5.0 driver links are the stronger version evidence. The full score matrix is retained.
The old search's four-byte anchor can miss a function whose first call is relocated;
the later whole-file and archive searches use invariant runs instead.

The image is not uniformly -O1. The original A316 -O0 USB interrupt source gives long
consecutive candidates, while most StdPeriph code matches at -O1. I also searched every
A316 source compiled at -O1; that is a separate inventory, not a replacement for the
required search under A316's own options. Options must be established per file.

## Libraries

The standard C library is present, not MicroLIB: its startup, scatter-load loop and region
handlers have the standard library's code. The archive scan searches seven available
archives in every MDK home, keeps member and section identity, and masks only listed
relocations. It also checks the opcode at each masked branch; a linker-made NOP.W is
allowed for an eliminated call or branch. Pool constants without relocations stay checked.

The real `exit.o` is the 18-byte push/pop form at 0x0802F318. It has an eliminated call
(NOP.W) and branches to `__rt_exit` at 0x0801421E. MDK 5.16-5.36 provide that form;
MDK 4.60-5.15 provide the shorter twelve-byte form and fail it. The working library home
is MDK516, separately from the compile home. `c_w.l` and `fz_ws.l` account for the
contiguous runtime clusters, including the double and single precision helpers.

Short application hits for other members are kept as candidates only. For example,
several unrelated getters have the same masked bytes as `__user_libspace`, and a twelve-byte
call wrapper inside application code resembles several stdio members. I have not named
application functions from those coincidences. The final library link must prove member
selection and resolved targets.

## A316 search and accounting

I compiled all 60 proved A316 sources and searched all 762 emitted Code symbols, including
misses, ambiguous hits, and tiny functions. The source hashes and complete inventories are
retained. With the original options, 77 symbols have candidates (37 unique, 40 ambiguous).
With -O1 on the C files, 462 symbols have candidates. These inventories overlap each other
and StdPeriph; their byte counts must never be added together.

**Proved now: 10,002 flash bytes from thirteen complete ST driver units, plus 20 initial
RAM bytes from RCC.** The flash proof covers 8.42% of the 118,790-byte loaded image.
The linked driver functions, constants and pools are checked together, not individually
masked. No whole-image gate exists at this checkpoint, so I make no claim of IDENTICAL
or zero blobs for the image. The remaining bytes need mapping, source matching and the
library/data/packaging links.

## Reproducing and retained evidence

Run the scripts with Exhume's Python, with dependencies configured as described in
`toolchain.example.toml`. The scripts write objects, listings, logs and local paths only
under ignored `build/`. `tools/evidence.py` exports portable metadata into
`evidence/fingerprint/`. No firmware bytes, objects, archives, tools or licences are exported.

The initial `fp_search.py` and `lib_search.py` searches were adapted from A316's historical
matching tools. The stricter archive search records relocations and member identity;
`st_prove.py` uses the original linker. Its small synthetic RW prefix only lets an odd-address
RCC data section be linked at its measured address, outside the compared code. The prefix
is not a firmware source or a proved byte.

The handover's v1.0.0 tag paths for the historical scripts were absent in this checkout;
I used the available A316 matching scripts and the current Exhume profile instead.

## Application compiler scan

The later application scan narrows the working compiler range. I compiled 21 complete
application draft units, CMSIS and pristine FatFs R0.12c under every installed home.
MDK521, MDK522, MDK524, MDK525 and MDK536 reproduce all 23 units (7,832 unpadded flash
bytes). MDK511 through MDK520 reproduce the small units and CMSIS, but fail the complete
FatFs unit. The older 4.x homes fail additional application units. The per-unit results
and source hashes are in `evidence/fingerprint/application-homes.tsv`.

These are resolved object-placement comparisons, not masked instruction scores. I added
narrow Thumb branch relocations to the vendored placer for this scan. Independent armlink
checks prove the selected build's source units. The scan supports MDK521 as the earliest
installed matching build; it does not distinguish the later byte-identical builds and
therefore does not uniquely identify the historical installation.

## Larger candidate cross-check

I also compiled the same three incomplete application drafts with all sixteen
installed MDK homes: USB user (2,900 bytes), settings core (2,724 bytes), and SPI
(1,068 bytes). Each used Cortex-M3, -O1 and --loose_implicit_cast, without split
sections. I resolved every relocation and compared the complete emitted text
against the fixed image interval; these are unmasked byte comparisons.
`near-complete-homes.tsv` records source and linked-text hashes, extents, differences
and relocation counts for all 48 builds.

MDK521, MDK522, MDK524, MDK525 and MDK536 produce identical linked text for each
of these three drafts. Their remaining differences are respectively 4, 16 and
242 bytes, with 155, 218 and 54 relocations resolved. Changing between those
homes does not resolve these mismatches. Older homes also reproduce the settings
and SPI drafts in some cases, but differ substantially on USB.

These drafts remain incomplete and do not supply source ownership or an
independent exclusion of a historical compiler. They extend the observed
compiler equivalence class to larger application examples. I still cannot name
one historical build uniquely from the compiled bytes.

## Near-complete source option checks

I compiled four preserved unproved application drafts under nine option
combinations, for 36 additional complete-object checks. The results and linked
text hashes are in `evidence/fingerprint/near-complete-options.tsv`. Every
relocation resolved against the mapped image symbols. These remain draft
comparisons rather than new ownership proofs.

The `-O1` controls reproduce the existing complete-span differences: USB four
bytes, settings sixteen, SPI 242 and interrupts 44. `-O2`, `-O3` and all tested
`-Otime` combinations change their sizes and many bytes. Adding `-g` or `--c99`
to `-O1` leaves each linked text byte-identical to its control. Disallowing
unaligned accesses changes the USB packed-record accesses and leaves the other
three texts unchanged. No tested combination eliminates a remaining mismatch.
I keep the established options; this check cannot independently identify the
original options from unproved source or distinguish options with equal output.
