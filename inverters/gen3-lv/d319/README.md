# Gen3 LV DSP firmware D319

D319 rebuilds byte for byte from C, the six inherited assembly support files,
and TI's runtime library. Both native lnk2000 18.12.8 and Windows lnk2000
16.9.6 produce the exact 131,072-byte `D319_DSPStore.bin`. No original words
are used as blobs. Nothing has been flashed.

The [source and build guide](../docs/319-readability.md) covers the subsystem directories,
shared headers, naming evidence and compiler-sensitive views. The 319 presentation pass
retains the established DSP subsystem names and updates explicit symbol addresses in
application comments. Unidentified fields retain offsets or register numbers.

| Supplied by | Words |
| --- | ---: |
| Proved source | 48,714 |
| Runtime library | 1,240 |
| Linker initialization terminator | 2 |
| Erased-flash fill | 15,580 |
| Original-word blobs | 0 |

All 65 source units pass with real relocations resolved. The full link also
checks all 414 initialization records in order: 408 from source, six from the
runtime library. The map covers every image word once, apart from verified
0xFFFF fill.

The source and build tools started from [D316](../d316/README.md). The compiler
and library selections are unchanged: TI C2000 16.9.6.LTS, COFF ABI,
`-v28 -ml -mt --float_support=fpu32 -O1`, with the existing per-file `-Ooff`
overrides, and the 15.12.3 runtime library. Configure the documented variables
or `toolchain.local.toml`; compiler executables are used in place. Each Wine
job creates its own empty prefix and never clones a toolchain installation.

```
python3 tools/coverage.py
python3 tools/gate.py --native
python3 tools/gate.py
```

Run these from this directory. Configure `C2000_CGT`, `C2000_RTSLIB`,
`C2000_NATIVE` and `WINE_BIN` as described in the inherited example and
[toolchain documentation](../../../docs/development/toolchains.md).

The 319 changes recovered and proved include:

- BMS receive/send buffers swap addresses. Two new words track the ARM's HR1
  request and the current/previous HR15 state. A falling HR15 condition sets a
  request flag, reported to the ARM in telemetry page 6 byte 24.
- The BMS task sends HR1 when request bit 2 differs from its last sent value.
  Loss handling also clears the serial number, firmware, capacity and HR15.
- Settings page 7 carries request bit 2 in bit 11 of its word at bytes 26-27.
  The request flag clears when that bit is off.
- Settings page 4 bytes 29-30 select the battery power target: zero copies the
  header setpoint; otherwise the signed word is multiplied by ten. The import
  current limiter uses this target. Calibration thresholds also change.
- Battery force-mode selection consults HR1 request bits with HR15. The run
  status word moves from D7C1 to D7C3; D7C1 carries the new BMS request flags.

The source keeps file-specific views where the compiler's behavior proves
them. In particular, the power task observes the HR1 request through a
volatile view, while its initializer and page writer use ordinary views.
Initialization runs before task scheduling starts. The map gives current addresses;
historical naming-provenance entries remain in `map/names.tsv`. Root cross-chip reference
pages describe 316 and must be checked against this source before applying them to 319.

Linker boundary and boot-ROM API aliases live in `[symbols]` of
`toolchain.toml`. They describe addresses, not additional image data. Coverage
and source-counting rules are unchanged. The [ARM half of 319](../a319/README.md) also rebuilds exactly with zero
original-flash-byte blobs; its independent proof is recorded there.
