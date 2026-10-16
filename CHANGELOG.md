# Changelog

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
