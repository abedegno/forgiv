# Automatic matching of A316

The scripts this page names were removed after v1.0.0, when the work was finished; they are in the
`v1.0.0` tag, and their reusable forms are in [Exhume](https://github.com/abedegno/Exhume).

All of it is scripted: no function was edited by hand. This page says how the pipeline works,
what it found, and how it was resumed between rounds. Numbers are at the end (section "Results").

This page is a log of the matching, so it names the files as they were then: `src/auto/f<ADDR>.c`,
named after each file's first address. Those files have since been joined into their original
files where that compiles to the same bytes, renamed and moved into `src/app`, `src/cmsis`,
`src/rtos`, `src/lib` and `src/clib` (docs/readability.md, step 4). `map/filenames.tsv` gives the
new file for every old name, and the map's file column now holds the new paths.

## What the image turned out to be

- **Compiler and options.** armcc 5.05u1 (MDK514) at `-O0`, *without* `--split_sections`, for
  the whole image: the StdPeriph driver files and the application code alike. MDK 5.11 to 5.36
  (511, 514, 515, 516, 517, 520, 525, 536) compile all 39 files proved by the end of round 2 to
  the same bytes, so the compiler does not date the build. The C library does: its `exit` as
  linked is MDK 5.16 or later (docs/gate.md), and the fingerprint's "5.04 to 5.05u2" came from a
  16-byte `abort` match at 0x08023226 that turned out to be application code. MDK514 stays the
  compile home because it gives the same bytes; the gate links the library from MDK516.
- **Why not -O1 for the application code.** `automatch.py optscan --all` scored every draft under
  `-O0`, `-O1`, `-O2`, `-O3`, each with and without `-Otime`: `-O0` gives 120 exact and is best or
  tied for 367 of 495 functions; every other level gives about 40 exact. There is no address
  range where another level wins consistently (the -O1 "wins" are scattered and are large-score
  noise). Two marks of -O0 that no -O1 build makes: 324 functions copy their parameters out of
  r0-r3 into r4-r7 on entry whether they use them or not (`drafts.py param_count`; -O1 drops the
  copies), and the application code has no tail calls (-O1 turns `g(); }` into `b.w g`; the 21
  `b` to a map row in app code all go into rows that are pieces of a larger function). Shared
  exits (`b` back to one `bx lr`), `cbz` and `ubfx` are -O0 code too (checked by compiling).
- **No --split_sections.** 421 functions load literals that lie after a later function: each
  source file's functions sit in source order with one pool after the last (sometimes one in
  between). Consequences: a function can only be proved by compiling its whole file; armlink
  could not drop unused functions, so every StdPeriph driver file is in the image whole; and the
  layout is file order, not sorted by name.
- **Device.** STM32F105/107 (connectivity line): `stm32f10x_rcc.c` only fits with
  `-DSTM32F10X_CL`.

## The pipeline

All tools run from the project root with `PY=$EXHUME/.venv/bin/python` and Wine on the path:

```sh
export PATH="/Applications/Wine Devel.app/Contents/Resources/wine/bin:$PATH" EXHUME_ARMCC_RUNNER=wine WINEDEBUG=-all
```

1. `tools/libfiles.py --defs STM32F10X_CL [--name]`: compiles every StdPeriph V3.5.0 driver file
   whole (no `--split_sections`), finds its `.text` in the image, and for a file that scores 0
   writes `src/stdperiph/<file>.c` (a wrapper that includes the library source, with an `at:`
   line per function) and proves it with Exhume's match (armmatch). `--name` names and splits the
   map rows at the file's function addresses.
2. `tools/lib_names.py`: names the Keil C library members found exactly (build/lib/names.tsv).
3. `tools/drafts.py [--ghidra]`: Ghidra headless (`tools/ghidra/ExportDrafts.java`) with the map's
   names, a label per data row, the flash block read-only (so pool words fold into the globals they
   point at), RAM and I/O blocks, and the parameter counts the -O0 prologues show; signatures are
   committed and every function decompiled again. The cleaner then fixes Ghidra's forms
   (`extraout_rN` made the callee's result, `_DAT_x` and `xRamADDR` as addresses, partial
   accesses `x._2_2_`, `while (b = n != 0, n = n - 1, b)` as `while (n--)`), and turns stack
   locals passed to StdPeriph `*_Init` functions into their init structures with one store per
   field (`src/include/stdperiph_types.h`). Output: `build/drafts/<ADDR>.c` and `.json`
   (callees with prototypes, globals with address and type, the literal words the image loads).
   Ghidra takes about 12 minutes.
4. `tools/automatch.py base`: every draft becomes a unit (`build_template`): `ghidra_types.h`, one
   `extern uchar g_X[];` per symbol the image's literal pool loads and a typed macro per draft
   token at its offset from that symbol (`#define DAT_20007f89 (*(T *)(g_20007F88 + 1))`: the
   access goes through the same literal as the original, and units merge into files without type
   clashes), peripheral registers as constant addresses, a prototype per callee. All units compile
   in batches of 40 per armcc call (a Wine prefix and a persistent wineserver per job) and are
   scored against the image: 15 seconds for all 596.
5. `tools/automatch.py climb`: the search (`climb_one`). Each function's template has choice
   points; first every combination of the structural ones is compiled (at most 128), then a
   steepest descent over all points from the best three, each step compiling the whole
   neighbourhood in one armcc call, with random restarts, within a build budget. Scores compare
   structure first (registers masked), then in full. Winners are logged by idiom
   (`build/automatch/wins.tsv`, `idioms.tsv`); `learn` makes a type replacement that wins most of
   the time the default of later passes (`defaults.json`).
6. `tools/automerge.py run [--all]`: groups the map rows into source files by their literal pools
   (a file ends at the row holding its pool; a run starting at 4n+2 continues the one before),
   merges the best sources of a file's functions into one unit (typedef and macro names per
   function, prototypes merged), and runs Exhume's armmatch on it. Whole-file matches are copied
   to `src/auto/f<ADDR>.c`; each function's linked bytes are also compared on their own
   (`build/automatch/verified.tsv`).

### The score

A316's pools lie far from the code that loads them, so armvariant's score (which keeps
pc-relative offsets) can't be used. `automatch.norm` decodes a function with capstone, drops pool
words and their padding, writes pc-relative loads as `ldr rN, [pc]` and branch targets as
instruction indices; `score` counts the differing instructions of a diff with targets masked,
plus one for each aligned branch whose target doesn't align. 0 means the same instructions; only
armmatch on the whole file proves bytes.

### The idioms (choice points)

| kind | what it varies |
|---|---|
| `global:T`, `local:T`, `param:T`, `return:T` | the integer type (int, uint, short, ushort, signed char, uchar) of every global access, local, parameter and return value |
| `volatile-local` | a local declared volatile (kept on the stack: `str; ldr [sp]`) |
| `order` | the declaration order of the locals (drafted, reversed, by first use, all orders up to 4) |
| `callee` | Ghidra's prototype or an old-style declaration |
| `extra-params` | up to 3 unused parameters the draft lost (a leaf that never copies them) |
| `io` | peripheral registers volatile or not |
| `ifelse` | if/else polarity; arms that each `return` their value instead of assigning one returned at the end, with or without `else` |
| `early-return` | (transform) a whole if/else tree before `return v;` as early returns, then-arm or else-arm first |
| `inline-temp`, `inline-var` | a temporary Ghidra introduced written inline (next line only / every use) |
| `postinc` | `*p++` / `a[i++]` for a use followed by `p = p + 1` |
| `switch` | (transform) an if / else-if chain on one variable as a switch, with or without a default |
| `divmod` | (transform, per line) shifts and masks as `/` and `%` by the power of two, cast kept, dropped or u16 |
| `array-index` | (transform) `*(T *)(&b + i * sizeof(T))` as `((T *)&b)[i]` |
| `init-zero` | (transform) a dead `= 0` initialisation the draft dropped |

## Map problems found

- Four rows were literal pools, not functions (0x08024B94, 0x080254E8, 0x08029C34, 0x0802DBC8):
  merged into the function before them.
- StdPeriph rows the map generator merged (RCC, TIM, ...) were split at the library's function
  addresses by `libfiles.py --name`.
- 0x0801C6B4 to 0x0801FA04 (99 rows, 13 KB) are case bodies of one function, sub_0801C160 (its
  `tbh` table is at 0x0801C6B6); rows that start with a `tbb`/`tbh`, and the 2-byte `b` rows before
  them, are likewise pieces of the function before them. `automatch.fragments()` lists them; they
  are reported as "fragment", not scored as functions. The map should merge them.
- sub_08018E8C (13 KB) and sub_08015FF6 (4 KB) are giant functions of the same kind.

## Results (2026-10-14)

Proved by Exhume's match (armmatch, linked at the real addresses, every byte compared):

| what | files | functions | bytes |
|---|---|---|---|
| StdPeriph driver files, whole (`src/stdperiph/`) | 9 (misc, bkp, dma, exti, flash, gpio, pwr, rtc, usart) | 134 | 5,188 |
| application files, whole (`src/auto/`) | 3 | 17 | 474 |
| application functions byte-equal at their address inside a file that is not whole yet (`build/automatch/verified.tsv`) | | 15 more | 438 |

StdPeriph `adc` (score 1), `rcc` (2) and `tim` (2) differ by one or two instructions from the
V3.5.0 source under every MDK release: the vendor edited a function in each.

Scored against the image by `automatch` (596 application rows: score 0 is the same instructions,
not yet a proof):

| size | exact (0) | near (1-3) | close (4-10) | far (>10) | no draft compiles | fragment |
|---|---|---|---|---|---|---|
| <= 16 B | 56 / 356 B | 14 / 158 | 14 / 132 | 7 / 14 | 0 | 8 / 58 |
| <= 64 B | 61 / 2008 | 8 / 274 | 48 / 1806 | 39 / 1684 | 4 / 202 | 66 / 2214 |
| <= 256 B | 15 / 1428 | 7 / 638 | 16 / 1664 | 114 / 15258 | 1 / 74 | 12 / 1506 |
| <= 1 KB | 1 / 300 | 2 / 980 | 1 / 290 | 65 / 34524 | 0 | 14 / 7148 |
| > 1 KB | 0 | 0 | 0 | 19 / 38876 | 3 / 15782 | 1 / 2458 |
| all | **133 / 4092 B** | **31 / 2050** | 79 / 3892 | 244 / 90356 | 8 / 16058 | 101 / 13384 |

Most exact functions come from the first compile of the draft (120); the climbs added 13. Only
32 of the 133 are proved yet, because a function can only be proved with its whole file (shared
pool), and only 3 application files have every function exact; in the others a function after
the first non-exact one sits at a shifted address.

### Which idioms won

Changes in the winning combinations of the functions the climbs took to 0 (`idioms.tsv`,
`wins.tsv`): extra unused parameters 6, inline temporary 6, post-increment 6, local types 5,
global types 5, if/else shape (polarity or returns in the arms) 4, dead `= 0` initialisation 4,
parameter types 3, inlined variable 2, division for shift 2, return type 1, volatile local 1. No
type replacement won often enough to become a default (`defaults.json` is empty). Over all
improving moves the most frequent were: inline-var 77, local types 47, init-zero 44, global types
39, volatile-local 36, if/else 36, parameter types 33, early-return 23.

### Why the rest stays unmatched

- **Ghidra's structure, not types.** For 214 of the 249 far functions the score with registers
  masked is still over 30% of the instructions: the draft's control flow and expressions are not
  the source's. Ghidra hoists common subexpressions (`uVar1 = x >> 8` once, where -O0 code shows it
  computed at each use), merges byte stores into wider ones, folds `(u16)x / 256` into `x >> 8`,
  rebuilds switches as nested ifs, and loses pointer locals (`p = &g.sub; p->a`, seen as
  `ldr r4,=g; adds r4,#9`). The idiom transforms cover the common shapes; each far function
  needs several at once, and the expression-level ones (evaluation order, casts inside an
  expression) are not in the search.
- **Wrong parameter counts.** Ghidra keeps parameters a function only passes through (r2, r3
  untouched); the original had fewer (`push {r4,lr}` against the build's 6 registers). The
  template can add parameters, not remove them.
- **Map fragments** (101 rows, 13 KB) and the giant functions (sub_08018E8C 13 KB, sub_08015FF6
  4 KB) need the map fixed first.
- **Files.** A file is only proved whole; one far function blocks the proof of its exact
  neighbours.

## Resume

These commands ran the pipeline at the time. They need the scripts from the `v1.0.0` tag.

```sh
cd inverters/gen3-lv/a316
export PATH="/Applications/Wine Devel.app/Contents/Resources/wine/bin:$PATH" EXHUME_ARMCC_RUNNER=wine WINEDEBUG=-all
PY=$EXHUME/.venv/bin/python
$PY tools/drafts.py                    # drafts from build/ghidra/out (add --ghidra after a map change: 12 min)
$PY tools/automatch.py base --jobs 5   # every draft compiled once (15 s); keeps earlier bests
$PY tools/automatch.py climb --jobs 5 --order size --max-size 2048 --budget 8 --restarts 1 --max-builds 800 --pass climb4
$PY tools/automatch.py learn           # defaults from the wins
$PY tools/automatch.py report          # exact / near / close / far / fragment
$PY tools/automerge.py run --all --jobs 4   # prove files with armmatch; whole ones go to src/auto
$PY tools/libfiles.py --defs STM32F10X_CL   # StdPeriph files
$PY tools/automatch.py show ADDR       # a function's best source and its diff
$PY tools/automatch.py template ADDR   # its armbrute template (static choice points only)
$PY tools/automatch.py optscan --all   # the drafts under every -O level
```

Results live in `build/automatch/`: `results.tsv` (addr, name, size, best score, method,
seconds, builds), `best/<ADDR>.c`, `tpl/<ADDR>.c`, `files.tsv`, `verified.tsv`, `idioms.tsv`,
`wins.tsv`, `optscan-all.tsv`. A changed template is picked up by the next `climb`; `results.tsv`
only ever keeps a better score.

### Next steps worth scripting

1. Fix the map: merge the switch fragments into their functions, then run Ghidra again.
2. A "drop parameters" transform (and the callers' prototypes old-style).
3. Expression-level transforms: un-hoisting (each use recomputes), cast placement inside
   expressions, struct pointer locals for `base + offset` runs.
4. Prove exact functions in partial files by matching files progressively from their start.

## Round 2 (2026-10-14): map, rewrites, files

Still scripted throughout: no function was edited by hand. Numbers at the end of this section.

### Map changes (`tools/mapfix.py`)

`mapfix.py report` classifies every row. A row is a function entry when a `bl` anywhere calls it,
a pointer points at it (a 4-aligned odd word in the vector table, a literal pool or the data after
the code) or it is library code. A row that is no entry is a fragment of the row before it when
it is a switch case target (tbb, tbh, or armcc's word-offset switch: `lsls rX, rI, #2; add rX,
pc; ldr rX, [rX, #4]; add pc, rX`, which the old sweep decoded as instructions), a branch target
of another row, entered by fall-through, starts with a `tbb`/`tbh`, uses r4-r11 without a
prologue, is a lone 2-byte `b`, or is the body of a loop whose `b cond` the map split off.
Library zones (below 0x080143C8, from 0x0803675C) and rows after a StdPeriph row are left alone.
`mapfix.py merge` merges (backup in `build/functions.tsv.bak-before-fragmerge-*`).

- 125 rows merged: 912 to 787 rows. sub_0801C160 now runs to 0x0801FA04 (0x38A4 bytes, its 99
  case bodies and a word-offset switch of 0x83 cases); sub_08018816 takes the old sub_08018E8C
  (0x394A); sub_08015D02 takes sub_08015FF6; eleven more functions take their case bodies, loop
  bodies or tails (sub_08021464, 08020A32, 08025B80, 0802B028, 0802C0D6, 080337C0, 080341F8,
  0802735A, 08029548, 08030486, 08029AA6).
- 102 rows stay functions though nothing in the image references them (no `--split_sections`
  keeps dead functions; RW data, which is compressed, may hold more pointers).
- The source files are in the map's file column (`auto/fADDR.c`, `mapfix.py files`), from
  `automerge.groups()`, which now puts a file boundary only where no load crosses it (no
  function before it loads a literal or adr block after it, none after it loads one before it)
  and at a 4-byte boundary. The old rule (a file ends at the row holding pool words) took
  armcc's intermediate pools, placed mid-file and even mid-function, for file ends (sub_08014B68
  holds 45 pool words of earlier functions). 129 files (67 by the old rule, still available with
  `AUTOMERGE_GROUPS=pool`). The finer split also proves small runs of exact functions on their own.
- Fixes to the scoring found on the way: the tbh index register was parsed as `lsl`, so every
  tbh table was decoded as code; `cmp.w` bounds were ignored; `cmp rN, #k; bhs` has k entries,
  not k+1; word-offset switch tables were code; a `b` over an intermediate pool (a placement,
  not source) is now dropped from both listings, and so is a `nop` before a pool.

### Drafts

`drafts.py --ghidra` again (about 10 minutes). New: `params.tsv` has a third column; a function
that calls out and copies fewer than four parameters in its -O0 prologue gets exactly that many
(ExportDrafts truncates Ghidra's pass-through parameters as well as widening). On the 456
functions whose rows did not change, combination 0 went from 119 exact and a score sum of 19745 to
127 exact and 19356, and drafts with `in_rN`/`extraout_rN`/`unaff_` leftovers from 144 to 41.
`build_template` also turns a Ghidra 64-bit return (`undefined8`, `return CONCAT44(junk, v)`) into
`uint` and `return v`.

### Rewrites (`tools/rewrites.py`, registered in `automatch.add_rewrites`)

Each is a transform choice point (global, and per line for the first six where that makes
sense); `whole:` transforms see the whole unit. `automatch.py ablate [--kinds a,b]` measures every
kind over all 471 drafts: from combination 0 it compiles each single option of each point and
reports the functions a kind applies to, how many its first option alone makes better and worse
(blind), the sum of those changes (blind gain) and the sum of the best single-option gains
(potential: what the climb can take). Kinds with a clearly positive blind gain are on by default
(`FLIP`: option 0 and 1 swap, so combination 0 starts from them); kinds that gained nothing are
left out (`UNHELPFUL`, still available through `$AUTOMATCH_REWRITES`).

| kind | what it does | functions | better / worse | blind gain | potential | decision |
|---|---|---|---|---|---|---|
| ptrlocal | the accesses through one pool symbol made through a pointer local (`p = &g; p[off]`), declared first or last | 139 | 30 / 106 | -4519 | 2907 | choice, structural |
| inlinecall | `v = f(..);` and v used once on the next line: the call written in place | 98 | 41 / 15 | +356 | 476 | on by default |
| cmp | constants to the right of comparisons, as is or by one with the other strictness (`5 < x`: `x > 5` / `x >= 6`); also per line | 90 | 8 / 5 | (on) | 342 + 268 per line | on by default |
| early-return (round 1) | measured again | 107 | 22 / 10 | +93 | 231 | on by default |
| arrfield | `p + i*S + C` as `p + A + i*S + B` (A the array's offset, the smallest constant seen with that p and i*S) | 24 | 17 / 2 | +97 | 115 | on by default |
| inline-temp (round 1) | measured again | 54 | 40 / 3 | +82 | 74 | on by default |
| rmw | `*P = *P & K \| E` through a volatile stack local (the USB library's union locals) | 57 | 4 / 49 | -668 | 71 | choice |
| gtzero | `x != 0` as `x > 0`, `x == 0` as `x <= 0`; also per line | 149 + 71 | 4 / 32 | -88 | 14 + 65 | choice |
| dsign | the signedness of every `*(T *)` turned round | 158 | 7 / 48 | -107 | 50 | choice |
| unhoist | a temporary assigned several times, each value written into its uses | 143 | 5 / 14 | -227 | 31 | choice |
| modulo | `x == (x / k) * k` as `x % k == 0` | 2 | 2 / 0 | +31 | 22 | on by default |
| booleq | a local tested bare as `== 1` / `!= 0` / `!= 1` | 38 | 5 / 14 | -8 | 17 | choice |
| vderef | every `*(T *)(p + n)` volatile (peripheral registers through a struct pointer: no narrowed loads) | 158 | 5 / 6 | -10 | 5, 3 to exact | choice |
| nop | `__nop();` in an empty loop | 8 | 1 / 7 | -15 | 2, 1 to exact | choice |
| loadsplit | a halfword load as two byte loads | 26 | 0 / 22 | -707 | 0 alone | choice (needs a return type change with it) |
| artifacts | drop statements that only move in_rN/extraout_rN | 29 | 1 / 3 | -38 | 1 | left out |
| elseret, concat, bytemask | else after a returning if; CONCAT11 stores as two bytes; a byte's dropped mask | 30, 10, 1 | 0 | <= 0 | 0 | left out |

Round 1's choice points over the same set, for comparison (potential): volatile-local 1222,
ifelse 717, extra-params 679, inline-var 520, divmod 450, init-zero 346, order 193, array-index
98, switch 92, postinc 24, callee 19. The changes in the climbs' exact wins: init-zero 7,
inline-temp 4, inline-var 3, vderef 3, early-return 2, cmp 2, volatile-local 2, switch 2,
ifelse 2, postinc, order, booleq, gtzero, nop 1 each.

The potentials are large but the exact gains small: a far function needs several shapes right at
once, and most far ones differ in control flow Ghidra rebuilt (loops as do-while with gotos,
switch arms merged), which no text rewrite here undoes.

### File context

armcc knows the registers and stack alignment of a function defined earlier in the same file and
uses them in its callers: such a caller pushes only lr (13 functions) or keeps a value in r2/r3
across the call. A function compiled alone can't show this. `automatch.unit()` now compiles each
function after the definitions of the same-file functions it calls (`context_funcs`, their best
sources, merged as automerge merges a file), taking only callees that are already exact (an
inexact callee's registers are noise; when a callee becomes exact, `rescore` changes its callers).
`AUTOMATCH_CONTEXT=0` turns it off. Every compile path uses it (base, climb, ablate, rescore,
show, mine).

### Files (`tools/automerge.py`)

`automerge.py blockers [--max N]` writes `build/automatch/blockers.tsv` (per file: functions,
bytes, exact, the blocking functions with score and size) and with `--max N` prints the blockers
of files with at most N of them, for `climb --only`. The merge gives a function of the file an
old-style declaration (and an old-style definition) when callers from another draft generation
pass a different number of arguments, and an external callee whose forms disagree an old-style
declaration.

### Results (round 2)

Proved by Exhume's match (armmatch, linked at the real addresses, every byte compared):

| what | before (round 1) | after (round 2) |
|---|---|---|
| StdPeriph driver files, whole | 9 files, 134 functions, 5,188 B | the same |
| application files, whole (`src/auto/`) | 3 files, 17 functions, 474 B | 30 files, 44 functions, 1,046 B |
| more application functions byte-equal inside files not whole yet | 15, 438 B | 18, 318 B |
| application functions proved | 32, 912 B | 62, 1,364 B |

Scores (map rows with a draft; round 1 on the old map of 596 rows, round 2 on 471):

| | exact (0) | near (1-3) | close (4-10) | far (>10) | no draft compiles | fragment |
|---|---|---|---|---|---|---|
| round 1 | 133 / 4,092 B | 31 / 2,050 | 79 / 3,892 | 244 / 90,356 | 8 / 16,058 | 101 / 13,384 |
| round 2 | 153 / 5,400 B | 27 / 1,980 | 80 / 4,458 | 205 / 88,684 | 6 / 29,430 | 0 |

By size, round 2:

| size | exact | near | close | far | none |
|---|---|---|---|---|---|
| <= 16 B | 62 / 438 | 12 / 120 | 9 / 86 | 0 | 0 |
| <= 64 B | 66 / 2176 | 6 / 226 | 48 / 1890 | 32 / 1372 | 2 / 96 |
| <= 256 B | 23 / 2196 | 7 / 654 | 22 / 2214 | 95 / 12938 | 1 / 74 |
| <= 1 KB | 2 / 590 | 2 / 980 | 1 / 268 | 59 / 30812 | 0 |
| > 1 KB | 0 | 0 | 0 | 19 / 43562 | 2 / 29166 |

The two that don't compile at all are the merged giants sub_08018816 and sub_0801C160 (14 KB each).

Where the gains came from, in order: the file grouping by crossing loads (3 whole files to 27 at
the scores of the time), the exact parameter counts (+8 exact at combination 0), the climbs with
the new rewrites (+11 exact over six passes), file context (+1 now, sub_08023ADA; sub_080299D0
and sub_0802D412 score 0 with their callees' current sources and will count once those callees
are exact), the scoring fixes (switch tables, branches over pools). The last full climb pass (`r2climb4`) added 2
exact functions and no file; a deep climb on the blockers of files with one or two blockers
(`r2block3`: budget 30, 5 restarts, 5000 builds) added nothing: the search has stopped paying.

### Blockers

`build/automatch/blockers.tsv` lists them per file. 41 files are blocked by one function, 19 by two;
almost all blockers score above 5. The largest files and what blocks them:

| file | functions | bytes | exact | blockers (score) |
|---|---|---|---|---|
| f08017FCC | 6 | 16,788 | 2 | 08018816 (no compile, 14.6 KB), 08018386 (396), 08018022 (276), 08017FCC (2) |
| f0801C160 | 1 | 14,500 | 0 | 0801C160 (no compile) |
| f0802039C | 6 | 11,688 | 0 | 08021464 (2794), 08020A32 (758), 080206A4 (326), 0802039C (233), 08022C50 (144), 08022F7A (94) |
| f080274A0 | 16 | 10,212 | 2 | 14, among them 0802752C (270), 0802887C (309), 08028E60 (293) |
| f08015378 | 6 | 8,692 | 0 | 08015D02 (2176), 080153CC (575), 080171DA (105), 08016FDC (72) |
| f0802B028 | 2 | 6,436 | 0 | 0802B028 (1406), 0802C0D6 (823) |

Kinds of blocker, from `mine` over the far functions: control flow Ghidra rebuilt (loops with
gotos, `do { if (n <= i) goto out; ... } while (...)` where the source had `for` with a return);
struct pointers and struct arrays flattened into offsets (`p + i*0x20 + 0x3b1`, partly undone by
arrfield; armcc's `add.w` of the array offset is not always the smallest constant); register
allocation of the parameter copies (`mov r4, r0; mov r5, r1` against r5, r4: the order of first
use); stores Ghidra removed as dead (two `strh` to one register); `__nop()` calls and delay
loops; a few functions from the USB library with union-bitfield locals (rmw covers one shape).

### Resume (round 2)

These commands ran the pipeline at the time. They need the scripts from the `v1.0.0` tag.

```sh
cd inverters/gen3-lv/a316
export PATH="/Applications/Wine Devel.app/Contents/Resources/wine/bin:$PATH" EXHUME_ARMCC_RUNNER=wine WINEDEBUG=-all
PY=$EXHUME/.venv/bin/python
$PY tools/mapfix.py report [-v]              # entries, fragments and why, dead rows
$PY tools/mapfix.py merge && $PY tools/mapfix.py files   # after a map change; then drafts.py --ghidra
$PY tools/drafts.py --ghidra                 # Ghidra with exact parameter counts (10-12 min)
$PY tools/automatch.py ablate --jobs 5 [--kinds a,b]     # measure transforms over every draft
$PY tools/automatch.py climb --jobs 5 --order size --max-size 4096 --budget 10 --restarts 1 --max-builds 1000 --pass NAME
$PY tools/automatch.py rescore --jobs 5      # after new exact functions (file context) or a scoring change
$PY tools/automerge.py run --all --jobs 4    # prove files; whole ones go to src/auto
$PY tools/automerge.py blockers              # per file; --max 2 prints blocker addresses
$PY tools/automatch.py climb --jobs 5 --order size --budget 20 --restarts 3 --max-builds 3000 --pass NAME \
    --only $($PY tools/automerge.py blockers --max 2)
$PY tools/automatch.py mine --jobs 5 [--min-score 4] [--max-score N]   # recurring mismatch shapes
```

Switches: `AUTOMATCH_REWRITES` (comma list of rewrites to register), `AUTOMATCH_FLIP` (kinds on
by default), `AUTOMATCH_PREPASS` (rewrites applied to every draft first), `AUTOMATCH_CONTEXT=0`,
`AUTOMATCH_DRAFTS` (another drafts directory, to compare generations), `AUTOMERGE_GROUPS=pool`,
`DRAFTS_EXACT_PARAMS=0`. Round 1's drafts, results and map are kept in `build/round1/`.

Next steps worth scripting: loop restructuring (Ghidra's do-while with an early `goto` back into
`for` with a return); struct types recovered from the offsets a function uses through one
pointer (so `p->arr[i].f` compiles to armcc's add of the array offset); the parameter copy order;
splitting the two giant functions' switch arms into case bodies the climb can work on one by one.

## Round 3: o0c translator

I wrote `tools/o0c.py`, a deterministic translator from the image's armcc -O0 Thumb-2 code to C,
to replace Ghidra as the source of drafts for the functions round 2 left unmatched. It reads only
the image and the map, needs no Ghidra run (all drafts in about 5 seconds), and writes its drafts
in the same form as `tools/drafts.py` (`build/o0c/<ADDR>.c` and `.json`), so automatch's
templates, base pass and climbs take them as they take Ghidra's. Still scripted throughout: no
function was edited by hand.

### What armcc -O0 does (learned from probes)

`o0c.py probe FILE.c` compiles a snippet and lists its code. The rules the translator relies on,
each checked that way (the probes are small, so I list the facts, not the files):

- Parameters are copied out of r0-r3 in the prologue (`mov r4, r0`), in a leaf only when the
  register is needed (`mov r3, r0` so that r0 can hold the result). A leaf keeps the registers of
  unused parameters: its scratch registers start above them.
- Every local gets a register of its own for the whole function (never shared), r4 upwards in a
  function that calls out, any free register in a leaf. Registers go by use count, ties by
  declaration order (lower register first). Stack locals are laid out in declaration order from
  the top of the frame down; `push {r3, lr}` or `push {r0-r3, ...}` without a `sub sp` is frame
  space, with a `sub sp` it holds the parameters' homes.
- Statements do not share values, with three exceptions: a literal pool address loaded once is
  reused while it stays in a register (also across an if/else, not into a loop), as are small
  constants used as stored values and `base + constant` addresses (`addw r0, r4, #0x39f` used by
  two statements). A value reused otherwise was a local, or an assignment used as a value
  (`if ((g = g + 1) > 300)`, `x = y = f()`, `if ((r = f()) != 0)` compiled as `subs r7, r0, #0`).
- `x * 256` and `x << 8` compile to the same `add rd, ra, rb, lsl #8` but with different register
  choices; `x & ~4` gives `bic`, `x & -5` loads the constant; `(ushort)wide_global` loads with
  `ldrh` and still casts with `uxth`; signed division by powers of two, `%` by constants and
  `mls` remainders have fixed shapes the translator folds back to `/` and `%`.
- Control flow follows the source's layout: `while`/`for` is `b test; body; test: bcc body`,
  `do ... while` starts with a `nop`, `while (1)` is `b end; body; end: b body`, the epilogue is
  placed at the first `return` in layout and later returns branch back to it, `if (x) break;` is
  `b!x skip; b exit`, and a switch is a dispatch (bounds check plus `tbb`/`tbh`/word table, or a
  binary tree of compares for sparse cases) followed by the case bodies in order.
- armcc adds things the source does not show: a `nop` for a jump to the next instruction and for
  an assignment that stores the value the variable already holds (`for (i = 0; ...)` right after
  `i = 0`); branch islands (a conditional branch out of range goes to a `b` placed in reach);
  conditional branches threaded through branches on the same flags; statement tails merged
  across paths (`ldr r1, =g; strb r0, [r1]` ending two arms); string literals placed after the
  function and taken with `adr` (or `subw rX, pc, #n`).

### Design

1. **decode**: capstone over the map row. Literal pools are skipped (automatch's words plus the
   words loaded by code after a `tbb`/`tbh` table, which `drafts.literal_targets` never swept), as
   are strings and tables an `adr` or a pool word points at, and `tbb`/`tbh`/word-offset tables.
2. **lift**: each instruction becomes micro-operations on expression trees (set register,
   flags, store, call, branch, switch, return), with the -O0 idioms above folded (`mk`).
3. **CFG and webs**: blocks from the branches (islands resolved, threaded branches untangled,
   branches over intermediate pools dropped), reaching definitions per register, defs that reach
   a common use joined into webs. A web is a parameter (its entry value, or the register its
   prologue copy went to), a temporary (one def used in its own block: substituted into its use,
   so statements rebuild themselves), a constant (a literal kept in a scratch register across
   blocks), a select (r0-r3 set in both arms of a diamond and used at the join: `c ? a : b`), or a
   local named after its register (`v4`; every web of one register is the same local).
4. **eval**: each block symbolically, giving statements and a terminator; locals' and
   parameters' types come from what is assigned to them (`uxtb`, `ldrb`, small constants).
5. **shape**: structured C from the block order: condition chains reduced to `&&`/`||` (two
   adjacent conditional branches merge when they share a target or the first skips the second),
   if, if/else (validated: the else part is entered only from the condition), joins that are the
   enclosing statement's join, while, for (when the loop's `b test` follows a dropped init),
   do-while, `while (1)`, break, continue, returns, switches found by walking the dispatch with
   the set of values still possible at each block (so chains, trees, tables and tables reached
   through compares all come out as one `switch`), and `goto` where nothing fits. When a
   function will not translate as laid out, merged statement tails are copied back into each
   predecessor and selects become locals, and it is tried again.
6. **emit**: C in the drafts' vocabulary: int, uint, ushort, short, uchar, sbyte; a `DAT_`
   token per global address and access type in `meta['globals']`, written through the literal the
   image loads (`(&DAT_x)[i]` when the index scale is the element size, else byte arithmetic on
   the base); peripheral registers as `*(volatile uint *)(base + offset)`; stack locals, arrays
   and StdPeriph init structures; string literals; intrinsics (`__disable_irq`, `__dsb`,
   `__nop`, ...); callee prototypes in `meta['calls']` (old-style when call sites disagree).

`o0c.py asm ADDR` and `blocks ADDR` show what the translator sees; `show ADDR` compiles combination
0 of a draft and diffs it; `mine` is automatch's mismatch miner over the o0c drafts.

### Integration

- `automatch.py base|climb --source o0c` reads `build/o0c` instead of `build/drafts`; results keep
  the best per function whichever source it came from (methods tagged `-o0c`). With o0c drafts the
  default-on rewrites (round 2's `FLIP` kinds) stay choices only: they undo Ghidra's habits and
  cost o0c drafts 19 exact functions at combination 0 (188 against 207 when I measured it).
- `drafts.py --o0c` runs the translator (`o0c.py gen`).
- `automatch.image_pool_words()` now adds the pool words loaded by code after a switch table
  (o0c's `extra_pool`, 188 words in 11 functions). Before, those words were decoded as
  instructions in the image's listing, so no source could score 0 there. `rescore` takes it in.
- `AUTOMATCH_TIES=1` lets an equal score replace the kept source (the score ignores literal
  pools, so two sources with the same instructions can differ in what proves).
- `o0c.py score --source o0c|ghidra` compiles combination 0 of every draft and scores it into
  `build/o0c/score-<source>.tsv` without touching `results.tsv`, on Wine prefixes of its own
  (job20 and up), so it can run beside a climb. `o0c.py compare [--before results.tsv]` prints
  the tables below.

### Results (round 3)

Combination 0 of the drafts, over the 318 map rows that were not exact after round 2 (no climbs;
`o0c.py compare --before build/automatch/results-before-round3.tsv`):

| drafts | exact | near (1-3) | close (4-10) | far (>10) | no draft compiles |
|---|---|---|---|---|---|
| Ghidra | 0 / 0 B | 22 / 1,720 | 67 / 3,296 | 220 / 89,538 | 9 / 29,998 |
| o0c | 107 / 10,804 B | 38 / 6,522 | 46 / 6,236 | 97 / 96,382 | 30 / 4,608 |
| best of the two | 107 / 10,804 B | 39 / 6,588 | 59 / 6,506 | 112 / 100,560 | 1 / 94 |
| round 2 results (climbed) | 0 / 0 | 27 / 1,980 | 80 / 4,458 | 205 / 88,684 | 6 / 29,430 |
| round 3 results (climbed) | 121 / 14,116 B | 40 / 5,630 | 62 / 7,104 | 94 / 97,608 | 1 / 94 |

o0c's draft is better than Ghidra's for 238 of the 318, worse for 52, equal for 28. Over all 470
rows, combination 0 gives 248 exact from o0c drafts against 132 from Ghidra's. The far o0c bytes
are larger than Ghidra's because the two 14 KB functions now compile (sub_0801C160 and
sub_08018816: about 3,860 and 3,560 instructions differ of some 7,000 each); 31 functions have no
o0c draft (see below), and for those the Ghidra draft stays.

All rows (`results.tsv`, after the base pass with both sources and three climb passes on o0c
drafts):

| size | exact | near | close | far | none |
|---|---|---|---|---|---|
| <= 16 B | 69 / 530 | 6 / 42 | 8 / 72 | 0 | 0 |
| <= 64 B | 111 / 4,114 | 9 / 376 | 24 / 852 | 10 / 418 | 0 |
| <= 256 B | 80 / 8,448 | 19 / 2,344 | 25 / 3,900 | 24 / 3,384 | 0 |
| <= 1 KB | 12 / 6,312 | 6 / 2,868 | 6 / 2,354 | 40 / 21,116 | 0 |
| > 1 KB | 0 | 0 | 0 | 21 / 72,728 | 0 |
| all (round 3) | **272 / 19,404 B** | 40 / 5,630 | 63 / 7,178 | 95 / 97,646 | 0 |
| all (round 2) | 153 / 5,400 B | 27 / 1,980 | 80 / 4,458 | 205 / 88,684 | 3 / 170 |

Most of the gain is the translator itself: the first base pass with o0c drafts took exact
functions from 153 to 226 (32 seconds), and of the 272 exact now, 103 were recorded by base passes
with o0c drafts (`base-o0c`) and 18 by climbs on o0c templates. The climbs (`r3climb1`, `r3climb2`:
every non-exact function up to 2 KB; `r3block1`, `r3block2`: the blockers of files with one or two
of them, budget 20, 3 restarts) also took several large functions much closer (sub_0802887C 309 to 46, sub_08028E60 293 to
20, sub_0802C0D6 780 to 446). The deepest climb pass added nothing: as in round 2, what the climbs
can still change is types and small shapes, not structure.

Proved by Exhume's match (armmatch, every byte):

| what | round 2 | round 3 |
|---|---|---|
| application files, whole (`src/auto/`) | 30 files, 44 functions, 1,046 B | 51 files |
| application functions byte-equal at their address (whole files and inside partial ones) | 62, 1,364 B | 139, 6,876 B |

The largest new whole files: f08035ADC (6 functions, 440 B), f08034BF4 (4, 428 B), f080305FC
(4, 316 B), f08029CF8 (6, 244 B), f08033FF4 (2, 212 B), f08034B40 (2, 180 B), f08031CFC (160 B),
f08031134 (140 B), f08034DA0 (140 B).

### What blocks the rest

- **Files.** 272 functions score 0 but only 139 are proved: a file is proved whole, and 34 files
  are still blocked by one function, 42 more by two or more (`build/automatch/blockers.tsv`).
  The big files are blocked by the big functions (f08017FCC by sub_08018816, f0801C160 is one
  function, f0802039C by six functions of 300 B to 6 KB).
- **A score of 0 that does not prove.** sub_08014CA4 compiles to the image's instructions and
  pool values, but its pool lies about 0x150 bytes past the function, beyond the 8-byte row at
  0x08014DAC that `build/lib/names.tsv` names `__alloca`; library rows split files, so the file
  automerge builds for it ends too early. That library name is probably wrong.
- **The giants.** sub_0801C160 and sub_08018816 now translate and compile (a 131-case switch
  through a compare tree and four word tables, found whole), but differ in about half their
  instructions; any change to them costs seconds per compile, so the climbs skip them.
- **Shapes I could not recover from the code alone.** Hand-written gotos into a switch's case
  bodies and a `return` placed among the cases (sub_08022C50, sub_0802C94C), else parts laid out
  after an unrelated join (sub_0803530A), nops for dropped redundant assignments whose variable
  the code no longer shows, which register a statement's temporaries get when an earlier
  local is kept in r0 (leaf functions returning a local), the USB library's union locals, and
  floating point through the library helpers (`__aeabi_f2iz` and others at 0x08036930 on, called
  without the source's float types).
- **No o0c draft (31 rows, 4.6 KB):** library and startup code with `pop` without `pc`, `ldm`/`stm`
  writeback, predicated stores, `msr`/`mrs psp`, `bx` through a register; rows whose branches leave
  the row (map pieces, 8 rows); one function with a label inside a skipped region.

### Resume (round 3)

These commands ran the pipeline at the time. They need the scripts from the `v1.0.0` tag.

```sh
cd inverters/gen3-lv/a316
export PATH="/Applications/Wine Devel.app/Contents/Resources/wine/bin:$PATH" EXHUME_ARMCC_RUNNER=wine WINEDEBUG=-all
PY=$EXHUME/.venv/bin/python
$PY tools/o0c.py gen                          # drafts in build/o0c (5 s); failures.tsv says why a row has none
$PY tools/o0c.py score --source o0c --all --jobs 5    # combination 0 of every o0c draft (results.tsv untouched)
$PY tools/o0c.py score --source ghidra --all --jobs 5
$PY tools/o0c.py compare --before build/automatch/results-before-round3.tsv   # the tables above
$PY tools/automatch.py base --source o0c --jobs 5     # o0c drafts into results.tsv (keeps the best)
$PY tools/automatch.py rescore --jobs 5
$PY tools/automatch.py climb --source o0c --jobs 4 --order size --max-size 2048 --budget 10 --restarts 1 --max-builds 800 --pass NAME
$PY tools/automerge.py run --all --jobs 4    # prove files; whole ones go to src/auto
$PY tools/automerge.py blockers --max 2      # blockers of files nearly proved, for climb --only
$PY tools/o0c.py show ADDR [--unit]          # a draft, its unit and its diff against the image
$PY tools/o0c.py asm ADDR; $PY tools/o0c.py blocks ADDR   # what the translator decoded and how it split it
$PY tools/o0c.py mine --min-score 1 --max-score 150 --jobs 1   # recurring mismatch shapes of o0c drafts
$PY tools/o0c.py probe snippet.c             # compile C with the project's armcc and list the code
```

Do not run `automatch.py base` while a climb runs: each process writes its own copy of
`results.tsv`. `o0c.py score`, `show` and `mine` are safe beside a climb (Wine prefixes job20 up).

Next steps worth scripting: fix the library name at 0x08014DAC and the file boundary it makes;
float types for the `__aeabi_f*` helpers; a struct type per base symbol so accesses through
`base + offset` keep armcc's base register the way field accesses do; the giants' switch arms
split into case functions for the climb; and the ties pass (`AUTOMATCH_TIES=1`) run per file,
keeping a source only when the file's proof improves.

## Round 4 (range C: 0x0802F000 to the end)

I worked by hand on the files of 0x0802F000 onwards, with the drafts as a starting point and
the shapes checked by compiling probes. The hand sources are in `build/hand/<ADDR>.c`;
`automatch.py show ADDR --file F --record` scores one and keeps it as the function's best.

### The compiler is armcc 5.06 (MDK 5.16 or later), not 5.05

Seven instructions in the image are `subs rd, rn, #0` where armcc 5.05 (MDK 5.11 to 5.15)
writes `movs rd, rn` for the same source: an assignment whose value is tested at once
(`while (--n)`, `if ((r = f()) == 0)`). MDK 5.16 and every later release I have write `subs #0`.
So the home in `exhume.toml` is now `toolchain/MDK516`, the same release as the library. The full
gate passes with it (every file proved before compiles to the same bytes), and a rescore of all
470 rows changed four scores, all to 0: sub_08017FCC (outside my range), sub_08035084,
sub_08035122 and sub_08034EC8.

### What the code in my range is

- 0x0803083C to 0x0803251C is FatFs R0.12c (`ff.c`), with `_FS_READONLY 0`, `_FS_MINIMIZE 0`,
  `_USE_LFN 0`, `_FS_RPATH 0`, `_FS_EXFAT 0`, `_FS_LOCK 0`, a single-byte code page (`ExCvt` at
  0x0803762E, the map's data row c_0803762E) and `_MAX_SS 512`. R0.12c and not an earlier R0.12:
  `create_chain` tests `free_clst <= n_fatent - 2` and `f_lseek` moves `fptr` before following
  the chain. I used the published source as a reference and wrote each function in its shape,
  with the objects in `src/include/fatfs_types.h`. Things that only show as `nop`s are the
  configuration tests the source keeps in C: `if (_FS_RPATH && ...)`, `(!_FS_EXFAT || ...)`,
  `if (_FS_EXFAT && ...)` (with `_FS_EXFAT` 0 they compile to a `nop`).
- 0x08032550 to about 0x08033400 is the STM32 USB OTG library (`usb_core.c`, host side). Its
  union locals (`USB_OTG_HPRT0_TypeDef` and the like) and its register structs are in
  `src/include/usb_otg_types.h`. With the registers reached through the real structs
  (`&pdev->regs.HREGS->HCFG`) armcc gives the value of a register write the higher register,
  as in the image; through casts of offsets it does not.
- `res` is an `int` in my hand sources where FatFs has `FRESULT`: armcc's enums are one byte, so a
  callee declared `int` in a merged file (the drafts' prototypes win there) would add a `uxtb`.

### Rules found by probing

- A byte array in a struct indexed by a constant (`dp->fn[11]`, or `(p + 0x20)[11]` when the
  base offset is not tiny) loads with `movs rX, #off; ldrb rY, [rX, rBase]`; `p[0x2b]` folds the
  offset into the load.
- `256 * p[1] | p[0]` loads `p[1]` first; `p[1] << 8 | p[0]`, `p[0] | p[1] * 256` and the other
  orders load `p[0]` first.
- A function that calls one defined earlier in the same source file pushes only what that
  callee leaves (`push {lr}`, the argument kept in r2). `automerge.groups()` cannot see that file
  boundary, so a group whose first row's map file column names the group before it now joins it
  (`join_by_map`); I set the column for 0x08032954 to 0x08032AFE to `auto/f08032550.c`.

### Tool changes

- `automatch.py show ADDR --file F [--record [--force]] [--quiet]`: score a hand source and keep
  it (a hand source replaces one of the same score).
- `automerge.py`: project headers other than the two standard ones are hoisted to the merged
  file's head with their `../include/` path (and copied next to the build); a `c_` symbol that
  has a row in `map/data.tsv` stays a symbol instead of becoming a constant address (a constant
  address is split into `ldr =0x08037000; ldrb [rX, #0x62e]`); `run --all --whole` places every
  function at its address even when one does not score 0; `join_by_map` (above).
- `o0c.py probe` prints each instruction's encoding (`movs` and `subs #0` look the same in
  fromelf's text).

### Results so far

36 files proved whole in my range (22,854 bytes of `src/auto`, 158 functions), plus the StdPeriph
`stm32f10x_tim.c` unchanged (3,610 bytes, `src/stdperiph/`): it scored 1 only because armcc
places an intermediate literal pool inside `TIM_OC4Init`, and the map row 0x0802F4B4 was the
part of `TIM_OC4Init` after that pool (merged back into it). `rcc` and `adc` (outside my range)
may be the same story under MDK516 and are worth one try with `src/stdperiph` wrappers.

| what | files | bytes |
|---|---|---|
| FatFs R0.12c (`ff.c`, groups from 0x0803083C to 0x0803240C) | 13 | 6,720 |
| USB OTG core and host interrupt handling (`usb_core.c`, `usb_hcd*.c`) | 5 | 6,712 |
| USB host core, control requests, channels (`usbh_core.c`, `usbh_ioreq.c`, `usbh_hcs.c`, `usbh_stdreq.c`) | 9 | 3,584 |
| USB host mass storage (`usbh_msc_core.c`, `usbh_msc_bot.c`, `usbh_msc_scsi.c`) | 3 | 3,400 |
| USB host CDC class (`usbh_cdc_core.c`, `usbh_cdc_funct.c`) | 2 | 1,492 |
| the task scheduler (`rtos_types.h`), SysTick set-up and the task stack set-up | 4 | 946 |
| StdPeriph `stm32f10x_tim.c` | 1 | 3,610 |

Gate: PASS, image IDENTICAL; proved C 35,592 bytes (24.5%) of the image, from 9,224 when I
started (with the MDK516 home).

Things that did the most work, beyond reading the library sources:

- **Real types.** With the library's own structs and register unions most functions compiled
  to the image's bytes at the first try, where the drafts' casts of offsets had failed: armcc
  orders operands and picks registers differently for a named object or a struct member than for
  `*(T *)(p + off)` (a register write puts its value in the higher register; `x -= s.f` loads
  `s.f` first). Typed RAM symbols (`extern uint g_2000051C;`) are now allowed in hand sources.
- **Context armcc does not use.** A `static` callee defined earlier in the file does not give its
  callers the register knowledge a global one does (`USBH_GetFreeChannel`, `sub_0803419A`).
- **Whole-file only shapes.** Literal pools and strings placed mid-function, a literal shared by
  functions (`ldr r2, =MSC_Machine; adds r2, #8`, `ldr =&tbl[10]; subs #0xc8`) and branch-island
  placement only come out in the file compile, so a few functions score 1 to 7 alone and still
  prove; `automerge.py run --all --whole` places every function for that test.
- **Statements that only leave a `nop`:** a block-scope declaration with an initialiser at the end
  of a block (`URB_STATE s = f();` inside the `if`), an empty `else if (x == 3) { }` at the end of
  a chain, `(void)0;` as an if body, configuration tests (`if (0 && ...)`).

What is left in my range:

- 0x08030450 to 0x080304E3 (`sub_08030450`, `08030468`, `08030476`, `08030486`) is the
  scheduler's assembly part (PendSV context switch with `msr psp`, `stm`/`ldm` of r4-r11, `cpsid`):
  hand-written assembly, not C.
- 0x08036B40 to 0x080376AF is constant data of many files (two 256-byte tables, a 0x570-byte table,
  strings, FatFs's `ExCvt` at 0x0803762E); it needs the owning files to define their const data.
- From 0x0803675C on the rows are C library members the gate links from the library.
- Some hand sources keep a `volatile` local only to reproduce a dead store to the stack
  (`volatile uint unused = 0;` in the CDC transmit and receive functions, `cpu_sr` in the
  scheduler): the original probably had an unused initialised local I could not name.

## Round 4 (range B: files starting 0x08020000 to 0x0802EFFF)

I worked by hand on the C where the drafts' structure was wrong, one function at a time, and
proved each file with `automerge.py run --only`. My helper (not in tools/) compiled a candidate
source in file context on Wine prefix job20, scored it, showed the diff and the branch targets that
disagree, and recorded it in `results.tsv` and `best/`. Every function stays C.

### What armcc -O0 does that the drafts missed (each checked with probes)

- **Named variable or cast pointer.** In `a + b`, armcc loads an operand reached through a cast
  pointer (`*(int *)(g_X + 0)`, `((S *)g_X)->f`) before a named variable, whatever the source
  order. The image often loads in source order, so the symbol must be declared as the scalar it is
  (`extern ushort g_20000476;`). Same for call arguments and compares. `automerge.merge` now
  accepts such a declaration from one function and rewrites the other functions' `uchar[]` uses
  of that symbol through its address.
- **Value first, then address.** A store to a member array or to a member of a nested struct at
  a nonzero offset (`c.tx[1] = 4`, `g.s.b = 0`) evaluates the value into r1 and the address into
  r0; a plain member store does the opposite. Cast macros keep this
  (`((struct N *)g_X)->s.b`), so the pattern tells struct layout.
- **Volatile bitfields.** `ldrb r0,[x]; movs r0,#7; strb` is a store to a volatile 8-bit
  bitfield; `movw r1,#k; ldrh; bfi r0,r1,#0,#16; strh` the same for 16 bits.
- **`short` member stores** keep an `sxth` when the member is declared `short` in a struct, not
  when the store goes through `*(short *)p`.
- **Switch tails.** A last case ending in an `if` gives a nop for the if's end and a nop for the
  switch end (breaks jump to the second). `default: break;` after the last case adds one more;
  `case 0: case 5:` gives case 0 its own nop. `nop; nop` after a switch whose dispatch is one
  compare is `switch (x) { case 0: ...; break; default: break; }`.
- **Constant reuse.** `if (x == 8) g = 8;` stores the compared register without reloading;
  the drafts had `g = x`.
- **Flash tables.** A `const` table indexed at run time must stay a symbol
  (`extern const uchar c_08037622[];`); automerge's constant-address form lets armcc fold the
  table address into the load and changes the size. All functions of a file must use the same
  form or the literal pool gets two entries.
- **Expression shape.** `(rx + 5)[i * 2] + ((rx + 4)[i * 2] << 8)` and
  `... + (R[0x3d] & 0xff)` were the only forms that gave the image's register choices.

### Map

0x08024D46 was listed as a C library member (`____mathlib_complex_invalid`), which split the file
at 0x08024C64 in two. `drafts.library_addrs` now ignores library names found inside the
application range (0x080143C8 to 0x0803675C), as docs/gate.md already said they are coincidences;
the file f08024C64 now runs to 0x08026BA3 (25 functions after the two row merges below, 8,000 bytes).

### Compiler release

The compile home is now MDK 5.21 (`exhume.toml`). f0802B028 and f0802C0D6 only match with 5.21 or
later: 5.11 to 5.20 reuse the flags of the first compare in a switch's range check, where the image
has a second `cmp`. A full gate with MDK521 passes with every proved file, and the library link
already said 5.16 to 5.21, so 5.21 is the release that fits both.

### More shapes

- `x = x` or `x = 0` when x already holds 0 compiles to `nop`. A self-initialised local
  (`ushort t = t;`) gives a `nop` in the prologue.
- `abs()` is inlined: `cmp; blt; mov; b; rsbs`.
- Float code needs real `float` types: `v * 10.0f` gives `bl __aeabi_fmul; mov r7, r0; bl
  __aeabi_f2uiz` (the `mov` is armcc's spill of the float temporary). A source gives the helpers'
  addresses with an indented at: comment (`  /* at: __aeabi_fmul 0x080369A8 */`), which automerge
  keeps.
- Register choice in one expression can depend on the shape of another statement elsewhere in the
  function. In f0802C0D6, rewriting one bit test as `(A & m) == (m | 0)` fixed the registers of
  two other tests. I found that with a batch search over whole-file compiles (non-split sections:
  island and pool placement also differ between split and whole-file compiles).
- A caller pushes an odd number of registers (no `ip` padding) only when every callee is defined
  earlier in the same file.

### Map

Two more fixes: 08024FD4 runs to 0x08025143 (it took the row at 0x080250E8, which began inside its
inner literal pool), and 08025494 runs to 0x080256D7 (it took the row at 0x08025500).

### Results (range B)

Eleven files proved whole, 41,232 bytes: f080274A0 (10,212), f08024C64 (8,000, now including the
old f08024D50), f0802B028 (6,436), f080234B4 (5,408), f0802C94C (3,420), f08029DEC (3,260),
f08026BA4 (2,256), f0802AAC8 (1,048), f08023154 (864), f0802AEE0 (244), f0802AFD4 (84). The full
gate passes: proved C 50,384 bytes, 34.7% of the image.

What is left in the range: f0802D6A8 (64 bytes) is the startup file's assembler. f0802039C
(11,688 bytes) cannot be proved on its own. Its handlers 08020A32 and 08021464 push nine registers
with no padding, and the padding goes away when sub_08017FCC, sub_0801FA04 and sub_0802007C are
defined earlier in the unit. So 0x08017FCC to 0x08023143 is one source file (f08017FCC, f0801C160,
f0801FA04 and f0802039C), and it waits on the two 14 KB functions. My sources for 0802039C,
080206A4, 08022C50 and 08022F7A score 0; 08020A32 is only off where the padding and the padding's
register effects show.

## Round 4 (startup, assembly, rcc/adc)

Three things that were blobs or unproved are now proved whole in the gate: StdPeriph `adc` and
`rcc`, the startup file with the vector table, and the scheduler's context switch. The compile
home is now MDK521 (`exhume.toml`): range B found that 5.16 to 5.20 add a `cmp` to a switch
dispatch in f0802B028. Every file proved before compiles to the same bytes under it.

### StdPeriph adc and rcc

Round 1 scored `stm32f10x_adc.c` 1 and `stm32f10x_rcc.c` 2 and took them for vendor edits.

- `adc` is V3.5.0 unchanged. As with `tim`, the score came from the listing, not the source;
  `src/stdperiph/stm32f10x_adc.c` is the same kind of wrapper as the others and matches whole.
- `rcc.c` itself is unchanged too, but `HSE_VALUE` is 8000000 where V3.5.0's `stm32f10x.h` sets
  25000000 for `STM32F10X_CL`. `RCC_GetClocksFreq` loads 8000000 for the HSE case and for the
  PREDIV1 source, and its pool has no 25000000, so the build's pool was one word longer and every
  function after it sat 4 bytes late. The wrapper compiles with `-DHSE_VALUE=8000000`; an edited
  `HSE_VALUE` line in `stm32f10x.h` gives the same bytes, so I can't tell which the vendor did.
  The board has an 8 MHz crystal.
- `rcc` owns RAM data: `APBAHBPrescTable` (16 bytes at 0x20000491) and `ADCPrescTable` (4 bytes
  at 0x200004A1). The map's row at 0x200004A1 was 5 bytes long; I split it into `ADCPrescTable` (4)
  and `g_200004A5` (1) and named both tables in `map/data.tsv`.

### The startup file

`src/startup/startup_stm32f10x_cl.s` is V3.5.0's `startup/arm/startup_stm32f10x_cl.s` with two
lines changed: `Stack_Size` 0x800 (shipped 0x400) and `Heap_Size` 0x1500 (shipped 0x200). It is
the vector table at 0x08014000 (0x150 bytes, the `RESET` area), `Reset_Handler`, the ten default
handlers and `__user_initial_stackheap` at 0x0802D6A8 (0x40 bytes with its pool), and the heap
at 0x20008898 with the stack after it (`__initial_sp` 0x2000A598). Twenty vector entries point
at the application's own handlers. I found which handler each is from its place in the table
(NMI 0x0801432C, PendSV 0x08030486, SysTick 0x08014B68, RTC 0x0802ADDA, EXTI1 0x08014B4C, ADC1_2
0x080146F4, TIM2-4, USART1-3, UART4, OTG_FS and the fault handlers); the source gives their
addresses on `; at:` lines, since most of those map rows are in other agents' ranges and keep
their placeholder names. To make that work the gate wraps armmatch's `place` (`place_weak`,
docs/gate.md): an annotated weak symbol is taken as overridden at that address. The vector table
needed a map row to give its size: `08014000 150 __Vectors` in `map/data.tsv`. That is a new row
inside range A's addresses; no existing row there changed.

The library link still has `__user_initial_stackheap` at 0x0802D6C4 in `lib_symdefs`; the address
is now the startup file's own symbol, so nothing changed there. The startup file needs
`__use_two_region_memory` (heapauxi, 0x08036868) and `__main` (0x08014150) named on its `; at:`
lines for verify, which links without the library.

### The context switch (`src/rtos/os_cpu_a.s`)

0x08030450 to 0x080304E3 is hand-written assembly in the shape of uC/OS-II's Cortex-M3 port
(`os_cpu_a.asm`), cut down for the application's small scheduler. The callers in `src/auto` say
which routine is which: OSStart (0x080305FC) calls 0x08030450, so that is `OSStartHighRdy`;
OS_Sched (0x080302F2) calls 0x08030468, `OSCtxSw`; OSIntExit (0x08030686) calls 0x08030476,
`OSIntCtxSw`; the vector table's PendSV entry is 0x08030487, `PendSV_Handler`. The variables are
the ones `rtos_types.h` names: `OSPrioCur` 0x200004CC, `OSPrioHighRdy` 0x200004CA, `OSTCBCur`
0x200004D4, `OSTCBHighRdy` 0x200004D0. It is not Micrium's file: it uses R4 and R5 as scratch and
pushes them, writes 0xFFFF0000 as a word to SHPR3 (PendSV and SysTick both lowest), has no
OSRunning, no OSTaskSwHook and no OSStartHang loop (OSStartHighRdy runs on into OSCtxSw), copies
a one-byte priority, and enables interrupts between saving and restoring.

Two details came from the literal pool. The variables' addresses come before the constants and
not in first-use order, and the first word is OSIntExit's address, which nothing loads. armasm
puts `LDR Rn, =x` literals in first-use order at `END`, so the variables were explicit `DCD`
words after the code, loaded pc-relative (`LDR R4, __OSTCBCur`), and the constants' pool followed
them at `END`. An unused `DCD OSIntExit` is the only way I found to put that word there. The
`nop` at 0x08030484 is a `NOP` in the source: an `ALIGN` would add nothing at a 4-byte boundary.
`LDR R5, =NVIC_PENDSVSET` comes out as `mov.w` (armasm picks a move when the constant fits), and
`SUB`/`ADD R0, R0, #0x20` are 32-bit because they don't set flags.

I renamed the four map rows (range C) to these names; armmatch keeps the old `sub_` names as
aliases, so the C files that call `sub_08030450` and the others still link.

### Results

Gate (full, MDK521 home): PASS, image, bin and hex IDENTICAL. 99 of 99 sources match whole.

    proved sources (C and assembly)    38522  26.5%  src/auto 26794, src/rtos 148, src/startup 400, src/stdperiph 11180
    C library members                   1208   0.8%
    region table + compressed RW         248   0.2%
    original bytes (blobs)            105342  72.5%

New since range C's round: adc 1,104 bytes, rcc 1,280 bytes (bytes in flash), the startup file 400
(vector table 336, code and pool 64), the context switch 148.

## Round 4 (range A: files starting 0x08014000 to 0x0801FFFF)

I worked by hand on the C, one function at a time, compiling each candidate in file context and
proving files with `automerge.py run --only`. Every function stays C. The compile home is MDK 5.21
now (`exhume.toml`, from range B); I rescored under it.

### Files proved

| file | functions | bytes | what it is |
|---|---|---|---|
| f0801432C | 19 | 2,424 | the interrupt handlers (USART1 receive, ADC, HardFault...) |
| f0801756C | 13 | 2,656 | the EEPROM block reader/writer (crc, put/get big-endian) |
| f08014F24 | 8 | 1,108 | the UART queues (transmit start, transmit ISRs, receive) |
| f08014CA4 | 4 | 640 | CMSIS system file: SetSysClockTo72, SetSysClock, SystemInit, SystemCoreClockUpdate |
| f08014234 | 2 | 220 | main and its neighbour |

That is 7,048 bytes more than before. The full gate passes (proved C 16,200 bytes, 11.1%).

### Close but not proved

- **f08015378** (8,692 bytes, 6 functions): five functions are byte-equal in the file. The last
  one, sub_08015D02 (4.8 KB), differs in one instruction: `movw r1, #0x60e; asrs r5, r1, #8`
  where every source I tried gives `lsrs` (armcc knows the constant is not negative). I tried
  every integer type for the local, casts, an assignment inside the expression, a const, other
  MDK releases.
- **0x08017FCC to 0x08023143** is one source file (range B's finding; I joined it in the map's
  file column and `automerge.groups()` now honours that column). Compiled whole, its bytes equal
  the image's (relocated words masked) up to 0x0801FD35. What is left: sub_0801FA04 (4 places
  where armcc puts `(0xffff & (hi << 8)) + lo` in the other register), and sub_08021464 (a few
  `mov r0, r1` choices in its byte-pair stores; with them its size differs by 12 bytes and the two
  functions after it shift). The two 14 KB functions, sub_0801C160 and sub_08018816, now match
  (0 and 1; the 1 is an alignment nop that depends on the address). **The file is 45 KB, and
  MDK-Lite's armlink refuses to link more than 32 KB, so `armmatch` cannot prove it even when
  every byte is right.** It needs a check that compares the object's bytes with relocations
  masked, or a full linker.

### What armcc -O0 does that the drafts missed

- **A static function changes its callers.** sub_08014DAC pushes `{r4, lr}` only when the
  function it calls is `static`; a caller of a known non-static leaf pushes only `lr`.
- **Plain variables and cast pointers** (as range B found): armcc evaluates an operand reached
  through a cast pointer (`*(ushort *)(g_X + 0)`) before a named variable. The drafts' DAT_
  macros are cast pointers, so I declared such globals with their type (`extern ushort
  v_200002C2;`). `automerge.merge` gives every `v_` symbol an at: line, and where one function
  declares `v_X` and another uses the template's `g_X`, it rewrites the second through `&v_X`, so
  the file keeps one literal pool word per address (two symbols at one address made two pool
  entries and moved every later load).
- **Struct arrays.** A store to an element of a struct array at a constant index
  (`tasks[5].c = 0`) loads the value first into r1 and the address into r0; a plain struct or
  array store does the opposite. The task table at 0x20007C6C is `struct task[]`
  (src/include/a316_types.h). A 2D member array behaves the same, a 1D member array gives
  `mov r0, r1` before its second store: this told me the layout of the settings block at
  0x20001AAE (`t[5][2]` at 0x13, an `int a[2][8]` at 0x194).
- **The type of one local moves registers elsewhere.** In sub_0801C160 the value local was
  `int` in the draft; as `ushort` (with no visible change at its own uses) about 600 differing
  instructions went away, because armcc then picks r0 rather than r1 for the address of each
  store of it. In sub_080146F4 the ADC buffer had to be `short[]`, in sub_080171DA a pointer
  `signed char *`, in sub_08016FDC a local `int`. `tools/handperm.py` and a signedness scan over
  every declaration found these.
- **Packed structs and pointer locals.** The UART queues are 9-byte `__packed` structs; with a
  pointer local `p = &tx[n]` armcc emits `adds r4, #1` and a write-back `strb r0, [r4, #-1]!`,
  which o0c cannot translate. `p = tx; p += 1;` gives the image's `ldr; adds r4, #9` (where
  `&tx[1]` folds the offset into the literal).
- **`buf[i + 2]` and `buf[2 + i]`** compile differently: the second computes the index before
  loading the base, as the image does.
- **`switch` with one case 0** compiles to `cbnz` like an `if`, and leaves the two nops
  (break and switch end) that the image shows at the end of the USART handler.
- **Constant reuse**: `if (busy == 1) return 1;` returns the compared register;
  `if (GPIO_ReadInputDataBit(...) == 0) x = 0;` stores the call's result register.
  armcc also builds constants from known registers: `0xFFFFFFFF` as `subs r0, r4, #2` when r4
  is known to be 1, `0x08000000` as `lsls r0, r0, #19` from `0xE000E100` in r0.
- **`&& 1`** in a condition leaves a `nop` (a jump to the next instruction); four register
  handlers in the 14 KB functions have it.
- **A function that never returns** (`while (1)` with no exit) has no push at all.

### Tools

- `drafts.library_addrs`: a names.tsv entry is not a library row when the map gives the row an
  application file (0x08014DAC was "__alloca"; it is SetSysClock and belongs to f08014CA4).
- `automatch.py try ADDR FILE [--save] [--home MDKnnn]`: score a hand-written source in file
  context, show its diff, keep it as the best when not worse. `AUTOMATCH_SHOW_JOB=20` makes
  `try`, `show` and o0c's `show`/`probe` reuse an existing Wine prefix.
- `tools/handperm.py`: armpermute's mutations on a hand source, scored as automatch scores.
- `o0c.py show --draft FILE --write UNIT`: render an edited draft into a unit to work on.
- o0c's switch walker keeps the table offsets per path (one register holds `x - 0x129` on one
  path and `x - 0x84` on another; the walker had taken the last one for all), and looks up a
  re-based switch register before the switch value itself. The 14 KB functions now have their
  385 cases (202 before), and the dispatch compiles to the image's.
- `automerge.merge`: `v_` and `extern const` `c_` symbols get at: lines; project headers in
  src/include are hoisted into the merged file; `g_`/`v_` unification as above; groups joined by
  the map's file column.

## Round 5 (constant data)

The constant data at 0x08036B40 to 0x080376AF (2,928 bytes) was an original-byte blob. armlink
puts every object's RO data after all the code, in link order, so the region is the const data of
the files that have any, one after another in the order of their code.

### What is in the region

I looked for every word in the image (and in the initial RAM image) that points into the region.
There are 11, all literal pool words, and no pointers from data. The region has no strings.

| address | bytes | what | owner (the file whose code loads it) |
|---|---|---|---|
| 0x08036B40 | 256 | Modbus CRC-16 high byte table | f08017FCC (sub_08018386), not proved |
| 0x08036C40 | 256 | Modbus CRC-16 low byte table | f08017FCC, not proved |
| 0x08036D40 | 1,392 | grid settings, `ushort [2][29][12]`; the second 29 rows are all zero | f080234B4 |
| 0x080372B0 | 580 | grid settings, `ushort [29][10]` | f080234B4 |
| 0x080374F4 | 232 | grid settings, `ushort [29][4]` | f080234B4 |
| 0x080375DC | 58 | grid settings, `ushort [29]` | f080234B4 |
| 0x08037616 | 12 | `table_week`, the month correction for the day of the week | f0802AAC8 (the RTC code) |
| 0x08037622 | 12 | `mon_table`, the days of each month | f0802AAC8 |
| 0x0803762E | 128 | FatFs `ExCvt`, code page 437 | f080311C0 (ff.c) |
| 0x080376AE | 2 | zero padding before the region table | armlink |

The order agrees with link order: the CRC tables' file starts at 0x08017FCC, the grid tables'
at 0x080234B4, the RTC file at 0x0802AAC8 and FatFs at 0x0803083C. The grid tables have one row
per country code (29 of them): voltage and frequency limits (1860 is 186.0 V, 4753 is 47.53 Hz)
and times. The RTC file is the common STM32 RTC calendar code (`RTC_Get_Week`, `RTC_Set`,
`RTC_Get`), whose two tables have those names. f08015378 has no RO data.

### What armcc does with const data (probes)

- Global `const` arrays go to the object's one `.constdata` section in definition order, each at
  its own natural alignment with no other padding; the section's alignment is the largest one in
  it. A `ushort` table followed by `uchar` tables gives a 2-aligned section, which is why the RTC
  tables start at 0x08037616.
- A function using such an array loads the array's own address from its literal pool, one word
  per array, whether the array is defined in the same file or not. So defining the data in the
  file that uses it changes no code.
- A string literal used in code goes into that function's literal pool and is taken with `adr`,
  with or without `--split_sections`. A string that only a data initialiser points at (`const
  char *const p[] = {"a"}`) goes to `.conststring`, each string 4-aligned; a `const char s[]` goes
  to `.constdata`. None of this shows in this image: no strings are in the region.

### Changes

- f080234B4, f0802AAC8 and f080311C0 now define their tables with their real types and the
  image's values, placed after the includes, in address order. The `extern const uchar c_...[]`
  lines are gone; the casts that read the grid tables as bytes now cast the array
  (`(const uchar *)c_08036D40 + 0`). The code of all three compiles to the same bytes, and verify
  now also compares the data (1,072, 3,004 and 7,670 bytes, each a whole match).
- `map/data.tsv`: the rows at 0x08037616, 0x08037622 and 0x0803762E are now `table_week`,
  `mon_table` and `ExCvt`; `ExCvt` is 0x80 bytes (the row was 0x17A, running to the image end over
  the padding and the region table). The file column of the nine rows names the owning file.
- `tools/gate.py` (`group_lines`): the region at 0x08036D40 is four sections that meet off a
  4-byte boundary, the three `.constdata` sections and the 2-byte blob. The gate refused groups of
  more than three, since only the first and last can be pinned. Same-named sections between the
  pinned ones keep the order of the input list, which is the proved objects sorted by name, so the
  gate now takes a longer group when its middle sections are same-named sections of proved
  objects in that order. The link listing check still confirms every section's address.
- `build/data/f08017FCC_data.c` has the two CRC tables as C for the 45 KB file, checked with
  armmatch on its own (512 of 512 bytes at 0x08036B40). Those bytes stay a blob until that file
  is proved.

### RW data

Of the proved files only `stm32f10x_rcc.c` owns initialised RAM (`APBAHBPrescTable` and
`ADCPrescTable`, 20 bytes at 0x20000491). Taking its values from the object would mean running
the library link after verify instead of beside it, splitting the RW blob around rcc's `.data`
and letting armlink drop rcc's code. That is possible but not a small change, so I left the
library link on the original's initial RAM. Verify already checks rcc's data.

### Results

Gate (full): PASS, image, bin and hex IDENTICAL. 115 of 115 sources match whole.

    proved sources (C and assembly)    89216  61.4%  src/auto 77488, src/rtos 148, src/startup 400, src/stdperiph 11180
    C library members                   1208   0.8%
    region table + compressed RW         248   0.2%
    original bytes (blobs)             54648  37.6%

Of the region's 2,928 bytes, 2,414 now come from proved sources. Left: the CRC tables (512, waiting
on f08017FCC) and the 2 bytes of padding before the region table, which no object owns.

## Round 5 (the 45 KB file, f08015378)

I finished the two files range A left close. Every function stays C (no `__asm`, no intrinsics).

### f08015378 (8,692 bytes, 6 functions): proved whole

The one instruction left in sub_08015D02 was `movw r1, #0x60e; asrs r5, r1, #8`. The local is
`ushort`, not `short`: armcc promotes a `ushort` to `int` for `v >> 8` and then uses an
arithmetic shift even when it has just loaded a constant that is plainly positive. A `short` or
`int` local holding 0x60e gives `lsrs`, so do casts on the shift (`(unsigned)v >> 8` gives
`lsrs` even with a `ushort` local). I found it in a probe with `v = A[0x68]` before trying the
constant. `automerge.py run --all --only 08015378` reports WHOLE FILE MATCHES (8692/8692), and
the fast gate passes with it in src/auto (proved sources 95,494 bytes, 65.7%).

### f08017FCC (0x08017FCC to 0x08023143, 45,432 bytes, 15 functions): every byte equal

armlink from MDK-Lite cannot link it, so I checked the object: `build/lst2.py
build/automatch/files/f08017FCC.c 08017FCC` compiles the merged unit whole (no split
sections) and compares the listing's `.text` with the image. The result is `size 0xb178; masked
1416; differing bytes 0`: the size is exactly 45,432 bytes. The 354 masked words (261 literal
pool words that name a symbol, 93 `BL`s) I then checked one by one against the image (each pool
word equals the symbol's address from its name or its at: line, plus the addend; each `BL` in the
image reaches the function it names): 354 right, 0 wrong. The file's `.constdata` (the two Modbus
CRC-16 tables, 512 bytes at 0x08036B40, defined in sub_08017FCC's source from
build/data/f08017FCC_data.c) equals the image too; I read it from the object with `fromelf -d`.

What it took:

- **sub_0801FA04** (`(0xffff & (hi << 8)) + lo`, 4 places where the and went to the wrong
  register): the low byte must be read through a cast pointer,
  `((uchar *)&v_200007A4[n])[0x47]`, while the high byte stays `v_200007A4[n].b46[0]`. This is
  the named-or-cast-pointer rule again; it changes which register armcc gives the `and`, not
  the load order.
- **sub_08021464**:
  - The draft's `__nop();` lines were `v4 = 0;` where v4 already held 0 (range B's rule). The
    intrinsic also makes armcc forget register contents, so after it the image's reuse of r1
    turned into a fresh `ldr r1, [pc]`.
  - The decompiler's `goto`s were wrong in two places: a label in front of a `goto` gives a
    `nop`. The tail is a plain chain, `if (a && b) {...} else if (c && d) {...} else if (e && f)
    {...}`. In it, the false branch of the last test jumps to the branch after the first loop,
    as the image does.
  - The `mov r0, r1` before the second store of a byte pair: armcc emits it when it
    knows the stored value fits in a byte (a `ldrb`, `& 1`, a `ubfx`, a constant). In the image,
    about 230 pairs in this function follow that rule and one does not
    (`ldrb r4, [r0, #0x68]` with no mov). With `+ 0`,
    `& 0xff`, `| 0`, casts or `<< 0` the `mov` stays, so armcc still sees a byte through them.
    `+ 0x100 - 0x100` compiles to just the `ldrb` with no `mov`: the sum folds away in the code
    but armcc no longer treats the value as a byte. I could not find a more
    natural source; the line has a comment.
- **sub_08018816** (1 in automatch) and **sub_08020A32** (11) are equal in the whole file: the
  first is the alignment nop range A expected, the second the push padding that only a
  single-function compile shows. automatch's single-function scores for 08021464 (16) and
  08015D02 (21) are branch-island placement only; islands sit where the real addresses put them.
  Use the whole-file compare for these, not `results.tsv`.

### Tools

- `build/lst2.py` (scratch, not in tools/): it now points at this worktree. It skips the
  `.rel.text` section, whose name also contains `'.text'`. It masks a `DCD` only when a `;`
  comes right after the value: before, a switch table word whose ASCII column happened to show
  `;` (`0.;.` at offset 0x47B0) was masked and so never compared. That word was right.
- I wrote two scripts that are not in the repo. One gives a whole-file instruction diff for one
  function (difflib over normalised disassembly). The other checks each masked relocation
  against the image. The checker should go into the object-placement proof being added to
  tools/gate.py, because masking without checking would let a wrong symbol through.

## Round 5 (last bytes)

Two 2-byte gaps of zeros were still blobs. Both are armlink's alignment padding, and the gate now
makes both.

### 0x08034E2A: the file was split wrongly

The map had `sub_08034E2C` as a file of its own (f08034E2C.c), then four more one-function files
and f08034F58.c. They are one file, ST's `usbh_stdreq.c`, from 0x08034E2C to 0x08035288:

| address | function |
|---|---|
| 0x08034E2C | `USBH_ParseDevDesc` (static) |
| 0x08034E84 | `USBH_GetDescriptor` |
| 0x08034EC8 | `USBH_Get_DevDesc` |
| 0x08034F00 | `USBH_ParseEPDesc` (static) |
| 0x08034F20 | `USBH_ParseInterfaceDesc` (static) |
| 0x08034F46 | `USBH_GetNextDesc` |
| 0x08034F58 | `USBH_ParseCfgDesc` (static) |
| 0x08035084 to 0x0803522E | `USBH_Get_CfgDesc`, `USBH_ParseStringDesc` (static), `USBH_Get_StringDesc`, `USBH_SetAddress`, `USBH_SetCfg`, `USBH_SetInterface`, two more control requests (`bRequest` 3 and 1), `USBH_ClrFeature` |

The functions are in an order where each is defined before its first use, not the order of ST's
V2.1.0 release, but they are the same functions. The file before it, f08034DA0.c, ends with
`USBH_IsocReceiveData` and `USBH_IsocSendData`, the last two functions of `usbh_ioreq.c`, and its
code really ends at 0x08034E2A.

The evidence:

- armcc gives a `.text` 4-byte alignment only when it holds a literal pool. On its own,
  `USBH_ParseDevDesc` has none (2-byte alignment), so armlink would put it at 0x08034E2A. Of the
  six files, only f08034F58.c had a literal pool (`USBH_ParseCfgDesc` loads the addresses of
  `prev_ep_size` and `prev_itf`), so the whole of `usbh_stdreq.c` is 4-aligned and armlink pads
  before it.
- f08034EC8.c (`USBH_Get_DevDesc`) calls `USBH_GetDescriptor` and `USBH_ParseDevDesc`, and
  f08034F58.c calls `USBH_GetDescriptor`, the two parse functions and `USBH_GetNextDesc`: all in
  the same ST file.
- The merged file only matches with the parse functions `static`, as in ST's source. With
  `USBH_ParseEPDesc` and `USBH_ParseInterfaceDesc` global, `USBH_ParseCfgDesc` gets a 0x18-byte
  frame where the image has 0x1C: armcc uses what it knows of a global callee defined earlier in
  the file (Round 4 found the same with `static` callees). With them `static` it does not, as
  when they were in other files. I made
  `USBH_ParseDevDesc` and `USBH_ParseCfgDesc` `static` too, as in ST; the bytes are the same either
  way.

I merged the six files into `src/auto/f08034E2C.c` (deleted f08034E84.c, f08034EC8.c,
f08034F00.c, f08034F20.c and f08034F58.c) and set the map's file column for 0x08034E84 to
0x0803522E to `auto/f08034E2C.c`. `USBH_Get_DevDesc` now has its real types (`uchar length`, the
`Rx_Buffer` and `Dev_Desc` members) and `USBH_GetNextDesc` is ST's code; the other bodies are as
they were. The object's `.text` has 4-byte alignment and the gate's padding rule (two proved
sections, zero bytes, ending on the second's alignment) now covers 0x08034E2A.

### 0x080376AE: the padding before the region table

The application's last `.constdata` (FatFs's `ExCvt`) ends at 0x080376AE and the region table
starts at 0x080376B0. In the original link both were in the same region and armlink aligned
`Region$$Table` to 4. The gate's library link makes the table, but armlink will not start a
region at 0x080376AE (L6244E). So the library link now starts the table's region at 0x080376AC
with a 2-byte, byte-aligned stand-in for the end of `ExCvt`, and armlink pads to 0x080376B0 by
itself. With stand-ins of 1, 2, 3 and 4 bytes it put the table at 0x080376B0 each time, with a
PAD of 3, 2, 1 and 0 bytes, and the table and the compressed RW data were unchanged. The gate
checks the PAD row in the listing and that the bytes are zero and the image's (docs/gate.md).

### Result

```
  proved sources (C and assembly)         143854   99.0%
  alignment padding (linker)                  10    0.0%
  C library members (linked from c_w.l, fz_ws.l)    1208    0.8%
  region table + compressed RW (armlink)     248    0.2%
  original bytes (blobs)                       0    0.0%
-- gate: PASS in 113s
```

The image, bin and hex are identical, and no byte comes from the original any more.
