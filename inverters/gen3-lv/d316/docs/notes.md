# Notes on the D316 build

What I learned matching the D316 image: the compiler and its flags, the image's layout, the
linker, the order of a file's `.ebss`, and the code shapes that decide whether C comes out as
the image's words. Each point was checked by a probe compile, a link or the gate. How each of the
hard functions was finally matched is in [hard.md](hard.md).

File and function names are today's (map/filenames.tsv has the old file names, map/names.tsv
the old function names). Variables keep their address names (`var_D940`), since the address is
what the point is about. Headers from the matching work (a_ram.h, b_meas.h, c_ctrl.h and the
like) were merged into the subsystem headers in the readability pass (docs/readability.md). Comments in src/ that cite docs/notes-B.md, notes-C.md, hard-B.md or
hard-C.md with a pass or session number refer to logs that are now folded into this page and
hard.md.

## The compiler and its flags

- TI C2000 code generation tools, COFF ABI, flags `-v28 -ml -mt --float_support=fpu32 -O1`. The
  tree is built with 16.9.6.LTS today; the matching started with 15.12.3 and moved through 15.12.7
  (next section).
- The runtime library. The members in the image are word for word those of 15.12.0 to 15.12.3
  (`tools/libfind.py --lib ...`: 19 sections, 1212 words with each). 15.9.0 places only 15
  sections and 15.12.4 only 14 (its `expf` changed); 16.9.6's own rts2800_fpu32.lib places 12.
  So the library is 15.12.3's whatever compiler builds the sources.
- 15.12.0 and 15.12.1 compile an interrupt handler that calls a function without the
  `SETC INTM|DBGM` before `POP RB` that the image's `timer2_isr` has (45 words, not 44). That is
  defect SDSCM00039432, fixed in 15.12.2. `src/rtos/os_tick_isr.c` proves it;
  `tools/match.py src/rtos/os_tick_isr.c --all-cgt` shows each installed version. 15.12.3's other
  fixes are for CLA, MISRA diagnostics, the linker and an unsigned array index case
  (SDSCM00052881). I tried 23 index patterns at -O1 and -O2 and 15.12.2 and 15.12.3 gave the same
  words for all of them.
- The optimisation level. `CRC16` matches only at `-O1` (not -Ooff, -O0, -O2 or -O3).
  `InitFlash` and `timer2_isr` match at -O1 to -O3. `-ms` (optimise for space) makes no
  difference to these three.
- The original build was not compiled with `--gen_func_subsections`: the image holds 56 functions
  that nothing calls or points to (default ISRs left unused, driver functions), which a link with
  per-function sections would have dropped. The tools add `--gen_func_subsections=on` anyway (its
  short form is `-mo`) so that every function is a section of its own. I have seen no change in
  code from it.

## Which compiler built each file

The evidence came in steps, and each step changed the default.

- **15.12.7, not 15.12.3.** 15.12.0 to 15.12.5 reach a struct member more than 255 words into a
  struct through a pointer with `MOVL ACC,XARn; ADD ACC,#k << s; MOVL XAR4,ACC`. 15.12.7 uses
  `MOVL XAR0,#k` and `*+XARn[AR0]`, as the image does in grid_ictl and eps_vctl (blocks of
  0x43B words with a 500-float delay line inside). grid_ictl matched only with 15.12.7, and every
  source proved until then gave the same words with 15.12.3 and 15.12.7. 15.12.6 gets only
  grid_ictl wrong.
- **Some files were built by 16.x.** The four -Ooff helpers that never matched (checksum_xor,
  isqrt_div, limitu, limiti) come out exact with 16.9.0, 16.9.4, 17.3.0 and 17.9.0 at -Ooff, and
  the other fifteen functions of src/app/util/util.c come out the same with every installed
  compiler. 16.x at -Ooff gives the 32-bit values the top frame slots and reloads a pointer
  argument into XAR7 for a store; 15.12 does neither. MemCopy (TI's F2806x_MemCopy.c, in the boot
  loader) is exact with 16.3 to 17.9 at the default -O1; every 15.x keeps reloading ACC in the
  loop. pv_boost_precharge (src/app/state/op_states.c) is exact with 16.3 to 17.3 only, and the 26
  other functions of that file are exact with those versions and with 15.12.7 alike.
- A scan of every proved file with 15.12.7, 16.3.0, 16.9.4 and 17.3.0 at that point: most files
  were exact with all four; ac_ctrl.c failed with every 16.x and 17.x, converter_ctrl.c,
  os_tick_isr.c and sci_isr.c with 16.3, device_init.c and arm_link.c with 16.9.4, and
  grid_support.c, power_limits.c, current_ctrl.c, dc_measure.c and mppt.c with 17.3. So no single
  later compiler seemed to build the whole application, and 15.12.7 stayed the default.
- How strong the case was for each file then. op_states.c: its 26 other functions do not tell the
  compilers apart (with pv_boost_precharge the file is 27 of 27 with 16.3 to 17.3, and 24 of 27 with
  17.9), so that one function was the only evidence for the file. util.c (four functions, frame
  layout and pointer reloads) and MemCopy (the boot loader, likely a separate project anyway)
  had real evidence. There was no other sign of a mixed build: op_states.c's `.ebss` and `.cinit`
  records sit in the same order among the other files' (toolchain.toml cinit_order), and the gate
  links them the same whichever compiler built the file. The function order inside util.c in the
  image is not its source order, but the linker decides that. A likely story at the time: the
  project moved to a newer CCS during development, and only the files that changed afterwards
  were rebuilt.
- **16.9.5 or 16.9.6 builds most of the image.** `match.py --all-cgt` showed that drafts of
  rcmu_fast_check and ring_read that 15.12.7 could not match are EXACT, unchanged, with 16.6.0 and
  later (ring_read from 16.3.0). 16.9.0 and 16.9.4 still reach a struct member more than 255
  words in with `ADD ACC,#k << s` (grid_ictl fails); 16.9.11 and later change about 15 proved
  functions. With every source then in src/:

  | compiler | not EXACT |
  |---|---|
  | 15.12.7 | none |
  | 15.12.6 | grid_ictl |
  | 16.9.5, 16.9.6 | InitPieVectTable (device_init.c), arm_link.c (arm_page0 to arm_page5 grow) |
  | 16.9.7, 16.9.8 | the same and bms_parse_hr, aging_test_update, soft_start |

  16.9.5 and 16.9.6 also give eps_vctl from a draft that 15.12.7 cannot match (the res.r
  pointer in XAR7). 16.9.5 and 16.9.6 gave the same words for everything I tried; I use 16.9.6.
- The seven functions that failed under 16.9.6 had been matched against 15.12.7, and their C
  needed redoing rather than a second compiler. InitPieVectTable matches 16.9.6 when it is TI's
  own F2806x_PieVect.c text (`Source` declared before `Dest`; 15.12.7 needs `Dest` first), and
  then all 21 functions of device_init.c are EXACT with 16.9.6. The arm_page functions push XAR1
  with 16.9.6: `var_CFC0[++i]` gives `MOVB XAR1,#1; ADD AR1,..` where the image has
  `ADDB XAR0,#1`. Written `x = var_CFC0[i++] << 8; x |= var_CFC0[i] & 0xFF;` (`i++;` between, or
  `i += 1;`, give the same), all of arm_link.c is EXACT under 16.9.6.
- grid_support.c settled it for that part of the image: under 16.9.6 the plain drafts of
  task5_arm_link, q_of_u_curve, uvr_storage_response (`var_CD5A - (var_CD5A - var_CD5E) * (var_CD60
  - var_D3CC) / (...)`), grid_import_current_limit and volt_watt_limit (multiply-first,
  initialisers in the declaration, two returns) are EXACT; under 15.12.7 they were 2, 3, 11, 16 and
  24 instructions off after weeks of searching. Every other function of the file is EXACT under
  16.9.6 too.
- util.c, F2806x_MemCopy.c and op_states.c, first given 16.9.4, are EXACT with 16.9.6 as well
  (19, 1 and 27 functions). dc_measure.c (22 of 22) and power_limits.c (12 of 12) are EXACT with
  15.12.7 and 16.9.6 alike.
- **More compilers.** 16.9.9 and 16.9.10 are installed too (TI public download, into the Wine
  prefix beside the others). In a small test 16.9.11, and only it, loads a constant straight into
  the register of a 32-bit local (`MOVB XAR6,#79`) where every other version goes through ACC
  (`MOVB ACC,#79; MOVL XAR6,ACC`) unless ACC is busy in that block. On boot_flash_loader 16.9.9
  gives 32 and 16.9.10 and 16.9.11 134 (other register choices), and all three miss MemCopy by 2,
  so none of them is the boot loader's compiler.
- **A per-file compiler.** tools/d316.py `compile_c` reads a line `// cgt: 16.9.6.LTS` in a source
  (`cgt_of`: a version matched against toolchain.toml's `others`, or a path); an explicit
  `--cgt` still wins. match.py and the gate honour it. While the default was 15.12.7 the 16.x
  files carried the line; toolchain.toml now names 16.9.6 for the whole tree and has the
  current evidence.
- **Per-file options make no difference.** For the hardest drafts I tried --opt_for_speed=0..5,
  -mf, --fp_mode=strict/relaxed, -g, --symdebug:none/dwarf/skeletal, --optimize_with_debug, -O0
  to -O4, -O2 with and without -mf, -ms, -mn, -mi, -ma, -md, -oi0, --auto_inline=0,
  --call_assumptions=0..3, --no_rpt, --no_fast_branch, --disable_dp_load_opt, --fp_reassoc,
  --sat_reassoc, --float_operations_allowed=all, --plain_char=signed,
  --silicon_errata_fpu1_workaround, --gen_func_subsections=off, without -ml or -mt, C99 and
  relaxed or strict ANSI, the tmu, vcu and cla switches, and inlining switches. None removes a
  difference; most change nothing, the rest make it worse. Whole flag sets compiled per draft
  showed every hard case best with the toolchain.toml flags. 15.9.0 is no closer either. When a
  function is close but will not match, try `python3 tools/match.py SRC --all-cgt` before tuning
  the source further.

## Section names with --gen_func_subsections=on (15.12)

| What | Section |
|---|---|
| a function | `.text:_name` |
| a function with `#pragma CODE_SECTION(f, "ramfuncs")` | `ramfuncs:_f` |
| a function's switch tables | `.switch:_name` (one section per function, all its tables) |
| a const object | `.econst:_name` |
| global and static variables | `.ebss` (one per file, no subsections) |
| initialised data records | `.cinit` (one per file; the library uses `.cinit:__lock` and the like) |

Every subsection is marked `.clink` (conditionally linked). A link with no entry point removes
all of them unless the linker is given `--disable_clink` (`--unused_section_elimination` is not
accepted for COFF output).

## Code generation details that matter for matching

- An `asm(" ...")` statement is followed by `SPM #0` before the next call, so `EALLOW`/`EDIS`
  and `EINT`/`ERTM` macros show up as `SPM #0` after them (`main`, `start`).
- Interrupt functions: `ASP; PUSH RB; PUSH AR1H:AR0H; MOVL *SP++,XT ...; MOV32 *SP++,STF; ...;
  SETFLG RNDF32=1,RNDF64=1; SPM #0; CLRC OVM|PAGE0; CLRC AMODE`, and the reverse at the end.
  TI's default handlers end in `ESTOP0; SB 0,UNC` (`asm(" ESTOP0"); for(;;);`).
- Switch tables: `MOV AH,@AL; ADDB AH,#-first; CMPB AH,#count-1; B default,HI; ...;
  MOVL XAR7,#table; ... MOVL XAR7,*XAR7; LB *XAR7`, with 32-bit entries in `.switch:_f`. A
  9-case switch at -O1 was compiled as compares, a 30-case one as a table.
- The library's `boot.obj` is `_c_int00`; it calls `_system_pre_init` (pre_init.obj),
  `_system_post_cinit` (startup.obj, one `LRETR`), `_args_main`, which calls `main`, then
  `exit`. `copy_in` is called only if there is a copy table (`binit` is -1 here).
- `memset(&s, 0, sizeof s)` with a constant size compiles to `RPT #n-1 || MOV *XAR4++,#0`.
- The third 16-bit argument is passed in AR4 (`MOVZ AR1, @AR4`), after AL and AH.
- In unoptimised code (-Ooff) a comparison loads its right operand first: `a >= b` compiles to
  `MOVL ACC,b; CMPL ACC,a` (median3ul).
- 15.12 at -Ooff gives the arguments their frame slots in order first and pads a long local to an
  even offset; 16.x puts the 32-bit values (pointer or long argument, long local) at the top of
  the frame and the 16-bit ones after them, and reloads a pointer argument from the frame into
  XAR7 for each store (`MOVL XAR7,*-SP[2]; MOV *+XAR7[0],AL`).

## How the image is laid out

| Words | What |
|---|---|
| 0x3E8000 | version word 0x013C |
| 0x3E8004 | `app_entry`: `LCR start; LRETR`, where the boot loader jumps (`LB 0x3E8004`) |
| 0x3E8008 to 0x3F1BB6 | .text: application code, the RTOS, TI's default ISRs, the library members, the RTOS context switch in assembly |
| 0x3F1BB7 to 0x3F336E | ramfuncs load image, 0x17B8 words, run at 0x8000 to 0x97B7. `init_system` (0x3ED38A, called first by main) copies it with `memcpy(0x8000, 0x3F1BB7, 0x17B8)` |
| 0x3F336F to 0x3F3B44 | .cinit: 414 records, then two 0 words the linker adds (--rom_model) |
| 0x3F3B46 to 0x3F3F35 | .econst and .switch: PieVectTableInit (128 vectors), the CRC tables, the ARM link page handler table, expf's constants, four switch tables |
| 0x3F3FFC | 0x00AA: the boot loader starts the application only when this word is 0xAA |
| 0x3F5000 to 0x3F52D3 | the boot loader (C): `boot_main` at 0x3F5226, `csm_unlock`, and a 546-word routine copied to RAM 0x0400 and run there |
| 0x3F7F80 to 0x3F7FF5 | CSM reserved words, all 0 |
| 0x3F7FF6 | `codestart`: `LB 0x3F5226` |
| 0x3F7FF8 to 0x3F7FFF | CSM passwords |

The rest is 0xFFFF (15,739 words). The order of code inside .text is not one file after another:
the library members sit between the application's RTOS functions (expf at 0x3F14B6, then
ll_div, then the RTOS `os_wait`, then fs_div ...). The gate does not depend on the order since
it places every section at its address.

### .cinit

Each record is `-count` (a negative count means a 32-bit address follows), the address (2 words),
then `count` words of data. All 414 records here use the 32-bit form. The table ends with a 0
word; the linker writes two 0 words after the last record (`.cinit` came out 2 words longer than
the records in the link). `c_int00` gets the table's start through the linker symbol `cinit`,
which `--rom_model` sets to the start of the output section named `.cinit`. The records come in
.ebss order of each file, so they show where one file's variables end and the next file's
begin. `unsigned int buf[100] = {0};` makes a record of one word (only the initialised element),
as the image has for the BMS buffers at 0xD940 and 0xD9C0.

### ramfuncs

The ramfuncs run at 0x8000 and are compiled for that address, so a ramfunc's calls and jumps
to other ramfuncs are to 0x8xxx. The map's ramfunc rows have the load address in `addr` and the
run address in the notes (`run=0x8000`). `tools/dis.py` shows them at the run address.
`DSP28x_usDelay` (TI's assembly) is the last ramfunc, `InitFlash` the one before.

## The data blobs (PieVectTableInit, function tables, .cinit, TI's assembly)

- **.cinit is linked, not a blob.** toolchain.toml's `[gate] cinit_order` lists the files that have
  initialised data in the original link order; the gate builds the output section .cinit from
  their .cinit sections in that order (library members as `rts2800_fpu32.lib<m>(.cinit)`), and
  the linker adds the two end words. All 414 records come from the link: 408 from sources, 6 from
  the library (exit.obj's three at 0xCF34..0xCF38, _lock.obj's `.cinit:__lock` and
  `.cinit:__unlock`, errno.obj's at 0xD2FE). The gate checks that the linked records are the
  image's, in order. Listing _lock.obj's two subsections one by one put them the wrong way round;
  `(.cinit)` gives the linker's own order, which is the image's.
- errno.obj has no code in the image (sqrtf and expf use `_errno`): the gate links it for its
  record and puts its .ebss where the image's code has `_errno`.
- The order is not the .text order of the same files (grid_support, power_limits, ac_measure,
  dc_measure, bms_link, converter_ctrl, mppt, arm_link, op_states, avg_win, os_main,
  boot_flash_loader, exit.obj, current_ctrl, _lock.obj, iq_consts, sci, errno.obj).
- Owners of the last records: 0xD2EC (16 words, the 4-bit unmap table) is os_main.c's
  `os_unmap_tbl`, next to os_highest; 0xD640 .. 0xD700 are the boot loader's update routine's
  (boot_flash_loader is the only code that uses them), in src/boot/boot_flash_loader.c; 0xDAD0 is
  arm_link.c's.
- fptrs_3F3E46 (the presets by 0xD8F1) is defined in rating_presets.c next to the functions it
  points to (its empty default, preset_none, lies between them in .text but is in app_init.c);
  arm_page_handlers in arm_link.c. Adding the table and the declarations of arm_page6/7 moved
  arm_link.c's .ebss; the .ebss solver put it back in one round.
- **Assembly.** gate.py and match.py take `.asm` files in src/ and assemble them with the same
  cl2000. A section with no symbol the map knows (codestart, csmpasswds, csm_rsvd) is placed at
  the row whose notes say `section=NAME`, in program memory. TI's files: CodeStartBranch.asm
  (`LB _boot_main`, WD_DISABLE 0), F2806x_usDelay.asm (a ramfunc), F2806x_CSMPasswords.asm. The
  RTOS context switch (os_cpu_a.asm) and its critical-section helpers (os_cpu_sr.asm, at the end of
  .text, so a file of their own) are the RTOS port's assembly.
- `version` (0x3E8000) and `app_valid` (0x3F3FFC), one word each, are in src/boot/app_header.c,
  each a `const unsigned int` in a section of its own (`#pragma DATA_SECTION(version, "version")`);
  the link places them by name. Their original file is not known (nothing in the image tells), so
  they have a file of their own, which moves no other file's .ebss.

## The linker (lnk2000 15.12 under Wine, or 18.12.8 native)

- Both give the same image.
- A section at a fixed address outside every MEMORY range is put in a made-up DEFAULT range, and a
  second one then fails with "DEFAULT memory range overlaps". The gate's MEMORY covers the whole
  address space in ranges, with two parking ranges outside the device for what is not compared.
- A library member is linked only for a symbol that something needs. The gate asks for one
  global of each member with `-u` and places its sections with `rts2800_fpu32.lib<boot.obj>(.text)`
  inside the braces (no `-l` there; the library is given once with `-l` and `-i`).
- The gate once defined every symbol a source uses from the map, library routines too
  (`FS$$DIV`, and `c_int00`, which `start` calls). That kept the linker from taking the member
  from the library (the member's words came out 0xFFFF, and boot.obj was left out). The gate now
  drops the map's definition of every symbol that a member it places defines.
- `--rom_model` sets `cinit` to the start of `.cinit` and appends the two 0 words; with no copy
  table and no .pinit it sets `binit` and `pinit` to -1 (the image's `MOVL XAR7,#0x3FFFFF`),
  and `___TI_pprof_out_hndl` to -1. `__stack` is the start of `.stack`: the gate places .stack
  at 0xE390, the image's `MOV SP,#0xE390`.
- The library members' data (exit.obj's `.ebss` at 0xCF34, `_lock.obj`'s `.ebss:__lock` at 0xD13E
  and `.ebss:__unlock` at 0xD2FC, `_errno` at 0xD2FE) are placed where the image's relocated
  words say (`d316.derive_bases`).

## hex2000

With `-romwidth 16` and `-order MS`, the ROMS origin and length are byte addresses: twice the
word address (`origin = 0x7D0000, length = 0x20000` for 0x3E8000 to 0x3F7FFF).

## A file's .ebss layout

- **A file's own variables.** cl2000 (15.12 and 16.9.6) lays out a file's `.ebss` as: one-word
  scalars, then two-word scalars (long, float), then aggregates (structs and arrays), each class
  in a fixed shuffle of the definition order. It looks like an unstable sort by size over the
  file's symbol table. The shuffle does not depend on the names or the initialisers of the
  variables, only on their slots: with n plain `unsigned int` variables it is, by declaration
  index, n=3: 1 0 2; n=4: 2 3 0 1; n=5: 3 4 1 2 0; n=6: 4 5 3 1 2 0; n=8: 5 4 7 6 1 0 3 2 (for
  2^k variables, the index XOR 0b...1010). Types and qualifiers change it.
- The shuffle also depends on everything else the file declares and on how the variables are
  used: the functions, the externs from headers. Adding a function or changing a header can move
  the variables, so after any change to such a file the order has to be solved again.
- **A callee's name can move a file's .ebss.** Renaming the call `sub_3F170C(0, 4)` to
  `os_post(0, 4)` in ac_measure.c (same address, same prototype) moved several of that file's
  .ebss variables, and 43 words differed. So the order depends on more than the sequence of
  types; identifier spellings in the file take part too. Rename only where it is needed (a
  ramfunc's `sub_` name resolves to its load address), and solve the file's .ebss again after any
  rename.
- **One-word structs sort with one-word scalars.** A one-word array or struct goes with the
  one-word scalars. (0xDAD0 is not one: it is arm_link.c's `var_DAD0[10] = {0}`, the last
  aggregate of that file's .ebss, and its record follows the file's others.)
- A section of more than 64 words starts on a page; one of up to 64 words does not cross one.
  Aggregates in a file's .ebss are blocked the same way.
- So a float that sits after a struct in the image is not a scalar of the same file: it is a
  member of a struct, or another file's. In converter_ctrl.c the layout is 0xC000 .. 0xC021
  (16-bit), 0xC022 .. 0xC02A (32-bit), then the structs 0xC02C {count, sum}, 0xC030 (7 words,
  cleared with memset elsewhere), 0xC040 (a PI block) and 0xC04E.
- **Solving it.** A file's own variables can be defined under their map names after all: declare
  them in some order, compile, see where each landed, and permute the declarations so that each
  lands at its address. One or two rounds are enough; the `.cinit` records then come out in the
  image's order by themselves (178 records for the file at 0xCCC0). The definitions sit between
  begin and end markers, and `tools/ebss_solve.py` does the solving today (it also swaps pairs
  when the plain iteration cycles). match.py shows misplaced variables as relocation differences
  and .ebss notes; a file whose variables are named after their addresses (`var_D902`) gets a
  note when two of them want different section addresses.
- The cinit record groups show which variables a file defines. Some of the sets: power_limits.c
  0xD4C0..0xD4FB (records 178..220), ac_measure.c 0xD300..0xD339 (221..257), dc_measure.c
  0xD140..0xD178 (258..291; 0xD176..0xD178 is a 3-word block cleared together, a union so that
  memset or pointer access and DP access both come out right), op_states.c 0xCF08..0xCF33
  (CF08..CF11 words, CF12 as a union of 8 words, CF1A a 26-word struct; 0xCF34..0xCF38 are the
  library's exit.obj), mppt.c 0xD12C..0xD13D, converter_ctrl.c 0xC000..0xC021 and on (each with its
  own .cinit record, in address order), settings.c 0xD801..0xD83F (0xD834 and 0xD838 are unused
  globals there; without them the layout shifts), avg_win.c 0xCF3A..0xCF3E.

## Which object a variable belongs to

How a variable is declared (the file's own, an extern, a struct member, one object or several)
changes the code well beyond the data page. Most of the hard functions turned on this.

- **The data page.** Globals defined in the same file share one `MOVW DP` while they are in the
  same 64-word page; an `extern` global gets its own `MOVW DP` at every access, because the
  compiler cannot know its page. Struct members share one too. So a run of accesses without DP
  reloads in the image means the variables were defined in the function's own file or are one
  struct. Where the image reloads the DP between two addresses of one page with nothing in
  between, they are separate objects (separate externs).
- **Defined here, extern, or struct member.** The three compile differently beyond the DP
  reloads: (1) a file's own scalars get store-to-load forwarding (`a = x; b = x; if (b ...)` does
  not reload b), an extern or a struct member is reloaded; (2) stores to a file's own scalars are
  scheduled in source order, while stores to struct members (extern or defined) and to plain
  externs are moved around (a store of 0 sinks, a `MOVB @x,#imm` store rises, a store to a member
  was moved next to an earlier access of the same page); (3) a load from one struct member is not
  moved above a store to another member of the same struct (the compiler assumes they may
  overlap), while it is moved above a store to another object. The 0xD800..0xD8FF block behaves as
  one struct.
- So when a run of stores in the image keeps source order with one data page shared, the
  variables are the file's own. The compiler keeps a struct's loads and stores in source order but
  reorders accesses to separate variables (`INC @x; MOV AL,@x; MOV @y,#0` for
  `x++; y = 0; if (x >= 5)`), and that is what the image has. My first approach, an `extern
  struct` over each page, gave the wrong code where the variables were really the file's own;
  with them defined in the file the code is the image's, and the cinit records the definitions
  make are the image's too (the gate checks them).
- The 0xC000 page is the control module's own globals (converter_ctrl.c). Written against a struct
  (`var_C000.xC0nn`) or as externs, converter_mode, fast_fault_check, control_isr and tz_trip_isr
  did not match: a store of 0 to a struct member went after later stores (fast_fault_check), and
  the stores to xC007 and xC002 were split by an extern store (tz_trip_isr). As the file's globals
  they all match. The same holds for the limit at 0xCCBE in current_controller: as a plain float of
  the file, with a .cinit record of its own, the image's reloads after each join come out without
  volatile. And settings_defaults matched at once with 0xD801 .. 0xD83F as globals of settings.c,
  after a permutation climb of the struct version stalled at 7 words.
- **Same-file globals are constant-propagated.** The preset tables (preset_5kw ..) store 36000 to
  0xD880 and then compute `0xD880 * 10 / 2300` with FS$$DIV at run time, which a global of the
  file would fold away; 0xD880 .. are a struct or another file's.
- **Data shared between files needs one object.** An extern plain variable gets a MOVW DP at every
  access, so where the image's code reaches a RAM group with one MOVW DP per page from several
  files (the settings at 0xD8C0, the BMS data at 0xD500, the status words at 0xCF12, the 0xD200
  page), the source has one object over it: an `extern struct` with members named by address
  (`vD148`). The converter state at 0xC000 .. 0xC87F was written as one struct per page, with float
  members where the code uses FPU instructions on a word. Some files reach the same words as plain
  externs (`extern Uint16 var_C01E;`) and get the image's MOVW DP per access (app_vars_init). The
  original most likely defined these as globals of the control module; the structs give the same
  words without guessing that module's whole .ebss.
- **An if/else-if chain on a struct member** (`if (s.x == 5 || s.x == 6) .. else if ..`) gets its
  first THEN block moved to the end; on a plain global (extern or own) it stays in source order.
- **One object, not one struct per page.** The scheduler treats stores to members of one object
  as possibly ordered and stores to different objects as free, so where a page struct crosses
  into the next page the split matters:
  - 0xD880 .. 0xD8FF is one object: the preset tables and init_defaults matched only with
    0xD8C2 .. 0xD8ED as members of the same struct as 0xD880 (the 0xD8C0 page a struct inside
    it). With var_D8C0 as a separate object the scheduler put the 1000.0f store early.
  - 0xC400 .. 0xC83A is one object (the block grid_ictl takes, with its PI block and the 500
    float delay line); 0xC440 .. 0xC45F are members of it. grid_ictl_clear went from 28 to 19 and
    grid_ctrl_init from 199 to 104 with that alone.
  - 0xCC40 .. 0xCC82 is one object (the PLL page and the angle wrap counters at 0xCC80): in
    grid_angle_update the layout of the else-if chain matched only so, and the stores to
    0xCC80..0xCC82 keep their order relative to the angle only when they are members of the same
    object.
  The page structs stayed for code already proved; a draft could rename the header's declaration
  (`#define var_C400 var_C400_hdr` around the include) and declare its own object type under the
  map name.
- **One object, two views.** Where the image reaches one word both through a base register
  (`MOVL XAR4,#0xD500; MOV AL,*+XAR4[1]`) and with DP addressing, the source had the memory as a
  union of an array and a struct: array elements with constant indexes come out through a pointer,
  struct members through DP. `var_D500` is `union { volatile w[3]; struct ... s; }`; in
  power_limits.c the fault words are a union of `w[8]` and a struct. soc_floor's `CF15.b8 = ...`
  reads the word through XAR5 and writes it with DP: a bit-field assignment to a struct member.
- **A struct member, not a separate extern.** meas_bus_voltage matches only with 0xD818 read as
  `var_D808.vD818` (the struct grew to 0xD818); `extern unsigned int var_D818` gives a different
  block layout. pv_power_limit_update needs the 0xD880 page as `var_D880`.
- **Two names for one word.** The compiler takes two names as two objects. pv_mppt_step's ramp
  `f02 = step + f02` and lower clamp go through `extern float var_D298` (DP), the compare and the
  upper clamp through `var_D240[1].f02` (base register); so it reads f02 back for the compare after
  the ramp's store, as the image does. The same trick, the other way, matched current_limit_tapers
  (hard.md): a plain extern for a word that is also a struct member lets a load move above a
  store to that struct.
- **A non-volatile view lets the compiler place the reads.** The five bit reads of 0xD500 for
  soc_floor's `CF15.b8` came out in source order through the volatile array view `var_D500.w[]`
  whatever the OR tree or term form (1,680 trees). Through the struct view
  (`var_D500.s.vD500.bit.bN`) the compiler orders them as the image has them. When the order of
  loads is the only difference, try each access path the headers offer (struct member, array
  element, volatile or not) before trying more expression shapes.
- **Owners found this way.** The 0xD400 page belongs to the 206-word block var_D3C0 (the PI blocks
  at 0xD44C and 0xD470 are `var_D3C0.f8C` and `fB0`, passed to pi_step, the ramfunc at
  0x82B4). The ADC channels at 0xC1C0 are twenty structs of three words in one struct (cleared by
  memset in 0x3ED7AD's function): with an array `var_C1C0[i].v` the compiler indexes from a base
  register, with named members (`var_C1C0.c14.v`) it uses the shared MOVW DP as the image does.
  0xC0C0 .. 0xC0EB are 22 floats. ac_measure.c's 0xD322 is an `unsigned long` (a 32-bit tick
  count, ADDL), not a float. 0xD180 .. 0xD197 is one volatile struct (count and sums).
- **One original file.** The state functions and the fault queries are one original file
  (0x3EDD81..0x3EE6A3, op_states.c), which defines the 0xCF00 page. The MPPT functions (mppt_po_pv1
  and mppt_po_pv2, P&O for the two PV inputs, and mppt_pv1 and mppt_pv2, their start-up and limits)
  are one original file with 0xD12C..0xD13D (mppt.c); pv_mppt_step uses 0xD136/0xD137 too and is
  probably the end of that file (it is in mppt_step.c).

## Volatile

- `volatile` is rarer than it first looks: much of what looked like volatile file variables was
  the struct approximation above. The signs that are real:
- **Reloads at every use.** Some variables are reloaded at every use (`var_CCEF`, a loop bound
  written just before) and must be volatile; a volatile scalar also keeps a store from being
  forwarded. A variable read twice in the image with a store between may still be volatile:
  var_C015 is.
- **Repeated reads per bit.** A bit test that reads its word once per bit (`MOV AL,@0x2; LSR AL,1;
  OR AL,@0x2; TBIT AL,#6`) is a volatile word tested with `((x >> 7) & 1) | ((x >> 6) & 1)`: the
  compiler folds the ORs into one shifted test but keeps each read. 0xD502 (struct member) and
  0xD500..0xD502 (array view) are volatile; so is 0xD31C in op_states.c (grid_in_range compares it
  twice), and the BMS current 0xD509 (written by the BMS link). A non-volatile word gets one read.
- **Stores.** A store of 0 to a non-volatile global compiles to `MOVB AL,#0` and `MOV @x,AL`; to a
  volatile one as `MOV @x,#0`, in source order. The image's `bms_lost` uses `MOV @x,#0`
  throughout. But a store of 0 or 1 to a global that is not volatile can still be `MOV @x,#0` or
  `MOVB @x,#1,UNC` when AL is busy, so it is not by itself a sign of volatile. A zero stored twice
  in a row through a base register (`MOV *+XAR4[4],#0; MOV *+XAR4[5],#0`) instead of `MOVB AL,#0`
  and two `MOV *+XAR4[k],AL` means the array is volatile (var_D9C0, the BMS request buffer).
- A store to a volatile global that is not on the page the compiler last loaded gets its own
  `MOVW DP`. A non-volatile extern store can move ahead of a volatile one: `os_init_vars` needs
  os_cur volatile and os_next not, and so does `os_sched`.
- A volatile struct member (`volatile unsigned long events`) is read into a register
  (`MOVL P,*+XAR4[4]`) before it is combined; a plain member is folded into `AND AL,*+XAR4[4]`.
- A struct member stored in both arms of an if/else is if-converted into one store of a selected
  value; a volatile member keeps the two stores (`MOV @x,#0` in the else arm), as in the image's
  rcmu_watch_step.
- A one-bit field assigned from an expression on a plain word compiles to `AND @x,#~bit; LSL AL,n;
  OR @x,AL` (the field cleared and set in memory); on a volatile word it is read into a register
  and stored once. The image's status words at 0xCF12 and 0xD7C0 have the in-memory form, so they
  are plain.
- var_D178 (the flag word at 0xD178) is volatile: with a plain extern the `||` test
  `CF0D == 5 || (D178 & 0x4000)` gets its THEN block moved to the end.
- A flag word compared after a path that set the counter to 0 (`cnt = 0; ... if (cnt > limit)`) is
  not jump-threaded in the image: the limit is volatile there (island_detect).
- **A DMA buffer is volatile.** The sample buffer at 0xDB86, which the DMA fills, is
  `extern volatile Uint16 var_DB86[32]`; the volatile reads moved one LSR behind two MOVXI in
  adc_scale.
- **A volatile destination keeps copies in order.** Stores to the volatile struct at 0xD180 keep
  source order and share a DP only when nothing else comes between (dc_acc_step).
- **A volatile index.** `MOV AL,*; MOV ACC,AL<<1` is a volatile index read into a 16-bit local:
  15.12 then loads the index into AL where the statement is and shifts it later, as the image
  does; a plain member always folds into `MOV ACC,*<<1` (rep_filter_step).
- **A volatile view for a read-back.** Where the image stores a value and reads it back (the
  ramp of pv2_ctrl, `f00 += 10` and then f00 for the fmin), a `volatile` pointer used only for
  that access gives it; the member volatile in the header breaks the twin function pv1_ctrl.
- The PI state blocks the ramfuncs use (pi_ctrl.c) have one volatile member, the output u: the
  image stores u and reads it back for the saturation and the return, and reads every other
  member once.
- **A volatile local.** pv_power_limit_update's `k` (a float whose address goes to limitf) is
  `volatile float k;`, passed as `limitf((float *)&k, ...)`. A plain scalar `k` gets the high stack
  slot and an array `k[1]` the low slot but the other MPYF32 operand order; only the volatile
  scalar gives both the image's slot (-SP[12], below `a[4]`) and `MPYF32 R2H,R2H,R1H`. When a stack
  local's slot and the operand order disagree, try `volatile`.
- **A plain pointer beside a volatile array.** var_DB00 (the ARM frame) is `volatile unsigned int
  [45]`. Plain stores can be scheduled across each other, so writing some of the stores through
  a non-volatile local pointer (`buf = var_DB00; buf[len++] = ...`) lets the index constants of
  the next pair load early, keeps AR0 and AR1 busy, and gives a single store as XAR7 = base + k
  (arm_link_frame, hard.md). When a store pattern looks like register pressure that no local
  change explains, check whether the original wrote some of the stores through a pointer that is
  not volatile (or the reverse).
- **Volatile is not always the answer.** Several functions came close with volatile reads or
  casts and then matched in plain C under 16.9.6 (pv_boost_select, current_limit_tapers; hard.md).
  Treat a volatile cast that improves a score as a hint about where the image reloads, not as the
  source.

## -O1 scheduling and code shapes

### FPU operands and registers

- **FPU operand order.** For `a OP b` the compiler writes `OPF32 Rd, Rb, Rc` with Rb holding b and
  Rc holding a (MPYF32, ADDF32 and MAXF32/MINF32 alike), so the register order in the image gives
  the source order of the operands. SUBF32 keeps the order (Rb - Rc). The rule holds for each
  operator separately: `var_CD74 + (d - c) * (a / b)` and `var_CD74 + (a / b) * (d - c)` differ
  only in the MPYF32 operand order, and a climb over the factor order of every product finds it.
  In power_limits.c the image therefore has `var_D8C0.vD8E0 * (float)var_D8C0.vD8C6` and
  `0.01f * (var_D500.s.vD526 * var_D200.vD212)`.
- **MPYF32 with a memory local.** For `D * k` with k in memory: k an address-taken scalar gives
  `MPYF32 Rk,Rk,RD`; k an array element, a struct member or a register copy gives
  `MPYF32 Rk,RD,Rk`. The source order of the operands makes no difference there.
- Integer operands load in reverse: `(int16)(a * b)` loads b first, and `a + b` of two 16-bit
  struct members loads b first (write `p->n + p->len` for `MOV AL,len; ADD AL,n`). Write the
  operands in the image's reverse load order.
- **MOVIZ without MOVXI.** A float constant loaded with MOVIZ alone has a low half of 0, unless
  the register's low half already holds the right bits: `MOVIZ R1,#0x42E6` after 0.1
  (0x3DCCCCCD) was in R1 is 115.0, not 115.4. Ghidra's decompiler reads MOVIZ as keeping the low
  half, so its constants carry stale low bits (115.4, 4828.2 for 4800.0, 1002 for 1000; and
  0xD8A4 is 0.87 * 0xD882, not * 0xD88E). Take the constants from the disassembly; a small
  register tracker over MOVIZ, MOVXI, ZERO and the stores rebuilds them.
- **Register colouring is global.** One expression can renumber the FPU registers of the whole
  function: in volt_watt_limit, dividing before multiplying by 0.8 gave the image's numbering
  everywhere else under 15.12.7, while the image's own multiply-first form renumbered everything.
  So a draft with many register differences can be one expression away, and a search should score
  whole-function variants rather than stop at the first local fix.
- **Initialisation order decides register sharing.** Locals that all start as 0 can share one
  register when the compiler sees no conflict. In pf_curve_interp the result r kept its own
  register (XAR1) only when `r = 0.0f;` came before the other two zero assignments, with the
  declarations written apart from the initialisers (`float xm, m, r; r = 0.0f; xm = 0.0f; m =
  0.0f;`). In volt_watt_limit writing `top = 0.0f` before `t = 0.0f` (ZERO R1, then t as a copy
  into R2H) fixed the register numbers of the whole function; the order of the declarations
  themselves made no difference (all 24 tried).
- **A local reused for the return value** (needed under 15.12.7, not under 16.9.6). volt_watt_limit
  matched 15.12.7 only when its last lines were `if (...) thr = var_D88A; else thr = var_CD90;
  return thr;` (thr a local used earlier as a threshold), with
  `(var_CD92 - thr) * 0.8f / (top - thr)` and `float step, thr, top, t; top = 0.0f; t = 0.0f;`.
  Writing the two returns directly renumbered the FPU registers of the whole function. Reusing a
  local changes its live range, and the colouring of every other variable follows. Try a
  function's existing locals as the carrier of the return value (and of other temporaries) when
  the registers are permuted.
- **A parameter reused as a local.** In arm_link_frame the length parameter is also the running
  index of the frame (`len = 13; var_DB00[len++] = ...`): with a separate `i`, len went to AR0;
  with len reused, to AR1 as in the image. Look for parameters that die early and could be the
  function's counter or temporary.
- **A separate local, or a variable split in two.** os_task_create's `top = os_stack_free;
  os_stack_free += stk; sp = os_stack_init(task, top);` gives the image's registers after the call;
  reusing `sp` for both moves them. A temporary of its own decides the register of a product:
  meas_pv_power's `c * (b * k)` goes into the register of `b * k` in the image only when written
  `e = b * k; ... e = c * e;` (e assigned twice, the first before the D228 statement); reassigning
  `b` itself does not work, since b lives in a callee-saved register across the limitu calls.
- **Products into locals before a filter.** meas_pv_power's `a = D204 * D21A; d = D206 * D21E;`
  before the two 0.6/0.4 filters lets the second product's loads move above the first filter's
  store, as in the image (a separate local `d`, not `b`, gives the image's registers).
- **Squares of a value loaded once.** `sqrtf(re * re + im * im)` with `re`, `im` locals loads each
  value once and squares it (`MPYF32 R1H,R1H,R1H`); `g * g` on a global loads it twice.
- **One statement per line.** `fb = 0; fa = 0;` on one line comes out of the optimizer as
  `fb = fa = 0` (see `--optimizer_interlist`), and the code generator then copies the register
  (`MOVB AH,#0; MOVZ AR6,AH`); on two lines the two stores stay separate (`MOVB XAR6,#0;
  MOVB AH,#0`). Ones were never merged. The order of the two lines then decides which flag gets
  which register.
- **Known return values.** `ZERO R0` before a return means the returned expression is a known 0 on
  that path: the source returns a local that the path set to 0 (`r = 0.0f; ... return r;`), not
  the global it just stored.
- `uint16 -> float` of a value that is in a register (a local, or a value just tested) is
  `MOVU ACC,AL; MOV32 R0H,ACC; NOP x4; UI32TOF32`; from memory it is `UI16TOF32 R0H,@x`. A local
  copy (`n = var; if (n) f = 1.0f / n;`) gives the first.
- **Constants into an XAR register.** 15.12 to 16.9.10 load a constant for a 32-bit local through
  ACC (`MOVB ACC,#79; MOVL XAR4,ACC`) unless ACC is live in the block; a pointer-typed local goes
  straight into its XAR register only when it is dereferenced. A 16-bit variable that shares the
  register gets `MOVB XAR4,#imm` (boot_flash_loader, hard.md).

### Loads, stores and their order

- **Loads kept before a call.** At -O1 a load of a global into a local is moved down to the
  local's use, past an FS$$DIV call, a volatile store, a label, an `if (1)` and stores to other
  objects. It stays where it is when a branch, a loop, a volatile read, a store into the same
  struct or a store through a pointer comes between. Copies of plain scalars and of array elements
  are not moved. At -O1 a common subexpression is only shared inside a block, so a block boundary
  (`if (D524 > f42) D524 = f42;`) can give two conversions of the same value.
- **Init functions: one object at a time, each in the image's store order.** Write the stores to
  each object as a group, in the order the image stores that object's members, the groups one after
  another; the scheduler interleaves the groups by itself (grid_ctrl_init, 88 to 0 in one step).
  Inits of PI gains match in plain struct order the same way (dcdc_ctrl_init, pv_ctrl_init). Read
  the store order per object off the image.
- **Copies first, then clears.** A pair of struct copies with clears (`D350.x = D380.x; D380.x =
  0;` seven times in grid_period_isr) matched with all the copies first and then all the clears:
  the image loads every source word before the first store. dc_acc_step is the same.
- **A delay line written in its order.** rocof_detect stores the new sample to two variables:
  `d = var_CD38 - x; var_CCF1++; var_CD30 += d; var_CD38 = x; var_CD3A = x;`. The order of these
  statements fixes which store is paired with the SUBF32; a climb over all orders found it.
- **Reload or register copy.** After `if (g < x) ... else x = g;` the compiler reloads g for
  `x = g` when g is a scalar (extern or the file's own); when g is a struct member it copies the
  register (`MOV32 R0H,R1H`), and a local copy of g gives the register copy too. For
  pv_boost_select this looked like the member against the scalar, but what decided it was the
  form of the max (Saturation forms, below).
- `x = x + y` and `x += y` on a global compile to `ADD @x,AL`; the image's `MOV AL,@y; ADD AL,@x;
  ...; MOV @x,AL` (with other stores in between) needs separate file variables.
- `x = x + c; if (x > m) x = m;` compiles to MINF32 with both stores kept and stops the compiler
  sharing loads across the statements; `x = __fmin(x, m)` drops the first store
  (eps_voltage_control, batt_power_setpoint).
- **One store at the join.** `if (d != 0) y = a / d; else y = a;` (grid_power_pf_calc) compiles to
  one store of y after the join with the value in R0H; `y = a; if (d != 0) y = a / d;` keeps two
  stores. The changed block also moved the whole `if (D807 == 0) A else B` layout: the block order
  came from the blocks' contents, not from how the outer test was written.
- **Separate assignments in both branches** (`if (D90A == 1) { CB = 15000; CF1A = 0; } else
  CB = 1500;`): start_eps_pv's first lines.
- **Pointers are folded back.** Local pointers to globals are folded back into direct accesses,
  and so is a cast through an integer (`(unsigned int *)(unsigned long)var_D940`). An absolute
  address the compiler cannot relate to the variable (`rx = (unsigned int *)0xD940`) stays a base
  register: task6_bms_link keeps the reply buffer in XAR2 across the loop that way, and the ARM
  request copy (`src = (unsigned int *)0xD60A`) likewise. A pointer that is folded can still
  change the code: reads through a plain word pointer to a page (`p[2]`) are no longer volatile
  member reads (current_limit_tapers).
- **Register blocks read as words.** sci_rx tests RXRDY with `TBIT *+XAR5[5],#6` and reads SCIRXBUF
  with `AND AL,*+XAR5[7],#0xff`, through the base it loaded for the RXERROR test; with
  `SciaRegs.SCIRXBUF.all & 0xFF` 15.12 reloads the DP for both.
  `((volatile Uint16 *)&SciaRegs)[7] & 0xFF` gives the image's code, and the compiler then keeps
  the base for the RXRDY test too. The same word-array read fixed the boot loader's receive.
- A run of volatile stores to a peripheral struct (the PIE vector table, the GPIO and ePWM
  registers) comes out in source order with DP or pointer addressing chosen by the compiler;
  TI-style source (`EPwm3Regs.TBCTL.bit.CTRMODE = 2;` ...) written in the image's order matches at
  once. The DP or base split itself is not a clue: 15.12 picks either for the same C.

### Control flow and block layout

- **Layout of if/else.** `if (c) A; else B;` usually comes out as `branch-if-c to A; B; ...; A`.
  `if (a == 0 || a > 60) X;` with a constant X is merged into one range test (`ADDB AL,#-1`); the
  image's separate tests come from `if (a == 0) X; else if (a > 60) X;`.
- **Float tests: read the branch sense.** `if (a < b) A; else B;` compiles to `CMPF32 a,b; B A,LT`,
  B in the fall-through and A placed after it; the CMPF32 operand order follows the source. The
  image's fall-through block tells which way the source wrote the test (mppt_po_pv1 needed
  `if (C2 <= CE) {..} else {..}` in one place and `>` in another). Writing each test from the
  image's branch condition fixes many float tests at once; a one-point-at-a-time climb does not
  find them, because inverting one test changes the cross-jumping of the shared tails.
- A ternary `c ? A : B` with the THEN arm a constant is laid out with the branch to B; the sense
  (`t >= 200 ? 200 : fmax`) follows the image's branch condition. A float local computed before a
  ternary (`p = 0.02f * x; if (p < (c ? a : b))`) keeps the image's order of evaluation; written
  inline the ternary comes first.
- **Bit fields.** Flag words read with TBIT can be plain `x & 4` or a bit field `x.bit.b2`; they
  compile alike except that the bit-field test decides which arm of an `if` falls through
  (`if (x.bit.b2 == 1)` puts the then-arm first; `if (x & 4)` puts it last). 0xD7C0..0xD7C2 are
  bit-field words.
- **Threaded jumps.** Where the image jumps from a block that sets a flag straight to the code the
  flag selects, the source has the jump (`goto`): the compiler does not thread it by itself.
  `if (a) b = f(); else b = 0; if (b < 4100) b = 0;` gives the image's double `MOV @x,#0` (jump
  threading into the second if), which a nested if does not (grid_freq_measure). A local copy stops
  threading: soft_start's chain tests `s = var_CF10` (one load), the later `if (var_CF10 == 3 ...)`
  re-reads memory, and the compiler no longer threads the chain's no-match exit past it, as the
  image does not.
- **Layout from goto.** soft_start's case 2 needed `if (D8F0 == 1) goto chk; if (ok) { CF0B = 50;
  goto chk; } CF0B = 1; goto post; chk: ...` (an if-chain over CF10, not a switch); if/else forms
  all came out the same.
- **Missing else arms show in the jump targets.** When two drafts differ only in registers, compare
  the targets of the conditional jumps too. In arm_link_frame the image's `D212 == 0` jump lands
  one instruction past `MOVB AL,#0`, so that path keeps the old x: the source has no inner else
  (`if (mode) { if (var_D212 != 0.0f) x = ratio; } else x = 0;`), which also explains why the image
  keeps x in AL through the compares. Two task6_bms_link branches landed on a shared `MOV @x,#0`
  instead of the instruction after it: the source had `else var_D919 = 0;` after the CRC test and
  `else var_D90C = 0;` after `if (var_D90B == 1) {..}`, and the compiler merged the two equal
  stores. pv_bus_limit leaves its offset unset when CF0D is neither 5 nor 7 and D178 bit 14 is clear
  (`if (CF0D == 7) off = 100; else if (CF0D == 5 || b14) off = 200;` with no else): the image
  branches straight to the add with R0H as it was.
- **Merged else.** `if (a) { x; if (b) {...} else c = 0; } else c = 0;` gives the image's one
  shared `MOVW DP; MOV @c,#0` block (relay_test_check).
- `x = !(a || b ...)` style conditions that the image computes as `MOVB AL,#1 ... MOVB AL,#0;
  XORB AL,#1` came from `if (cond) c = 1; else c = 0;` (arm_page7 case 0x1A).
- `x |= 1 << n` under an `if` at -O1 can be if-converted into `x |= (cond) << n` (a 0/1 in a
  register). `x = x | (1 << n)` is not converted; the RTOS's `os_post` uses that form.
- **Switches.** Case blocks mostly come out in source order (the sources often list the cases
  from highest to lowest), but not always: the compiler lays out some case bodies itself, and the
  source order of the cases still moves a case with branches inside it. A block shared by several
  labels (`case 0x28: case 0x18:`) stays where it is written. The last
  compare of a switch's chain falls into its case block when that block is a plain `if/assign` the
  compiler can move; `var = b < 8 ? b : 7;` kept the image's layout where `if (b >= 8) b = 7;
  var = b;` did not.
- **Counters.** `if (x < N) x++; else {...}` comes out with the else part in the fall-through;
  `if (++x >= N) {...}` gives `INC; MOV AL; CMPB; SB LO` with the reset in the fall-through.
  Post-increment tests: `INC @x; MOV AL,@x; ADDB AL,#-1; CMPB AL,#10; SB HIS` is `x++ >= 10`, and
  `... CMPB AL,#30; SB LOS` is `if (x++ > 30)`; `if (++x > 30)` has no ADDB (mppt_pv1 against
  mppt_pv2). `n = ++x; y = 0; if (n >= k)` gives `INC @x; CMP @x,#k; MOV @y,#0` (the compare
  before the store); `x++; y = 0; if (x >= k)` puts the store first.
- A loop counter initialised before an `if` (`i = 0; if (...) { for (i = 0; ...) }`) is how the
  image's `MOVB XAR0,#0` comes before the test in bms_parse_hr and bms_fc4_reply; for bms_fc4_reply
  `unsigned int i = 0;` gives the same words.
- **Unsigned compares against negative-looking constants.** `CMP AL,#-16384; SB LO` is
  `x < 0xC000U` on an unsigned x.
- **Saturation forms.** `m = y1; if (m < 10000.0f) m = 10000.0f;` compiles to a copy and MAXF32;
  `if (y1 < 10000.0f) m = 10000.0f; else m = y1;` gives the same instructions with other
  registers. Whether a clamp reads its value or its limit first depends on the form:
  `{ float t_ = i; if (t_ > max) i = max; else i = __fmax(t_, min); }` reads i back before umax,
  the ternary `t_ > max ? max : __fmax(t_, min)` reads umax first, and `t_ <= max ? __fmax(t_, min)
  : max` is a third shape (pi_step, pid_inc_step). Explicit ifs instead of `__fmax`
  (`if (x < g + 300.0f) x = g + 300.0f;`) still give MAXF32, but the `x = g` in the other arm is
  then a reload rather than a register copy, and equal blocks are cross-jumped into one
  (pv_boost_select). Try every form whenever a MAXF32 or MINF32 is near a difference.
- **OR chains are normalised.** For soc_floor's `CF15.b8 = b0 | b8 | b3 | b5 | b12` the compiler
  gave the same code for every OR tree, term form, cast, alias pointer, helper and condition form
  through the volatile view: it reassociates the chain and pairs the first two terms. The access
  path, not the expression, decided it (above).
- **Frame release.** In the image every SUBB SP before an LRETR is a branch target; 15.12.7 moves
  the SUBB of an unused frame above the last instruction of a straight block every time (loops,
  gotos, switch, early returns, volatile and pointer stores all tried). 16.6.0 and later do not
  (rcmu_fast_check).

### Two-byte and other integer shapes

- **Two-byte reads.** The ARM link page handlers read a big-endian word as
  `x = buf[i] << 8; x |= buf[++i] & 0xFF; var = x;` (`MOV ACC,*<<8; ADDB XAR0,#1; ...; OR AL,AH`)
  under 15.12.7, and as `x = buf[i++] << 8; x |= buf[i] & 0xFF;` under 16.9.6 (arm_page6/7 have
  `t = buf[i] << 8; i++; t |= buf[i] & 0xFF; var = t;`). `var = buf[i] << 8 | buf[i+1] & 0xFF;`
  in one expression is turned into `MOVB AL.MSB,AH`, which the image never uses. The form with
  `buf[i + 1]` (index into a fresh AR1) appears once per handler where it was written so.
- A 32-bit big-endian read wrote its second byte as `(unsigned long)buf[i] << 16 & 0xFF0000`
  (`MOVB ACC,#255; AND ACC,*; LSL ACC,16`); `(unsigned long)(buf[i] & 0xFF) << 16` gives
  `CLRC SXM; MOV ACC,AL<<16`.
- DELAY_US(A) (TI's macro, CPU_RATE 11.111) appears as `MOVL XAR4,#n; MOVL ACC,XAR4; LCR
  DSP28x_usDelay`; 0x15F8F is DELAY_US(5000L), 0xD2EE DELAY_US(3000L).
- The linker symbols RamfuncsRunStart, LoadStart and LoadSize appear as `MOV AL,#sym;
  MOV AH,#sym>>16`.

### Calls and the other functions in the file

- **What the compiler knows about earlier functions in the file.** Within one source file the
  compiler uses what it learnt from a function defined earlier: parameters the callee never reads
  are not loaded, the DP the callee leaves set is reused after the call, a callee that does not
  touch XAR6 lets the caller keep a value in XAR6 across it, a call to one that never returns (an
  endless state loop) is laid out last with no branch back, and a store the callee makes to a
  struct on the caller's DP page is known (the 0xCF1C group was widened to 24 words so 0xCF32 is a
  member). It does so only for callees above the caller in the file, so a scratch copy must keep
  the functions in address order.
- So file boundaries and function order show. task0_state_machine only matches in a file of its
  own, and soft_start only next to has_batt_fault, so the tasks (state_task.c, meas_task.c) are
  files of their own. task6_bms_link reloads the DP after calling batt_warning_update in the image;
  with batt_warning_update defined above task6 the compiler reused the DP the callee leaves, and
  defining the functions in address order (task6, then bms_lost, then batt_warning_update) gave the
  reload.
- -O1 inlines a small function defined later in the same file into its caller (boot_update into
  boot_copy_run's caller); the image keeps boot_update a separate call, so the two are in different
  files.
- **Parameters passed through.** `float q_of_u_lockin(float x)` passes x straight on to
  q_of_u_curve(x); a callee that returns one of the file's scalars (`return var_X;`) costs no reload
  in the caller.
- **Unprototyped calls.** Early drafts called limitf as `sub_3F1014(...)` with no prototype (floats
  promoted to double, which is the same 32 bits here). It made no difference to the code; the
  sources call `limitf` with its prototype.

### Frames and the stack

- **Unused locals make a frame.** A local that is never used still costs `ADDB SP,#2 ... SUBB
  SP,#2` (batt_power_blocks; arm_page4, with a flag `c = var_D208 < 1600.0f;` evaluated into AH
  first). So does a block-local temporary (the saturation macro's `float t_`), and a float local
  the compiler keeps in a register (pv_iso_check, pv_input_detect).
- **Stack order.** Arrays get the low stack slots and scalars the high ones whatever the
  declaration order; a later-declared array gets a lower slot than an earlier one
  (pv_power_limit_update's `float k[1]` declared after `float a[4]` lands below it; the source has
  a volatile scalar there, above).

## TI's code in the image

XtalOscSel, InitPll (DisableDog written inline), InitPeripheralClocks, InitPieCtrl (with ENPIE = 1
and PIEACK = 0xFFFF at the end), InitPieVectTable, ConfigCpuTimer, AdcOffsetSelfCal and
AdcChanSelect, and AdcConversion in its later form (512 samples, ACQPS 40, the SOC9/SOC1 waits).
The boot loader's update routine has its own inline copies of the set-up (hard.md). MemCopy and
the boot loader point to another compiler (16.3 to 17.9) and probably another project.

## Pitfalls

- `tools/dis.py` has the standard library module's name. Every script in tools/ imports `d316`
  first, which loads the real `dis` with tools/ off the path.
- dis2000 prints a parallel instruction (`MPYF32 ... || MOV32 ...`) on two lines; tools/d316.py
  appends the second half to the instruction text, so diffs show it.
- A Wine prefix has one wineserver. Each tool run takes a job prefix (`build/wine/jobN`, an APFS
  clone of the master prefix, held with an flock), so tools can run side by side. A new server
  is started with no pipes, then `wine cmd /c exit`, as in the A316 project; otherwise a tool's
  output pipe can stay open. Servers exit 60 s after their last client. `gate.py` stops them at
  the end.
- ofd2000's XML has `<section>` inside symbols too: read only the direct children of `ti_coff`.
- Relocation fields (`d316.RELOC_FIELDS`): `R_REL22` is 6 bits in the opcode and the next word;
  `R_PARTLS6` the low 6 bits of the opcode (a direct-page offset); `R_PARTMS16` the DP word of
  `MOVW DP`. Masking whole words around a DP access hides too much, and libfind then cannot
  place `_lock.obj`.
- Constants loaded with `MOVL XARn,#imm` that land in a code range are not always code
  addresses: in the boot loader's RAM routine (run at 0x0400) small counts look like addresses.
  The map takes code addresses from constants only in the application's code.
- The CRC tables hold byte values that can look like code addresses (`0x00FF, 0x003F`).
- ram_view.h (once a_ram.h) was generated from the map early on and not regenerated as the map's
  names changed; it is kept by hand and by tools/rename.py.
- Read the image twice before searching: two near misses (arm_power_check, state0_idle) were misread
  semantics (a statement outside the inner if; a return after the if), not compiler choices.

## How the search was done

The matching work used scripts that were removed after v1.0.0, when every function was matched;
they are in the `v1.0.0` tag. Their reusable forms are in
[Exhume](https://github.com/abedegno/Exhume)'s ti-c2000 profile (`c28batch.py` for batch search,
`c28ebss.py` for the .ebss order, `c28libfp.py` for the library fingerprint). What they were and
what they taught:

- A draft runner that links a draft longer than its map row in a parking area and still compares
  it, so one oversized function does not stop the link of the file; whole-function and aligned
  (difflib) instruction diffs that show only the hunks that differ; a probe that prints the
  compiler's assembly for one function across versions and options; a runner that compiles one
  draft under whole flag sets; page-struct and register-write generators from the map and the
  image (the latter writes TI-style C for a run of peripheral stores); and the .ebss solvers.
- **Climbs.** Variants written as choice-point templates, scored as the words an alignment leaves
  unmatched: all combinations up to a limit, else a coordinate climb, with moves of two slots at
  once after it; and a line-permutation climb (moves of one line per step), which found
  dcdc_ctrl_reset (42 zero stores).
- **Batch search.** Compiling 40 renamed copies of one function in a single cl2000 run and linking
  each copy at the function's address (the native linker is fast) scores about 10 variants a
  second, so a few thousand random combinations of choice points take minutes. volt_watt_limit was
  found this way: 6000 random variants of statement orders stopped at 36, and adding the return
  forms as one more choice point found the match in the next climb.
- Batch scoring has limits. The default scorer ignores DP offsets and data addresses, since the
  copies move the file's .ebss, so it cannot tell C018 from C01E or D1C2 from D1C4, and a variant
  that swaps two such blocks scores as a match; a strict mode keeps data addresses (tracking the
  DP) except the file's own .ebss. The same function text can come out with other registers as a
  renamed copy than under its own name (4 against 8 for one text of pv_boost_select). A batch link
  sometimes fails with "program will not fit" for variants that are not larger. The batch finds a
  function by its first `void NAME(` text, so a forward declaration must not repeat the parameter
  name. And in one exhaustive run a variant that match.py found EXACT was not ranked best. Check
  promising variants under the real name with match.py.
- **What moved the register and scheduling cases** was never a compiler option: splitting one
  variable in two, a volatile member or view where the image reads a value back or splits a load
  from its use, which object a member belongs to, the access path of a read, and in the end the
  compiler version. Local temporaries the compiler can propagate change nothing, and plain-C
  pointers to globals are folded back.

## Earlier research, checked against dis2000

The Ghidra pipeline's starts are not always the function starts: `FUN_003F122D` (SCI-B receive
interrupt) starts at 0x3F122A with `ASP`. `FUN_003F1BB7` and the other ramfuncs are named by
their load address there; this map does the same and adds the run address.
