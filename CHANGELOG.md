# Changelog

## Unreleased

- A316 `tools/gate.py --whole` (needs a licensed MDK, found through `MDK_LICENSED_HOME` or
  `mdk_licensed_home`; skipped without one): verifies every source with armlink, the 45 KB
  `modbus_slave.c` included, and links the whole image in one armlink run, every proved section
  FIXED at its address, with the library members, the region table and compressed RW data that
  armlink makes from the proved RAM sections and the image's initial RAM. Image, bin and HEX
  IDENTICAL; fail tests in `docs/gate.md`. The MDK-Lite piecewise gate is unchanged and stays the
  default proof.
- A316 mod build `tools/modbuild.py` (needs a licensed MDK): compiles every source with the
  gate's compiler and links a working `ARMStore.bin` (header version + 0x5566, then the image)
  with an ordinary scatter file. The vector table at 0x08014000 and the RAM layout are fixed;
  code and constants float in the 256 KB flash, new variables go above the stack. Five checks
  (link, fixed addresses, objects against the gate's, moved-not-changed over every section and
  library member, numeric addresses). Built unchanged it gives the original `ARMStore.bin` and
  `A316.hex` byte for byte; `--shift` moves everything that floats and passes every check. The
  nine task and callback addresses the sources write as numbers are given their names in the
  build's copy of the sources. Free: 34,904 bytes of flash, 23,144 bytes of RAM.
- A316 `tools/emudiff.py`: runs functions of a mod build and the original side by side with
  Exhume's differential tester.
- A316 sample mod `examples/mod-example/arm-version.patch`: HR21 reports 317.
- docs/development/modding.md: the ARM section (licensed relink or MDK-Lite patch mods, the
  layout, the checks, the sample mod, the free space); building.md: the `--whole` mode.

## Planned

- v1.1: a GCC (arm-none-eabi) build of A316, so the ARM firmware can be changed and relinked
  without armcc or MDK-Lite's 32 KB limit, checked against the byte-exact build with
  differential tests.

## v1.0.0

First release: the GivEnergy Hybrid Gen3 LV inverter's two firmwares from the package
`A316-D316.zip`, both byte-exact and readable.

- **A316** (ARM, STM32F105/107): `tools/gate.py` rebuilds `ARMStore.bin` and its HEX form byte
  for byte with Keil armcc 5.06 (MDK 5.21) and MDK 5.16's C library. 99.0% of the image's
  145,320 bytes from proved C and assembly sources, the rest from Keil's library and armlink;
  0 bytes of original blobs. Names, shared headers, structs, constants, original file names and
  comments throughout; notes on the battery, the subsystems and the findings.
- **D316** (DSP, TMS320F2806x): `tools/gate.py` rebuilds `DSPStore.bin` word for word with TI
  CGT 16.9.6 and the 15.12.3 runtime library, in one link. 74.1% of the words from proved
  sources, 1.9% from TI's runtime library, the rest erased-flash fill; 0 words of original
  blobs. Every function in C except six that were assembly in the original. Names, headers,
  comments, and notes on the battery, every subsystem and the findings.
- `tools/fetch_firmware.py` and `firmware/manifest.toml`: fetch the images from the public
  giv-firmware archive and check their sha256s.
- Documentation (`docs/`): system architecture, operation of both chips, register and link
  references, findings across both chips, building and modding, with mermaid diagrams.
- D316 modding build (`tools/modbuild.py`): relinks changed sources with a floating layout and
  packages `DSPStore.bin`, with object and moved-not-changed checks and an emulator
  differential test; a sample mod in `examples/`.
- Source quality: consistent formatting (clang-format), meaningful local names, field access
  in place of pointer casts, every change proved by the gates.
- `tools/repocheck.py`: repository checks for CI (Python syntax, Markdown links, no binaries,
  the manifest).
