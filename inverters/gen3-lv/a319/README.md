# Gen3 LV A319 reconstruction

A319 is reconstructed as readable application C, the original startup/context-switch
assembly forms, and linked toolchain runtime members. Both rebuild paths reproduce
the published ARMStore file exactly, including its four-byte version header.
There are **zero original-flash-byte blobs**. Unknown register meanings still have
explicit offset names; byte-exact reconstruction does not establish their semantics.

The [source and build guide](../docs/319-readability.md) explains the subsystem layout,
descriptive names, shared headers, required compiler boundaries and remaining semantic unknowns.
Start at [application_startup.c](src/app/application_startup.c), then the communication,
control and driver directories. Naming provenance remains in `map/names.tsv` and
`map/filenames.tsv`.

The target is `firmware/gen3-lv/A319_ARMStore.bin`: version 319, magic `0x5566`,
147,540 file bytes, comprising 147,536 payload bytes at `0x08014000`. The initial
stack pointer is `0x2000A7D8` and Reset_Handler is `0x0802DF28`. Checksums and
per-source evidence are in [docs/rebuild.json](docs/rebuild.json).

The object-placement gate proves 57 complete source units (55 C, two assembly),
applying every relocation and comparing every byte without masks. Its accounting is:

| Supplier | Flash bytes |
| --- | ---: |
| Proved source sections | 146,030 |
| Linked C runtime | 1,248 |
| Region table and compressed initial RAM, emitted by armlink | 248 |
| Linker alignment | 10 |
| Original-flash-byte blobs | **0** |

The independent licensed gate verifies all 57 units through armlink and links the
entire image in one armlink invocation. Two eight-byte-total C stand-ins are also
proved independently; that whole link uses the identical stock library members
instead, so it reports 146,022 bytes of application sections. The initial RAM and
region-table treatment follows A316's existing gate. Counting and acceptance rules
have not been relaxed.

A negative control changes the reported ARM version from 319 to 320. The normal
gate rejects the changed source and image; the original source is restored before
the final acceptance run.

## Rebuilding

Configure the existing Exhume checkout, armcc 5.06 toolchains, Wine and ST
StdPeriph 3.5 using [toolchain.example.toml](toolchain.example.toml) and the
[toolchain documentation](../../../docs/development/toolchains.md). Keep local
configuration, vendor links, compilers and licences outside Git. The examples below
run from this directory, with a Python environment containing Exhume's dependencies.

```sh
python3 tools/check_map.py
python3 tools/gate.py --place all --jobs 4
python3 tools/gate.py --fast --jobs 4
python3 tools/gate.py --whole --jobs 4
python3 tools/check_map.py --coverage build/gate/coverage.tsv
```

`--whole` requires the installed licensed MDK. A skipped licensed build is not a
whole-image proof. Acceptance requires PASS, identical BIN/HEX output, and a zero
original-byte count. `check_map.py --coverage` independently rejects gaps, overlaps,
missing coverage and original-byte blobs. The tools leave a previously running
licensed Wine server alone.

## Recovered changes from 316

- The DSP link carries the new HR1 BMS request and HR15 transition acknowledgement.
  Page 4 also supplies a separate battery-power target, using the extended meter's
  first-phase active power in meter mode 2. The calibration release threshold changes.
- BMS pack replies now undergo CRC checks. Their signed current is retained without
  the former positive-range filter, and the DSP's battery-fault indication clears
  all six pack records. Battery-register readback masks the corresponding energy values.
- Meter records grow from 46 to 144 packed bytes. The extended protocol handles
  three-phase voltages, currents, powers, power factors and energy counters, plus
  configuration requests and protocol changes. A sustained invalid frequency sets
  the meter fault and clears derived energy readings. Several extended fields remain
  named by offset because their meanings are not yet independently established.
- Modbus writes add HR104 (BMS request), HR107 (extended-meter direction mask) and
  HR192 (meter aggregation option). Readback, PV aggregation and power-flow calculations
  follow the larger records and the new aggregation option. HR21 reports 319.

`board.c` contains board setup, interrupt setup, the four empty hooks and panel
outputs in native order. They share literal pools and include halfword function
starts, so compiling them as one C unit reproduces the original placement without
an assembler composition stage.

The gate's synthetic zero-init section is four-byte aligned: this image's ZI region
starts at `0x20000594`. An eight-byte alignment inserts four unwanted bytes. The
runtime link also requests the newly used `__aeabi_ui2f` member. These are placement
and runtime-selection changes, not counting exceptions.

The final meter parser retains an explicit `& 0xFF` on its eight-bit direction mask.
Removing this otherwise redundant width mask swaps two registers under armcc 5.06.
The extended polling code also reads the live enable mask through a volatile view.
Both forms are documented in the source and covered by whole-unit proof. Historical
naming-provenance rows are retained; current native bindings are in `map/functions.tsv`
and `map/data.tsv`. The older cross-chip operation/reference pages describe 316;
use this target's source for 319-specific behavior.

The [semantic recovery notes](docs/semantic-recovery.md) record the remaining-name audit,
including battery reply evidence, likely meter quantities, ignored frame fields, and confirmed
Keil heap-hook identities. Meter-model identification and several register meanings remain
unproven; inferred names are labeled in the naming ledger and notes.
