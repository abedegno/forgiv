# Firmware 319 handover

Both processors now have byte-exact reconstructions with zero original-flash-byte
or original-word blobs. This is a completed 319 byte-matching milestone.

| Target | Complete source units | Independent acceptance |
| --- | ---: | --- |
| [A319](../a319/README.md) | 57: 55 C, two assembly | Unmasked object placement; normal/fast gate; licensed armlink verification of every unit and one whole-image link |
| [D319](../d319/README.md) | 65: 59 C, six assembly | Native TI and Windows TI linker builds; complete word coverage |

Both gates reject a version-319-to-320 negative control. Exact source is restored
before the final positive runs. Per-source checksums, original/rebuilt image hashes
and accounting are committed in each target's `docs/rebuild.json`.

The main recovered changes are the new BMS HR1 request/HR15 acknowledgement path,
CRC validation of pack replies, the separate DSP battery-power target, and the ARM's
larger three-phase meter records, extended meter protocol, additional Modbus settings
and revised aggregation logic. The target READMEs describe these changes and the
rebuild commands. A319's `docs/compiler-context.md` records the placement/type audits
and the last allocation-sensitive width mask so future work does not repeat them.

Version 318 remains pinned and surveyed, without a source reconstruction. Some
extended-meter register meanings remain unidentified and have honest offset names.
Neither point leaves unmatched bytes in 319. Further source edits must retain the
whole-unit and full-image proofs; individual-function scores are insufficient.

The [naming and presentation pass](319-readability.md) is complete. Startup, scheduler,
delay and CRC units now have descriptive filenames; the ARM headers no longer carry the
316 version or decompiler name. Scheduler, RTC, USB and power-flow globals have evidence-based
names, and compiler-sensitive private views have descriptive types and accessors. Comments
describe the current unit boundaries and correct the DSP's changed symbol addresses and
swapped BMS buffers. Remaining offset names explicitly mark unknown semantics.

The repository README and documentation index now distinguish the complete 319 trees from
the 316 cross-chip reference and the 318 survey. Both target proof manifests record the final
source hashes. No firmware, toolchain or license files are committed, and nothing was flashed.

The [semantic investigation](../a319/docs/semantic-recovery.md) traces the remaining names
through producers, receivers and RAM aliases. It identifies battery current/cell extrema,
eight likely SDM-compatible meter quantities, seven ARM global roles, six DSP members and
the two Keil heap hooks. Fifteen ARM address names remain, mostly clear-only state.
Protocol-map inferences and a possible apparent/reactive-power discrepancy are documented
separately from confirmed code behavior; firmware routing and acceptance rules are unchanged.
