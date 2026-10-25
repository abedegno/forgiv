# Relocation survey and source reuse probes

## Recommendation

Target 319 directly, retaining 318 as a comparison image. Do not make a full
318 reconstruction a prerequisite. This is a porting recommendation from a
candidate inventory, not a completed 318/319 decompilation.

318 is closer to the existing ARM source. However, most identified DSP shapes
are shared by both newer versions, and much of 319's additional ARM byte churn
has the pattern of RAM layout movement. Building a complete intermediate port
would repeat substantial placement and validation work. Use 318 to distinguish
the shared protocol/BMS changes from the later meter, configuration and import
current changes. Start with the unchanged infrastructure and the ARM/DSP link
pages, then reconstruct the remaining control changes with their callers.

## What these counts mean

Ghidra supplies ARM instruction bodies; native TI dis2000 supplies C28x
instructions. Signatures preserve instructions, registers, ordinary constants
and local branch targets/displacements while abstracting selected address
operands. Every abstracted operand is retained in the ignored analysis output.
The committed candidate inventories contain names, addresses and statuses.

These are **instruction-shape candidates**, not accepted source matches or
coverage percentages. Repeated leaves/default interrupt handlers remain
ambiguous. Unmatched shapes can reflect code changes, compiler scheduling,
function-boundary errors, pools or analysis limitations. They are not a count
of changed source functions. Ghidra can also produce spurious entries; newer
function totals are provisional.

The ARM scanner does not validate jump-table contents, ADR targets, RAM object
identity, flash data contents or the meaning of normalized pointers. C28x DP
tracking is linear, not a control-flow data-flow proof; its address/pointer
classification is heuristic. A consistent call check only verifies a target
against another unique shape candidate. An unresolved target is not silently
accepted. The unmasked compiled-unit comparisons below are stronger evidence.

| Comparison | Unique shape | Ambiguous shape | No shape hit | Inventory rows |
| --- | ---: | ---: | ---: | ---: |
| ARM 316 -> 318 | 695 | 65 | 22 | 782 |
| ARM 316 -> 319 | 681 | 65 | 36 | 782 |
| ARM 318 -> 319 | 693 | 63 | 44 | 800 |
| DSP 316 -> 318 | 352 | 113 | 10 | 475 |
| DSP 316 -> 319 | 351 | 113 | 11 | 475 |

The 318-to-319 ARM names are inherited only where 318 has a unique 316 shape;
otherwise the candidate keeps an address name. See [the named lists](function-lists.md)
and the candidate JSON files for all ambiguous and unmatched rows.

## Address movement versus changed behavior

The ARM branch-reference cross-checks find 1,253 consistent targets for
316-to-318, 1,140 for 316-to-319 and 1,180 for 318-to-319, with zero conflicting
targets among those resolved. Respectively 92, 93 and 107 remain unresolved.
Two 316-to-318 shape candidates change RAM operands: `dsp_link_settle_tick`
and `task_dsp_rx`, by two bytes. In 316-to-319, 116 shape candidates change
RAM operands; their observed deltas are +12, +580, +576 and +2 bytes. The first
three recur between 318 and 319. This strongly suggests layout movement for
many references, but does not prove the new objects have the same identity.

DSP call/run-address checks find 593 consistent references in 318 and 589 in
319, zero conflicts, and 26/27 unresolved. The RAM copy source moves from
3F1BB7 to 3F1C2C and 3F1C54; its destination remains 8000. There are 37/36
unique-shape functions with different RAM operands versus 316. Those are
listed separately; they require binding/structure review before reuse.

351 DSP functions have the same 316-derived shape in both newer versions,
with identical recorded RAM operands between 318 and 319. For windows bounded
by independently matched neighboring functions, the additional 318-to-319
shape changes occur around `grid_import_current_limit`, `arm_link_frame`,
`arm_page4`, and the `c_int00`/`os_init` startup window. The startup difference
can include linker-generated initialization, so it is not attributed to a C
logic edit. See [the bounded windows](DSP-changed-windows.json); this comparison
does not supply an exhaustive new-function inventory or an exact DSP logic
change count.

One concrete behavior-relevant example: the import-current limiter in 319
reads word address D8F2 where 318 reads D8B8, with a new DP-page setup and two
additional words. In the 316 source D8B8 is `arm_batt_setpoint`; the meaning of
319's D8F2 is still to be reconstructed. This is a changed data dependency,
not just a moved branch or function. Its calls also move, independently.

## Complete compiled-unit probes

The source files were compiled unchanged with their recorded per-file options.
ARM used armcc 5 / Cortex-M3 / -O0; DSP used TI C2000 16.9.6.LTS / FPU32 /
-Ooff. No bytes or relocation fields are masked in these comparisons.

| Unit from 316 | 316 placement | 318 placement | 319 placement | Result |
| --- | --- | --- | --- | --- |
| ARM `src/app/util.c`, six functions | 08024BA4 | 08024CAC | 08025028 | All 192 compiled .text bytes exact in each image; no relocations |
| ARM `src/app/drivers/panel_led.c`, eight functions | 08024AA8 | 08024BB0 | unresolved | All 252 compiled .text bytes exact in 316/318, with every call relocation applied |
| DSP `src/app/util/rate_limit.c` | load 3F3320 | load 3F3395 | load 3F33BD | All 46 words exact after native linking at RAM run address 9769 in each image |

For the panel unit, objplace resolves every call from the actual object, and
callee bodies are checked against complete 316 rows: unique raw candidates
where available, short GPIO leaves at the decoded targets otherwise. Repeated
references to each callee must agree. The 319 panel starts at a halfword
boundary (08024F26); the existing complete object does not match as placed.
Literal-pool alignment/composition is the first issue to investigate. It is
**not** claimed exact in 319 and no source workaround has been accepted.

[Unit evidence](unit-proofs.json) records source/object hashes, actual addresses,
sizes, comparison results and methods. The probes establish specific reusable
units, including a live DSP helper and a relocated ARM driver in 318. The ARM
utility functions are dead code in 316; their proof alone says nothing about
live inverter behavior. No complete 318 or 319 firmware source port, final
coverage, zero-blob rebuild or target gate is accepted by this work.

## Reproduce locally

Fetch the three pinned packages using the existing manifest. Configure the
existing A316 and D316 toolchain settings or their documented environment
variables. The exporter needs Exhume's armcc5 tools, Ghidra and its Java runtime,
Capstone in the Python environment, and native TI assembler/linker/disassembler.
The probes additionally need the existing armcc and TI compilers under Wine.
No toolchain needs to be copied and MDK-Lite suffices.

From the repository root, using that Python environment:

```
python3 inverters/gen3-lv/tools/compare_versions.py
python3 inverters/gen3-lv/tools/version_survey/export_inventories.py
python3 inverters/gen3-lv/tools/version_survey/normalize_arm.py
python3 inverters/gen3-lv/tools/version_survey/normalize_dsp.py
python3 inverters/gen3-lv/tools/version_survey/check_arm_references.py
python3 inverters/gen3-lv/tools/version_survey/check_dsp_references.py
python3 inverters/gen3-lv/tools/version_survey/dsp_changed_windows.py
python3 inverters/gen3-lv/tools/version_survey/prove_reuse.py
python3 inverters/gen3-lv/tools/version_survey/publish_report.py
python3 tools/repocheck.py
```

All binary inputs, disassembly, Ghidra projects, compiler output and temporary
Wine prefixes stay under ignored paths. The probe stops only its owned Wine
server at exit. Source hashes are independent of debug-path-dependent object
hashes. No firmware was executed or flashed, and no source counting or target
gate rules changed.
