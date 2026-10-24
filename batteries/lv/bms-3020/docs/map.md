# Map checkpoint

I classified all 118,790 loaded bytes. `tools/check_map.py` passes with no unknown bytes.
This is a structural coverage proof, not a source-matching gate.

The map has 922 primary entries, 725 symbol aliases, 206 data rows and 65 C/assembly
units. Of the entries, 325 are in the thirteen proved StdPeriph files. Library aliases
can name an entry inside an existing library body; I keep those in `aliases.tsv` rather
than making overlapping rows in `functions.tsv`.

## What each table records

- `functions.tsv`: A316's address/size/name/kind/file format. A row extends to the next
  entry, so intervening pools and alignment bytes are within its physical span. Its file
  is the proposed source unit. Library rows have kind `lib` and no C owner.
- `data.tsv`: the RO tables and the entire initial-RAM and zero-init ranges. The RCC
  arrays, library space, alignment before the heap, heap and stack have exact bounds.
- `aliases.tsv`: vector names and the matched library's symbol aliases. Default startup
  stubs whose weak name is overridden get a `_default` map name, avoiding two addresses
  for the same global symbol.
- `coverage.tsv`: a flat partition of the image, independent of the function-row spans.
  There is no overlap, gap or `UNKNOWN` row.
- `literals.tsv` and `literal-refs.tsv`: each pool/string/switch span and the actual reached
  instruction referring to it.
- `switches.tsv`: dispatch addresses, widths, bounds and table spans.
- `shared-tails.tsv`: code reached from more than one function. Application overlaps are
  verified return sequences or branches to them; library overlaps are inside matched
  archive sections.
- `anchors.tsv`: the compiled sections independently located in the image.
- `files.tsv` and `file-constraints.tsv`: source-unit ranges and the references that forbid
  a boundary inside them. There are 1,574 crossing constraints and no contradiction with
  a complete compiled section.

## Coverage

| Kind | Bytes |
|---|---:|
| Vectors | 336 |
| Instructions | 109,564 |
| Literal pools | 5,692 |
| Strings inside code sections | 64 |
| Switch tables | 188 |
| RO tables | 2,010 |
| Alignment | 119 |
| Region table | 32 |
| Compressed initial RAM | 663 |
| Package FF fill | 112 |
| Package signature | 6 |
| Package CRC | 4 |
| Total | 118,790 |

The compressed region ends at 0x08030F8B. A zero byte rounds the load region to four bytes,
then 112 FF bytes lead to `BMS2.0` at 0x08030FFC. The last four bytes are a big-endian CRC generated over the package before its
ten-byte trailer. I derived and checked its polynomial independently across six releases;
evidence/gate/package-crc.md records the proof.

## Corrections to the draft map

Ghidra initially supplied 981 candidates. I removed branch/case fragments from function
entries by following actual control flow, then retained independently referenced entries,
compiled Code symbols and remaining dead-function candidates. I did not use all odd flash-like
words as function pointers: instruction encodings can have those values. I used vectors,
initial RAM and literal loads reached by the control-flow walk instead.

The large switch at 0x0801E11E has seven entries. Its range check is at 0x0801E106, separated
from the dispatch by a branch and intervening trampolines. An eight-instruction look-back
missed it. The literal-pool sweep also produced false pool words inside real instructions;
reached loads, rather than a linear decode of arbitrary data, now establish the pools.

Many functions share a previous function's POP return. I checked this with the small C probe
in `evidence/probes/shared_returns.c`: armcc 5.06u3 at -O1 without split sections emits the
second function's final branch back to the first function's wide POP. Those shared returns
forbid a source-file boundary, just as a shared pool does. The linker is not manufacturing
those source-level relationships; the branch is already in armcc's object.

## Boundary confidence and remaining work

Complete compiled sections establish concrete footprints. Elsewhere the inferred units are the
smallest four-aligned partitions that obey reached literal references and shared returns.
They are matching units, not a claim to have recovered every original filename or every
otherwise invisible boundary between pool-less files. Source matching must confirm them,
and may join or split an inferred unit when compiled bytes require it.

The check requires complete loaded-byte coverage, unique primary names, code aliases pointing
to code, a partition of all non-library entries into units, valid PC references and full RAM
coverage. The refinement also requires zero code/data contradictions and no unverified shared
code. It does not infer the meaning of a constant table, prove a C function, or pass the firmware
gate. Those are the next stages.

The standard C library reader is the unchanged reader from A316. The generic Exhume map
generator's `table_span` only recognised MicroLIB; the wrapper supplies the standard-library
table span and seeds it with the already compiled units. All scratch objects, Ghidra projects,
raw exports and paths remain in ignored `build/`.

Whole-source proofs now refine the initial map through `recovered-sections.tsv`.
`apply_proofs.py` installs their data ownership and exported names; `file_units.py`
then rebuilds the source partitions. `ram-layout.tsv` preserves the finer initial
RAM map while compiled C fragments own the remaining initialisers. The two zero
bytes at 0x0802F342 are the linked archive's `indicate_semi.o` Code selection marker.

I later found two prologueless tail-call entries missing from the initial candidate
set. Their bytes were already classified as code.
evidence/matching/tail-entries.md records the
control-flow evidence and the limited audit I used to find the same pattern.

Narrow tail branches between known entries now also forbid source boundaries.
They join the initially separate 0x08022998 and 0x080236C8 units; the full unit
ends at 0x0802395C. This correction adds no classified bytes and leaves every
complete-source anchor consistent.

I retain reviewed compiler-context constraints in `source-constraints.tsv`, with
a link to the evidence for each row. Two protocol callers acquire the original
odd-sized stack frames only when their CRC body is compiled in the same unit;
the external-only control retains the same prototype. This joins the unproved
range `08024874..080267B8` without crediting any of its bytes as matched source.

I installed explicit complete-source anchors for the eight small autogenerated units
already included in the gate. The USB interrupt configuration and four-phase
control dispatcher now live under `src/app/`; this moves existing proofs and adds
no coverage. At that checkpoint the map had 66 source units and no compiled-anchor contradiction.

The complete chain proof joins its earlier three-entry prefix and six remaining
entries into one source unit, leaving 65 units and no boundary contradiction.
The SPI/AFE proof adds its six source-owned entries without changing its boundary.
There are now 691 source-owned primary entries; all 922 entries and all 118,790
loaded bytes remain mapped.
