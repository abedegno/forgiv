# Readability pass, step 2: headers, names and constants

The scripts this page names were removed after v1.0.0, when the work was finished; they are in the
`v1.0.0` tag, and their reusable forms are in [Exhume](https://github.com/abedegno/Exhume).

File names: this is a log of its time, so it keeps the flat file names src/ had then. Step 3
of the readability pass renamed the sources and moved them into subdirectories;
map/filenames.tsv gives each old name its new path.

The tree was byte exact after step 1 (the names), but every file still declared what it used for
itself, under headers from the three agents (a_*.h, b_*.h, c_*.h) and B's copies of TI's register
headers. In this step I gave every name one declaration in a header per subsystem, named the
remaining var_XXXX words, and added named constants. The gate (`python3 tools/gate.py`) gave an
identical image after each batch. The log is build/readability/progress.txt.

## How I tested each edit

The gate takes about 105 s, so I checked each edit with a faster test first.
`tools/rd/objsig.py` compiles every source in parallel (eight Wine jobs, about 40 s for all 59) and
saves a signature of each object: the words and relocations of every section and the symbol table.
`objsig.py check TAG` compares the tree with a saved signature. When every object is the same as
before, the linked image is the same, so I ran the full gate only at the end of each batch.

The trap of this compiler is the .ebss order. CGT 16.9.6 lays out a file's variables in an order
that depends on every symbol the file declares and on their spellings, so a header change moves
variables even when no code changes. When objsig showed only .ebss offsets and .cinit records
moving, `python3 tools/ebss_solve.py --all` put the definitions back in order (one round each).

## Batch 1: TI's register headers

Each peripheral now has one TI-style header: F2806x_Adc.h, F2806x_ECap.h (new), F2806x_EPwm.h,
F2806x_Gpio.h (now with GpioCtrlRegs), F2806x_Sci.h, F2806x_SysCtrl.h (with FlashRegs, as TI has
it), F2806x_PieCtrl.h, F2806x_PieVect.h, F2806x_Dma.h, F2806x_CpuTimers.h (new) and
F2806x_Device.h. The boot ROM's flash API is in Flash2806x_API_Library.h. I deleted b_adc.h,
b_epwm.h, b_ecap.h, b_gpio.h, b_sysctrl.h, f2806x_flash.h, a_periph.h and a_cpu.h, and the local
copies in ctrl.c (ECAP_REGS), boot_ram.c (GPIO_DATA_REGS) and init_b.c (PIE_VECT_TABLE).

Bitfield unions against plain words made no difference anywhere. Registers that the code used as
plain words and that TI declares as unions (SysCtrlRegs.LOSPCP) are written `.all` now. a_armlink.c
read SCIHBAUD through its own symbol `SciaRegs_SCIHBAUD`; it now reads `SciaRegs.SCIHBAUD`, which
moved its .ebss and needed one solver round. No copy had to stay. The four held type renames in
build/names/hold.tsv (B_AQCTLB_BITS, B_AQCTLB_REG, B_GPA32_BITS, B_GPB32_BITS) went with the copies,
so hold.tsv is empty.

## Batch 2: one declaration per name

The headers per subsystem are util.h, os.h, sci.h, settings.h, faults.h, bms.h, arm_link.h, meas.h,
control.h, power_limits.h, grid.h (a_ctl.c: the grid code's protections and responses), state.h,
rating.h, mppt.h, rcmu.h, init.h, boot.h and F2806x_GlobalPrototypes.h (TI's support functions).
Each header has hand-written types and constants at the top and a generated part at the end,
between `/* ---- declarations ... ---- */` and `/* ---- end of declarations ---- */`.

First I merged the structs that two agents had defined for the same memory:

- bits16_u, B's fault_word and A's a_bits are one union in util.h (`.all`, `.bit`, `.byte`).
- dsp_settings_t (0xD8C0) is one struct in settings.h, with B's names and C's words past 0xD8F1.
- The 0xD500 BMS page is one bms_data_t in bms.h. C's code writes `.all` on the two flag words now.
- B's flat PI fields in ac_meas_t (eps_pi, grid_pi, pvp_pi, pvi_pi) are PI_BLK members now, and
  B's files call pi_step with a PI_BLK pointer.
- B's 0xD240 phase view is C's pv_phase_t (mppt.h). state_b.c writes `cmp` and `cmp_pending`.
- freq_resp_t (0xCE00) is one struct in power_limits.h with the names of a_ram.h's macros, and
  a_ctl.c uses `freq_resp.uf_active` and so on instead of the 22 macros.
- relay_cmd_t, batt_power_rating_t, ac_acc_t, rcmu_selftest_t and rcmu_watch_t (the function's
  view and the caller's view of the same block), island_det_t, dc_acc_t and the vctl.c, pll.c and
  detect.c block types moved into their headers.

Then `tools/rd/gen.py` did the declarations. It preprocesses every source with clang, parses it
with pycparser, and finds every external declaration and definition. For each name it takes the
definition's form, or, for a name no source defines (the gate places it at its map address), the
form most files see. It writes that declaration into the header of the defining file's subsystem
(tools/rd/genplan.py has the map), deletes the copies from the sources and the old headers, and adds
the includes each file needs. Where a file had compiled against another form, the file keeps its
own declaration, defines `OWN_name` before its includes, and the header wraps the name in
`#ifndef OWN_name`. `tools/rd/trydrop.py` then tried each own form against the header's form and
dropped the 27 that compiled to the same object. `tools/rd/drain.py` and `movetype.py` moved the
old headers' types into the new headers, and I deleted a_funcs.h, a_types.h, b_common.h, b_ctl.h,
b_meas.h, b_rating.h, c_bms.h, c_cal.h, c_ctrl.h, c_os.h, c_params.h, c_sci.h and c_status.h.

a_ram.h is now ram_view.h. Range A's two files (a_ctl.c, a_armlink.c) were matched against it, and
it reaches RAM through one extern struct per 64-word page with a macro per word. That gives the
image's one MOVW DP per page, so it stays as a view, included last. Its plain declarations and its
structured types moved into the subsystem headers like everything else.

### What stayed in the files, and why

Each of these has a `match:` comment at its `#define OWN_`. In every case trydrop found that the
file does not compile with the header's form or that the object changes.

| Name | Files | Why |
|---|---|---|
| PI_BLK | ctrl_pi.c, pll.c, pid_inc.c, pid_pos.c | each function sees other members as volatile (u; u; umax and u; i and u). The file defines its PI_BLK before including control.h |
| fault_words | bms_link.c, cal_fault.c, ctrl.c, offset_cal.c, settings.c, control_b.c | state_b.c defines union bits16_u[8]; C's files use the named struct fault_words_t, control_b.c a union of both |
| eps_cfg_bits | arm_page_b.c, rating_b.c, state_b.c | one bit word here; the header has C's three-word struct |
| bms | control_b.c, state_b.c | union bms_page_u (named words and an array); the header has C's volatile struct |
| state_flags | a_ctl.c, app_init.c | used as a plain word with masks |
| batt_current_cmd_i, chg_current_bound_i, dis_current_bound_i | control_b.c | signed here; the header's Uint16 changes batt_power_setpoint and current_limit_tapers |
| meas_fast | init_b.c | 44 words by index; the struct changes InitAdc |
| grid_ictl_blk, mppt_flags, power_limits, eps_cycle | ctrl.c | its own object layout, a volatile word, a volatile word, and eps_vctl_tail with the cycle words |
| dc_acc | ch_avg.c | the task copy at 0xD180 as one volatile block |
| pv_phase | mppt_b.c | the first word of 0xD240 as a float |
| rating_params | presets.c | its own view of 0xD880 |
| arm_fault_word | rating_b.c | one bit word; a_armlink.c defines it unsigned long |

Some views of the same memory still have different struct tags, because their files need other
field types or volatile members, e.g., c_ctrl.h's page views (now in control.h: grid_ictl_view_t,
eps_vctl_view_t, grid_pll_view_t, q_loop_view_t) and the function types grid_ictl_t, eps_vctl_t,
PLL and q_loop_t. The calls in ctrl.c, ctrl_glue.c and a_ctl.c cast the view pointer to the
function's type, which costs nothing. eps_vctl_tail_t keeps an `#ifdef C_CTRL_EPS_TAIL_WITH_CYCLE`
for ctrl.c, which sees three more words as part of that object.

## Batch 3: the var_XXXX names

All 509 were in a_ram.h (one in a comment, three plain declarations), and none was used in range A's
code. So I named each word from the other files that use it. `tools/rd/fieldmap.py` lays out every
object with a map address and lists the member at each word, and `tools/climb/b_refs.py` lists the
functions that read or write it. `tools/rd/varnames.py` takes the member name another file gives
the word (with the object's name in front when the bare name is already an identifier, e.g.,
`meas_fast_batt_v`, `pv_bus_pi_kp`), and logs the users in the note. rename.py applied the table
(build/readability/var_names.tsv) and logged it to map/names.tsv; map/data.tsv has the new names.

- 474 words have the member name another view gives them.
- 18 have an honest descriptive name, `<subsystem>_word_<addr>` (e.g., `ctrl_word_c04c`), where
  no file names the word; the subsystem is that of the functions that use it.
- 14 macros aliased words that already have names (os_cur, sci_rx_buf, the library's _lock, _unlock
  and errno), so I deleted them.
- var_CF34, var_CF36 and var_CF38 are the runtime library's exit.obj words. Their declarations were
  unused, so gen.py dropped them; the map calls them rts_exit_data_cf34 and so on.

## Batch 4: named constants

Each constant keeps the spelling of the literal it replaces, so the type stays the same.

- os.h: the task ids (TASK_STATE ... TASK_BMS_LINK) and the events, as bit numbers for os_post and
  masks for os_wait (EV0_STOP 4 and EV0_STOP_M 0x10, EV1_GRID_PERIOD, EV5_FRAME and so on).
- state.h: the run states OP_IDLE 0, OP_START 1, OP_FAULT 3, OP_UPDATE 4, OP_GRID_PV 5,
  OP_GRID_BATT 6, OP_EPS_PV 7, OP_EPS_BATT 8.
- faults.h: masks for the fault and status bits the notes describe (FB_BMS_COMMS 0x0200 is CF15
  bit 9, BB_FULL, BB_FORCE_CHARGE and BB_NO_DISCHARGE are CF19 bits 8, 9 and 11). The code reads
  most of these bits through `.bit.bN`, so only the mask uses changed. `fault_stop1 &= 0xFBFF` keeps
  its spelling with a comment, because `~FS1_BAD_PRESET` is another expression.
- bms.h: the Modbus slave, function codes, the registers the DSP writes and the poll ranges.
- arm_link.h: ARM_FRAME_FROM_ARM 0x43 and ARM_FRAME_TO_ARM 0x44.
- boot.h: the update commands 'S', 'D', 'W', 'F', 'E' and the replies 'O' and 'C'.

I named only bits and values whose meaning the code that sets them shows. Bits I could not explain
stay literals.

## Numbers

Counted by tools/rd/declcount.py.

| | Before (ea3729e) | Batch 1 | Batch 2 | Batch 3 | Batch 4 |
|---|---|---|---|---|---|
| extern lines in sources | 308 | 297 | 23 | 23 | 23 |
| file-scope prototypes in sources | 20 | 20 | 0 | 0 | 0 |
| named struct and union definitions | 328 | 304 | 298 | 298 | 298 |
| struct tags defined more than once | 62 | 7 | 0 | 0 | 0 |
| anonymous struct bodies | 94 | 40 | 16 | 16 | 16 |
| var_XXXX identifiers in src | 509 | 509 | 509 | 0 | 0 |
| numeric constants in headers | 15 | 16 | 16 | 16 | 107 |

The 23 extern lines left are the own forms in the table above. The 107 constants have 346 uses.

## The tools

The scripts were in tools/rd/ (now in the `v1.0.0` tag). gen.py, decls.py, fieldmap.py and the
struct movers needed pycparser. gen.py rewrote the generated part of every header and was safe to
run again; `tools/ebss_solve.py --all` and the gate ran after it.

# Readability pass, step 3: file names and directories

The 59 C files and 6 assembly files were flat in src/, with the names the matching agents gave
them (a_ctl.c, control_b.c, init_b.c and so on). In this step I gave every file a name from
what it holds and put it in a directory by subsystem. I only renamed and moved files. No file
was split or joined, because the compiler's output depends on the whole file: the functions
before a function, the data page loads, the .ebss order and the order of the literals. The
gate gave an identical image after each batch.

map/filenames.tsv lists every file: its old path, its new path, the kind of name and the
evidence. The kinds are:

- original (7): TI's own files, kept under TI's names: CodeStartBranch.asm,
  F2806x_CSMPasswords.asm, F2806x_usDelay.asm, F2806x_DefaultIsr.c, F2806x_Dma.c,
  F2806x_MemCopy.c and F2806x_PieVect.c.
- likely (1): flash.c is now F2806x_SysCtrl.c. It holds only TI's InitFlash, which TI puts
  in F2806x_SysCtrl.c as a ramfunc. TI's file also has InitSysCtrl, XtalOscSel, InitPll and
  InitPeripheralClocks, but here they are in drivers/device_init.c with init_system and this
  firmware's own set-up, as one object that I cannot split. For the same reason TI's other
  file names (F2806x_PieCtrl.c, F2806x_CpuTimers.c, F2806x_Adc.c, F2806x_Gpio.c, F2806x_EPwm.c)
  name no file: their functions all sit in device_init.c.
- descriptive (57): named from the functions and data in the file. 22 of them kept their
  names, which already said what the file holds (bms_link.c, sci.c, os_task.c and so on).

The layout:

    src/ti/            TI's support files and assembly
    src/boot/          the boot loader (boot_main.asm, boot_csm.c, boot_update.c,
                       boot_flash_loader.c) and app_header.c, the words it reads
    src/rtos/          os_main.c, os_task.c, os_event.c, os_tick.c, os_tick_isr.c, os_cpu_a.asm,
                       os_cpu_sr.asm
    src/drivers/       device_init.c, sci.c, sci_isr.c, pwm_force.c
    src/app/grid/      grid_support.c (was a_ctl.c: task 4 and the grid code)
    src/app/power/     power_limits.c (was control_b.c: task 2), mppt.c, mppt_step.c
    src/app/battery/   bms_link.c
    src/app/comm/      arm_link.c, arm_page_write.c, crc16.c
    src/app/meas/      meas_task.c, dc_measure.c, ac_measure.c (was rating_b.c: task 1),
                       adc_scale.c, meas_avg8.c, avg_win.c, dc_avg.c, eps_rms_accum.c,
                       grid_rms_accum.c
    src/app/rcmu/      rcmu.c, rcmu_selftest.c, rcmu_watch.c, rcmu_fast_check.c
    src/app/state/     state_task.c, op_states.c, app_init.c
    src/app/settings/  settings.c, rating_presets.c
    src/app/control/   converter_ctrl.c, current_ctrl.c, ac_ctrl.c, pi_ctrl.c, pid_inc.c,
                       pid_pos.c, pll.c, island_detect.c, ctrl_glue.c, grid_freq_setup.c,
                       eps_vctl_init.c, eps_vctl_clear.c
    src/app/util/      util.c (was util_ooff.c), rate_limit.c, iq_consts.c

I added src/app/rcmu, src/app/settings and src/app/util to the planned layout, because the
residual current monitor, the settings and the helpers each have a header of their own and fit
none of the other directories. The headers stay in src/include, and the layout specs stay in
src/layout with the new stems (power_limits.spec, dc_measure.spec, mppt.spec, ac_measure.spec,
op_states.spec).

## The tools

Before I moved anything, I made the tools find the sources by walking src/:

- d316.py has `sources()`, which walks src/ and its subdirectories (not src/include) in sorted
  order and fails when two sources share a stem, and `source(name)`, which finds a file by its
  name or stem. Objects, work directories and cinit_order are keyed by stem, so stems must stay
  unique.
- gate.py takes its list from `d316.sources()`. ebss_solve.py --all does too.
- match.py and ebss_solve.py take a path or a file's name or stem (`match.py crc16`).
- tools/rd (objsig, objsig_lib, gen, fieldmap, trydrop, decls, declcount) and
  tools/climb/b_progress.py glob src/ recursively. objsig compares signatures across the
  renames through map/filenames.tsv, and reports a file that is in one set only (before, it
  skipped it in silence).
- tools/climb/check.py takes range A's files from a_types.txt and no longer looks for a_*.c.
- toolchain.toml's cinit_order has the new stems, in the same order. a_types.txt and
  genplan.py have the new names.

rename.py already walked src/ recursively and finds a layout spec by its file's stem. It skips
files whose names start with F2806x_, so the new F2806x_SysCtrl.c (InitFlash) is now skipped
too, like TI's other files.

## The stem effect

A file's name could reach the object through its static symbol names or section names. I
compared the objects before and after (tools/rd/objsig.py, with the .file symbol left out) and
ran the gate. No object changed, so every file took its new name.

## How I checked

- objsig.py check against a signature taken before the first move: 59 of 59 objects the same,
  after each batch.
- Batch 1 moved the 29 files that kept their names. Batch 2 renamed the other 36 and their
  layout specs and updated cinit_order. Batch 3 changed only comments: the file names in the
  sources' comments, README.md, docs/notes.md, docs/hard.md, toolchain.toml and the notes
  columns of map/data.tsv and map/names.tsv.
- The dated logs (then notes-B.md, notes-C.md, hard-B.md and hard-C.md, since folded into
  docs/notes.md and docs/hard.md with today's names; and docs/names/*) kept the old names, with a
  note at the top.
- The gate: IDENTICAL after each batch.
