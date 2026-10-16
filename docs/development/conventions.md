# Conventions: the notes in the sources, the maps, the rules

How to read the sources and the maps of both trees, and the rules every change follows. The
method behind them is in [method.md](method.md); the history of each tree's readability pass is
in its `docs/readability.md` ([A316](../../inverters/gen3-lv/a316/docs/readability.md),
[D316](../../inverters/gen3-lv/d316/docs/readability.md)).

## Notes in the sources

Every source file starts with a header comment, and every function has a comment. Some lines in
them are read by the tools; others record evidence.

| Note | Tree | Read by | Meaning |
|---|---|---|---|
| `/* at: name 0xADDR, ... */` | A316 | the gate, `srcfiles.py` | the image address of each function and of the data the file defines or uses; a source is placed by these, not by its directory |
| `at: name 0xADDR; ...` in the header | D316 | people | the addresses of the file's functions (the D316 tools take addresses from `map/functions.tsv`) |
| `/* opts: ... */` | A316 | the gate | armcc options for this file, for example `--cpu Cortex-M3 -O0 --loose_implicit_cast` |
| `// cflags: ...` | D316 | `match.py`, the gate | extra cl2000 flags for this file, for example `-Ooff` |
| `match: ...` | both | people | the code is written in this shape because the compiler needed it to give the image's bytes; a plainer form compiles differently |
| `name: ...` | both | people | the evidence for a name: `original`, `likely` or `descriptive`, with the map file that records it |
| `#define OWN_x` | both | the preprocessor | this file keeps its own declaration of `x` instead of the header's, because its code was compiled against that form; a `match:` comment says why |

A header comment typically says what the file does (its job), its entry points, the data it
owns, its neighbours in the image, and the units it uses. Function comments state the rules the
function applies, with numbers. Where a comment goes beyond what the code shows it says
"inference".

## Names and their evidence

Three kinds of name, in both trees ([A316 names.tsv](../../inverters/gen3-lv/a316/map/names.tsv),
[D316 names.tsv](../../inverters/gen3-lv/d316/map/names.tsv)):

| Basis | Meaning | A316 | D316 |
|---|---|---|---|
| `original` | the name the code had: vendor library code (ST, TI, FatFs, Keil, uC/OS-II) under its own names | 295 | 111 |
| `likely` | strong evidence short of the original name, for example a public register name that fits the use | 79 | 410 |
| `descriptive` | a name that says what the code does with it | 531 | 2,121 |

The numbers are the rows of each `names.tsv`.

`names.tsv` has the columns `addr`, `kind`, `old`, `new`, `basis` and `note`: the note is the
evidence, usually the functions that read or write the name and what they do with it. A rename is
logged there with the old name.

Placeholder names keep an address until a meaning is known: `sub_ADDR` and `g_ADDR` in A316,
`sub_XXXXXX`, `var_XXXX` and `c_XXXXXX` in D316. Fields of the ARM-DSP pages whose meaning the
ARM code does not show are named by position, `dsp_pN_wK` or `dsp_pN_bK` (page N, a word or byte
at offset K from the page number byte). Several A316 names are known to be misleading and kept
for now, for example `board_init_done`, which is the meter fault flag; the notes say so where it
matters.

## The maps

| File | Tree | Columns | Content |
|---|---|---|---|
| `map/functions.tsv` | both | address, size, name, kind, file (A316) or notes (D316) | every function in the image |
| `map/data.tsv` | both | address, size, name, kind, file or notes | every data object, RAM variable and (D316) peripheral register |
| `map/names.tsv` | both | above | every rename and its evidence |
| `map/filenames.tsv` | both | old files, new file, kind, evidence | each source file's name and why |

Addresses are flash or RAM addresses of the image: bytes for the ARM, 16-bit words for the DSP. A
D316 ramfunc row has its load address in `addr` and its run address in the notes (`run=0x....`).
The D316 function kinds are `c`, `ramfunc`, `rts` (a runtime library member) and `asm`.

## File names and layout

Sources live under each tree's `src/`, by subsystem, under their original file names where the
evidence gives them (`map/filenames.tsv`). The tools find a source by walking `src/`, by its stem
or (A316) by an address on its `at:` lines, so a file can be moved or renamed with `mv` alone.
Two rules hold:

- **Stems are unique across `src/`.** Objects and work directories are keyed by stem; the gates
  check it.
- **No file is split or joined casually.** The compiler's output depends on the whole file: the
  functions before a function (both compilers use what they learnt about a callee defined earlier
  in the file), the literal pool (armcc puts one at the end of a file's code), and on the DSP the
  order of the file's variables in `.ebss`. A316 keeps some original files in parts named
  `<name>_pN.c` for these reasons.

## The rules for a change

1. **The gate stays IDENTICAL.** Run the tree's full `tools/gate.py` after every change. A change
   that alters one byte is not a readability change.
2. **Prove each edit.** Readability edits can change code: armcc at -O0 compiles a plain use of
   a variable differently from the same address through a cast, and CGT 16.9.6 reorders a file's
   variables when a header changes. The gate tells which file changed; `tools/match.py` (D316)
   shows which function, and `tools/ebss_solve.py --all` (D316) puts a file's variables back in
   order. When a set of edits breaks the gate, split it and run the gate on each half until only
   the edits that keep the bytes remain. The scripts the readability pass used for that halving
   (A316) and for comparing objects with saved signatures (D316) were removed after v1.0.0; they
   are in the `v1.0.0` tag.
3. **Name only with evidence**, and record it in `names.tsv`. A name that would need guessing
   stays a placeholder.
4. **Say "inference"** where a comment or doc goes beyond what the code shows.
5. **Plain English, ASCII only**, and no home paths, user names or serial numbers
   ([CONTRIBUTING.md](../../CONTRIBUTING.md)).
6. **Cite the function and file** for every rule written in the docs.
