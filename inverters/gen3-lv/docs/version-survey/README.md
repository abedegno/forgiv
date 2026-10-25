# Gen3 LV versions 316, 318 and 319

Downloaded with approval on 2026-10-24. Both members of each package have the
expected version word. The 316 archive and its members match the existing
manifest hashes. The newer packages are pinned to their archive commit and
observed package/member SHA-256 hashes in `firmware/manifest.toml`.
`provenance.json` records the URLs, sizes, hashes and ARM vector fingerprints.
These are archive downloads, not independent vendor checksum attestations.

| Pair | ZIP bytes | ARMStore.bin bytes | DSPStore.bin bytes |
| --- | ---: | ---: | ---: |
| A316 / D316 | 128147 | 145324 | 131072 |
| A318 / D318 | 128070 | 145512 | 131072 |
| A319 / D319 | 130307 | 147540 | 131072 |

ARM headers retain magic 0x5566. The payload load address is 08014000.
Initial stack pointers are 2000A598 for 316/318 and 2000A7D8 for 319.
The reset vectors are 0802D6A9, 0802D769 and 0802DF29 respectively.
The DSP boot sector A, word addresses 3F4000..3F7FFF, is byte-identical across
all three releases.

## Initial reuse candidates

The 316 source trees document byte-exact builds for both processors. A raw
scan searches complete 316 function-row byte strings, with no relocation
masking, in the newer images. Only rows of at least 16 bytes are considered.

| Image | Unique exact byte-string candidates | Ambiguous candidates | No exact hit | Short rows skipped |
| --- | ---: | ---: | ---: | ---: |
| A318 | 488 | 2 | 142 | 150 |
| A319 | 453 | 2 | 177 | 150 |
| D318 | 192 | 93 | 165 | 25 |
| D319 | 187 | 93 | 170 | 25 |

The ARM baseline has 782 map rows; the DSP has 475. The candidate JSON files
give the old names, baseline addresses and candidate new addresses. These
are **not accepted source ports or coverage figures**. Rows may overlap; a
matching byte string does not prove a function boundary or that its calls and
RAM references still mean the same thing. Conversely, relocated calls or data
addresses can change bytes while the source logic is unchanged. Same-offset
image byte differences in `comparison.json` therefore do not measure changed
source either.

The preliminary DSP counts in commit ae2fc00 were wrong: the helper read
map sizes as hexadecimal, but DSP sizes are decimal word counts. The table,
comparison metadata and DSP candidate files are corrected here; ARM counts
are unchanged.

The [relocation survey](relocations.md) now tests instruction shapes, call/data
references and complete compiled units. Its recommendation is to target 319
directly and use 318 to isolate changes. No complete 318 or 319 port has been
accepted.

## Reproduce

From the repository root:

```
python3 tools/fetch_firmware.py --only gen3-lv-316 --only gen3-lv-318 --only gen3-lv-319
python3 inverters/gen3-lv/tools/compare_versions.py
```

Packages, extracted binaries and derived HEX files are ignored. No firmware
image, compiled object or compiler is committed. The comparison tool does not
invoke the target firmware or flash any device. The 316 sources are unchanged.
