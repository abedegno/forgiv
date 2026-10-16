# Readability: types and headers

The scripts this page names were removed after v1.0.0, when the work was finished; they are in the
`v1.0.0` tag, and their reusable forms are in [Exhume](https://github.com/abedegno/Exhume).

The matched sources compiled to the right bytes but read like what they were: Ghidra and o0c
drafts. Every file declared what it used for itself, most globals were byte arrays reached through
casts and offsets, and each function had its own typedefs and `DAT_` macros. This pass changed
that in three steps. The gate (`tools/gate.py`) rebuilt the image identical after each one, and
every edit that changed a byte was taken back. The scripts were in `tools/rd/`.

## How I tested each edit

Most edits here are safe by construction, but some are not: armcc at -O0 compiles a plain use of
a variable differently from the same address reached through a cast (docs/matching.md, round 4).
So every edit that could change code went through `tools/rd/editbisect.py`. It writes all
candidate edits, runs `gate.py --fast --verify-only`, and for a file that fails it reads which
functions differ (`tools/rd/fdiff.py` compares the linked bytes in `build/gate/verify` with the
image) and halves the edits of those functions until only edits that keep the bytes are left.
Edits in functions that still match are kept at once, so one round settles most of a file.

## Step 1: shared headers

`tools/rd/headers.py` gives every global and every function one declaration, in a header per
subsystem under `src/include` (battery, board, clib, dsp_link, eeprom, energy, faults, ff,
measure, meter, modbus, rtc, schedule, settings, stdperiph, sys, tasks, uart, usb). A function's
declaration is its definition's; a library or assembly function's is the commonest prototype
the callers used; a global's is the commonest typed declaration. Where a file had declared a
global as a byte array, its uses became `((uchar *)&X)`: the same address, still through a cast,
so the code does not change. Each file lost its own declarations and includes the headers it
uses.

Before that I folded the `v_ADDR` aliases into `g_ADDR` (57 names; no file used both).

Thirteen declarations in eleven files must stay as the file had them, because the callers
compiled against them: a prototype with narrower types (`ushort ld_word(void *)` in
f080311C0.c, `int fault_flags` in f08024C64.c (now monitor.c), several USB library prototypes with
`void *` or `uchar` returns). The ld_word one went in step 4: FatFs's `ld_word` returns WORD, and
declared `ushort` for the whole of ff.c every part compiles to the same bytes. The header wraps such a name in `#ifndef OWN_name`, and the file defines
`OWN_name` before its includes with a `match:` comment saying which declaration it keeps.
`tools/rd/hdrbisect.py` found them.

Extern lines in the sources went from 1,120 to 4 at the end of this step (5 now), file-scope prototypes from 900 to 16.

## Step 2: structs, fields and names

- The settings blocks are structs now, laid out from every access the code makes
  (`tools/rd/survey.py`) and named from the holding register that `modbus_fc06_write_reg`
  stores at each offset (`build/types/hr_map.tsv`): `settings_t` (EEPROM block 1, 0xD3 bytes),
  `settings6_t` (block 6, the time slots), `cal_gains_t` (block 2), `energy_t` (block 4). The
  names of HR0 to HR116 come from the public GivEnergy register list where the type and the use
  agree; the rest are named by register number (`hr305`). Other structs: `meter_t` (one meter,
  named from `meter_parse_fc04`), `bms_t` (one battery module, named from the input registers
  `modbus_fill_battery_regs` serves), `rtc_cal_t`. `hr_regs` and `ir_regs` are `ushort[60]`.
  The task table uses `OS_TCB` everywhere; the drafts' `struct task` is gone.
- All structs are `__packed` where the block is: the code reads halfwords at odd offsets.
- The slots in block 6 and the gains in block 2 are flat fields (`slot_b3_start`, `gain4`), not
  arrays of records. I tried records first: a store to a member of an array of structs compiles
  value first (docs/matching.md), and in the Modbus write handlers that changed the bytes.
- 138 globals declared as byte arrays but only ever read as one scalar became that scalar
  (`tools/rd/scalars.py`).
- `tools/rd/fields.py`, `views.py` and `simplify.py` then replaced casts at constant offsets with
  field accesses and plain uses, and the files' own struct views (`PK`, `BMS`, `PACKS`) with the
  global's fields. Constant-offset cast sites went from 2,913 to 463.
- 2,348 per-function typedefs (`typedef ushort T802CE2E_v4;`) were inlined, and the drafts'
  object macros (`#define DAT_20001b29 ...`) were replaced by their bodies: `#define` lines in the
  sources went from 2,065 to 73. Both are token for token what the compiler saw before.
- 127 of the 181 `g_ADDR` globals have names (`map/names.tsv`): 27 are the names ST's, CMSIS's,
  FatFs's and uC/OS-II's own code uses (the sources already called them that through macros),
  the rest describe what the code does with them. DSP page fields whose meaning I could not tell
  are named by their place in the frame (`dsp_p4_w03`: page 4, 16 bits at byte 3).

What stayed (step 5 tested each of these again; the first two no longer hold):

- In `modbus_fc06_write_reg` and `modbus_fc16_write_regs` most stores to `cfg_block6` and
  `cal_gains` are still casts of the byte address. Written as fields, armcc orders them
  differently. The file says so in a `match:` comment.
- Low-byte reads of a 16-bit field are written `(*(uchar *)&cfg_settings.inverter_module_l)`,
  still a cast, since a field read would load the halfword.
- A few views stay where their field types disagree in sign with the struct (`short` battery
  temperatures in `bms_poll`, `ushort` in the Modbus handler): one type would change the other's
  loads. `TASKS`, `PK` and `PACKS` remain in those places.
- 54 `g_ADDR` globals are left: counters in `sys_tick_1s`, `task_schedule` and `task_monitor`
  whose purpose I could not tell from their few uses, bytes only zeroed in
  `sys_runtime_vars_init`, and a few with no use in the sources.

## Step 3: named constants

`tools/rd/consts.py` defines constants next to the declarations they belong with and puts them
in place of literals whose meaning the code shows: peripheral base addresses where they are the
first argument of a StdPeriph call or the address under a pointer cast (`GPIOB_BASE`,
`USART1_BASE`, `RCC_BASE`), the system registers written directly, the DSP frame codes
(`DSP_FRAME_TX` 0x43, `DSP_FRAME_RX` 0x44), the two link fault bits, the Modbus function codes in
`modbus_slave_process`, and the holding register numbers with a public name in the case labels of
the write handlers and in `modbus_hr_update`. The peripheral addresses are plain int literals,
not ST's `((uint32_t)...)` expressions, because the callers compiled against int parameters and
an unsigned constant could change a comparison. 90 constants, 406 uses.

## Step 4: file names and directories

The 98 files of `src/auto` were named after their first address and cut wherever no literal
load crosses a 4-byte boundary, so one original file was often several parts. Now there are 46,
under their original names where the evidence gives them (`map/filenames.tsv`: old files, new
file, kind of name, evidence):

    src/app/{,drivers,comm,control,usb}   the GivEnergy application and ST's USB example files
    src/cmsis                             system_stm32f10x.c
    src/rtos                              the scheduler (os_cpu_a.s was already here)
    src/lib/fatfs                         FatFs R0.12c ff.c
    src/lib/usb_otg, src/lib/usb_host/{core,msc,cdc}   ST's USB OTG host library
    src/clib                              two Keil library members also built from C

Of the 46 names, 21 are original (ST, FatFs and Keil files), 14 likely and 11 descriptive.

First the tools: `gate.py` and `objplace.py` already found sources by walking `src/` and placed
them by their at: lines and the map, so nothing there named `src/auto`. `tools/srcfiles.py` now
does the same lookups (all sources, the decompiled C, by stem, by an at: address) for the other
tools (`tools/rd/*`, `mapfix.py`, `drafts.py`, `automerge.py`), which had globbed `src/auto/*.c`
or tested the map's file column for `auto/`. The map's file column holds paths from the repo
root (`src/app/...`), as the StdPeriph rows already did.

Then the joins, one original file at a time, each with the full gate. A join is the parts' at:
lines, the union of their includes and the bodies in order. Two things stop a join:

- **A literal pool.** armcc puts one pool at the end of a file's `.text`. A part whose `.text`
  ends in a pool ends its object, so it cannot join the part after it (main.c: `sys_delay_loops`
  follows main's pool).
- **A callee defined earlier in the file.** armcc then saves only the registers it knows the
  callee needs. In schedule.c and ff.c the joined file saves fewer registers than the image does
  (crc32_table_init after bit_reflect; ld_clust, st_clust and get_fileinfo after ld_word and
  st_word), so those callers were compiled without the callee in the same file.

To find the longest runs that join, I compiled each candidate join and compared every function's
instructions and the `.text` size with the parts' before running the gate. Where a join fails,
the parts stay as `<name>_pN.c`, N the first old part the file holds: main_p1 (parts 1-3) and
main_p4, schedule_p1 (parts 1-2) and schedule_p3, ff_p01 (parts 1-8) and ff_p09 (parts 9-21).
Every other proposed join compiled to the same bytes.

Two files held the end of one ST file and the start of the next, at a 2-mod-4 address the
automatic cut could not use: f08032550.c (usb_core.c, then usb_hcd.c from 0x0803316E) and
f08034A48.c (usbh_hcs.c, then usbh_ioreq.c from 0x08034A82). Each was split there and each half
joined with its neighbours. The map's rows for HCD_Init, HCD_GetCurrentSpeed and HCD_ResetPort,
which still named `auto/f080330B4.c`, now name usb_hcd.c, and the rows for Keil library members
that named `auto/` files that do not exist are empty, like the other library rows.

The moves were plain `mv` with the include depth fixed (`../../include/` and so on). The stems
stay unique across `src/`, which the gate checks.

## Step 5: casts

After step 4, src/app still had 2,680 pointer casts (every `(T *)` outside comments and strings,
counted by line of code, not by site). This step took them to 106. The scripts are
`tools/rd/casts.py` with `cexpr.py` (a small C expression and statement parser) and
`ctypes_a316.py` (the layouts of the structs and globals in `src/include`, with every member's
offset). Every change went through the gate's fast verify with the per-function bisect of
`tools/rd/editbisect.py`, and each stage ended with the full gate.

The passes, in order:

- **Fields.** `casts.py apply` evaluates each dereference, index and member access whose pointer
  goes through a cast: through `&`, `&*`, casts and constant offsets, or one variable index, to
  an offset in a global, a local or a pointer variable. Where the object has a member of the same
  size there, the access becomes the member (`*(ushort *)(((uchar *)&cfg_block6) + 0x12)` became
  `cfg_block6.hr242`). A member of the other sign is tried too; the gate decides. 1,160 sites on
  the first run.
- **Types.** 32 globals declared as byte arrays got their real type in the header:
  `meter_enable_mask` (ushort), `p_inverter_dsp`, `drm_bits` and other scalars, `aux_wave_buf`
  (`short[192]`), `crc32_table` (`uint[256]`), the FatFs objects `fil_*` (`FF_FIL`), and
  `dev11_data`, which is now `dev11_t` in meter.h (it replaces two views in meter.c). New
  members: `cal_gains` +0x24-0x33 four gain and offset pairs (`gain_24` ... `offset_32`) and
  `offset_36`; `cfg_hr360_table` is `ushort[2][48]`, since HR360-407 are written through it at
  [1] (the symbol `sched_table_b`). Parameters and locals that held an address in an `int`
  (`frame`, `req`, `buf`, `data`, `block_data`, `fs_info` ...) are `uchar *` or `FATFS *` now.
- **Parts of a member.** A low byte or a low halfword of a wider member is a value cast where the
  gate allows it (`(uchar)cfg_settings.inverter_module_l`, about 200 sites), and otherwise a cast at
  the member's own address (`*(uchar *)&cfg_settings.v_pv_input_start`) instead of the block
  address plus an offset.
- **Addresses.** `&(*(uchar *)&cfg_settings.inverter_module_h)` is `(uchar *)&cfg_settings`,
  `(int)(((uchar *)meter_data) + 0x2e * 2)` is `(int)&meter_data[2]`, and the GPIO and timer
  arguments lost their `&*` (`GPIO_Init((uint *)GPIOB_BASE, ...)`).
- **Views.** The file-local macros and structs that viewed a global at another type were replaced
  by the global where the gate allowed: `CFG16`/`CFG8`, `U16`/`U8`/`I32`/`U32`, the grid table
  views `T1`-`T4`, `TBL`, the task and UART views (`task_tbl[TASK_METER].period`). Unused
  view structs and the second `PK` went.
- **Tidying.** Parentheses around primaries, unary expressions and whole operands, `(&x[0])[i]`,
  casts to a variable's own type, 196 `#undef` lines of names that were never macros, 54 counter
  `while` loops as `for` loops, task priority constants (`TASK_MEASURE` ... `TASK_USB`) and
  `SYSTICK_CTRL_ADDR`, and names for three leftovers: the slot flags `f1`-`f20` of
  `sched_slot_soc_targets` (`charge_hit1`... `discharge_hit10`) and the drafts' `v5` and `v7`,
  still named in two comments (now `attempt` and `reply_idx`).

Two of step 2's findings no longer hold. Every store to `cfg_block6` and `cal_gains` in the
write handlers is a field now and compiles to the same bytes; the reordering came from the code
around them, which has changed since. And most low-byte reads of a 16-bit field can be value
casts: armcc loads only the byte.

The bisect had a fault, fixed here: when a trial changed a function's size, the verify could not
link the file, `fdiff.py` read stale bytes, and good edits were rejected or bad ones kept (the
final check caught those). `editbisect.compile_failed` now sees that, and the bisect then tries
one function at a time. Running every pass again afterwards kept edits it had rejected, among them
the frame reads in meter.c.

What stayed, by kind (each has a `match:` note where a reader would want to change it):

- Memory-mapped registers and fixed addresses: SysTick CTRL, SCB AIRCR, the NVIC view, the USART
  status reads, the bootloader jump through 0x2C, the flash word at 0x08013FFC, the RAM peek
  (`peek_addr + 0x20000000`), and `GPIO_Init`'s `uint *` argument (about 30).
- Byte pointers to whole blocks for the EEPROM and clear routines (`(uchar *)&cfg_settings`).
- Sign: views whose fields differ in sign from the struct, so that one type would change the
  other's loads: `PACKS` in `bms_poll`, `PK` in meter.c (the float converts with
  `__aeabi_f2uiz`) and in `modbus_fill_slave30_regs`.
- Loads wider or narrower than the member: `(ushort)(*(int *)&cal_gains.offset0)` loads a word
  and keeps its low half (12 in `modbus_fc03_read_holding`, 3 for `dev11_data`); halfword loads of
  the low half of an int (`energy_totals.batt_charge_total`, `bms_modules[n].v_out`, `drm_bits`);
  the signed low byte of `OSTCBCur->FlagsRdy`.
- Another symbol's address: `AAE` and `PX` in the Modbus write handlers (the serial number and the
  meters' day-start counters reached from `cfg_settings`), `(sched_table_b - 48)[i]` for
  `cfg_hr360_table[0][i]`; as the real symbol the literal pool differs.
- Address arithmetic: the status bytes in `modbus_fill_battery_regs` (through `dev_t`), the three
  volatile bit-field stores in `cfg_load_defaults`.
- ST USB library objects declared as bytes for the library's sources (`CDC_SetLineCode`,
  `UserCb`, `USB_Host`), and the `COMM` mailbox view of `dsp_bms_raw` and `LOG` for `fault_log`,
  whose other users treat them as bytes.

`build/casts-progress.txt` has the count per file after each stage.

## Numbers

| | before | after |
|---|---|---|
| extern lines in sources | 1,120 | 5 |
| file-scope prototypes in sources | 900 | 16 |
| per-function typedefs | 2,351 | 6 (pointer typedefs) |
| `#define` lines in sources | 2,065 | 73 |
| constant-offset cast sites | 2,913 | 463 |
| `g_ADDR` globals | 181 | 54 |
| structs in headers for RAM blocks | 2 (partial) | 7 |
| named constants | 0 | 90 (99 after step 5) |
| pointer casts in src/app (step 5) | 2,680 | 106 |
| `for` loops in src/app (step 5) | 41 | 95 |
