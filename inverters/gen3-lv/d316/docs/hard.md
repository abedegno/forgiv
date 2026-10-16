# How the hard functions were matched

No function is left as a blob: every C and ramfunc function of the image is in src/ and proved,
and the gate passes IDENTICAL. This page is the record of the functions that held out longest:
what was different, what I tried, and what finally matched. The general findings behind them are
in [notes.md](notes.md).

File and function names are today's; each function's address name is given once, in its heading,
and map/names.tsv has every rename. Variables keep their address names. A score is the number of
words or instructions still different; the drafts behind the scores were working files and are
not kept. Many functions were close under 15.12.7 and matched only when the compiler moved to
16.9.6 (notes.md, "Which compiler built each file"); a scan of the open drafts with every
installed compiler at one point found no other exact function.

## Grid support and the ARM link

### arm_link_frame (src/app/comm/arm_link.c, 1141 words)

One of the last functions to match. Under 15.12.7 it stayed 34 instructions off in pages 4 and 5,
where the image stores [14] and [21] through `MOVL XAR7, XAR4; ADDB XAR7, #k` instead of
`MOVB XAR0, #k; MOV *+XAR4[AR0]`. Hundreds of forms of those single stores changed nothing.

Matched under 16.9.6 when pages 4 and 5 write their first fields through a local pointer that is
not volatile: `unsigned int *buf;` ... `len = 13; buf = var_DB00;` and then `buf[len++] = ...` for
page 4's `if (var_CDE0 != 0)` block and for page 5's [13] and [14]; the rest of each page keeps
`var_DB00[len++]` (var_DB00 is volatile, as task5_arm_link needs). The compiler then forms
base + 14 and base + 21 in XAR7 as the image does. Reusing the parameter p as that pointer
matches too. Using the pointer for the whole of page 4 or page 5, or for every page, is worse (44
to 283 off); a volatile local pointer changes nothing (34). The length parameter doubles as the
running index (notes.md, "A parameter reused as a local"), and the image's `D212 == 0` jump
shows the missing inner else.

### task5_arm_link, q_of_u_curve, uvr_storage_response, grid_import_current_limit, volt_watt_limit

task5_arm_link, q_of_u_curve (sub_3E9F03), uvr_storage_response (sub_3EA6A7),
grid_import_current_limit (sub_3E8D5F) and volt_watt_limit (sub_3E8ABA) are EXACT under 16.9.6 in
the drafts' plain form (multiply-first, initialisers in the declaration, two returns for
volt_watt_limit). Under 15.12.7 the plain drafts stayed 2, 3, 11, 16 and 24 instructions off
whatever the source after weeks of searching. volt_watt_limit did match 15.12.7 in the end, found
by a batch search, but only with a local reused for the return value and the zero initialisers in a
fixed order (notes.md). The rest of grid_support.c is EXACT under 16.9.6 too.

### arm_page0 to arm_page5 (arm_link.c)

Matched under 15.12.7 with `x = buf[i] << 8; x |= buf[++i] & 0xFF;`. Under 16.9.6 that form puts
i+1 in XAR1 and pushes XAR1; `x = var_CFC0[i++] << 8; x |= var_CFC0[i] & 0xFF;` gives the
image's `ADDB XAR0,#1`.

## Power limits, measurements and MPPT

### current_limit_tapers (src/app/power/power_limits.c, 0x3ECA0A, 985 words)

One of the last functions to match. Matched in plain C under 16.9.6. Starting from a draft
at 54, four changes took it to 0, each a change in which object the compiler thinks an access
reaches, not in the statements:

- **D514's product reads var_D528 and var_D212** (plain `extern float`s at those addresses), not
  `var_D500.s.vD528` and `var_D200.vD212`: `var_D500.s.vD514 = 0.01f * (var_D528 * var_D212);`.
  With the member, the load of D528 waited for the store of D512 (both in var_D500; D526 and D528
  have their address taken by limitf, which is probably why), so D512's MIN came first. Under its
  own name the load moves up and D514's product is done before D512's reloaded MIN, as in the
  image (54 to 21). var_D212 gives the second load of D212 in the image's place.
- **`if (var_D500.s.vD514 > var_D880.vD894) var_D500.s.vD514 = var_D880.vD894;`** for D514's bound
  (not `__fmin` with var_D892.power_plus): one data page with D8B0 for the next compare (21 to 19).
- **0xD502 is read through a plain word pointer to the page**: `unsigned int *p = (unsigned int
  *)&var_D500;` and `p[2]` for all six reads in the two status tests. The pointer is folded to the
  direct address (the same `@0x2` under DP 0x354), but the reads are no longer volatile member
  reads, and the head block and the `UI16TOF32 R4H,@0x5` slot come out as in the image (19 to 2).
  Another volatile object for one of the reads gave the head too but added DP loads;
  `var_D500.w[2]` changed nothing.
- **The discharge limit is one nested `__fmin`, before the charge limit's**:
  `t = __fmin(var_D500.s.vD51C, __fmin(__fmin(var_D500.s.vD510, var_D500.s.vD514),
  var_D500.s.vD518));` then the x line, the `if`, `var_D4E0 = var_D4E2;`, `a = 10.0f * x / v;`,
  `b = 10.0f * -t / v;`. The compiler keeps the four loads above the first division and moves the
  MINs after it; the image's load order (D514 before D510) and registers follow (2 to 0).

What did not matter or did not help at the end: plain or second-struct names for
D8E0/D8C6/D8C5, local pointers to var_D8C0 or var_D500.s (folded away), temporaries for the head
products, a local for the status test, statement orders of the head (3360) and of the tail
copies, local declaration orders, aliases for D510..D51C and for the head stores.

How it got to 54, in order:

- **What the image's code says.** (1) The two conversions of D8E0 come from a block boundary
  between the D524 and D522 computations: at -O1 a common subexpression is only shared inside a
  block. `if (D524 > f42) D524 = f42;` gives the boundary, the image's two UI16TOF32 and its
  operand order (124 as a whole at first, because the registers then shift). D512 has the same
  shape (D212 is loaded twice). The D522 reload after its store, and the f42 reload, come out
  right with `__fmin` for D522. (2) The compiler moves a load of a global into a local down to
  the local's use, past an FS$$DIV call (notes.md, "Loads kept before a call"). In the image
  `var_D4E0 = var_D4E2` is a 32-bit integer move done before the first FS$$DIV call, together
  with the loads of D510/D514/D518/D51C into R5H/R7H/R6H/R4H; the four MINs come after the call.
- **Under 15.12.7** the best was 104: the b-limits read with `*(volatile float *)&` into locals
  before `a` put the loads before the call and save R7H as the image does, but two of the values
  then go to XAR2/XAR3 through MOVL, and v leaves XAR2 (95 to 104). Putting the b-min before the
  a-division in plain C moves the MINs before the call too (108). Also tried then: the D524 and
  D522 if-forms (one gives the image's head, total 124); divisions after the MINs; a static inline
  helper that takes the four b-limits (they are still moved down); the b-limits in locals
  (register, const); labels; MIN macros; every option set and every installed compiler from 15.9
  to 18.1 (16.9.11 gives 116; 16.9.9 and later are worse).
- **Under 16.9.6, from plain C** (no volatile casts), the best drafts came to 53 (979 of 985 words)
  and 47. The tail: `d0 = D510; d1 = D514; d2 = D518; t = D51C; x = __fmin(D51A,
  __fmin(__fmin(D50E, D512), D516)); if (x > var_D4E2) x = var_D4E2; var_D4E0 = var_D4E2;
  a = 10.0f * x / v; d0 = __fmin(__fmin(d0, d1), d2); b = 10.0f * -__fmin(t, d0) / v;` gives the
  four b-limits in R5H/R7H/R6H/R4H across the first division, v in XAR2 and the R7H save. The `if`
  is the branch that keeps the loads up. Writing the b-min in place puts the MIN results in d0's
  register; one nested expression left a value in XAR3. D51C must go into `t`, the float that
  holds `(float)vD505` earlier in the function (R4H in both places). The MPYF32 operand orders of
  the head follow the source (`var_D8C0.vD8E0 * (float)var_D8C0.vD8C6` and the same for vD8C5,
  `0.01f * (var_D500.s.vD526 * var_D200.vD212)`, `0.01f * (var_D500.s.vD528 * var_D200.vD212)`);
  `t * var_D880.vD892` for D516 shares one data page with D894 (with var_D892.power the page is
  loaded again). What was left at 53 was scheduling in three blocks with the same instructions
  and registers: the image converts D8E0 and D8C6 for D524 before `MOV32 XAR6,R0H; MOV AL,@0x2`
  and D8E0 and D8C5 for D522 after them (two extra DP loads, 4 of the 6-word size difference);
  `UI16TOF32 R4H,@0x5` one slot later; and in the D512/D514 block D514's product before D512's MIN
  with D512 reloaded, and D514 loaded before D510. None of this changed with the order of the four
  head statements (all 24), product and sum forms, D50B/D50C/D52C constant types, the head on one
  line, if, `?:` and `__fmin` forms for D524, D522, D526, D528, D512 and D514 (with both var_D892
  and var_D880 objects), statement orders of D512/D514, the order of all local declarations
  (120), where the function sits in the file, every compiler from 15.12.7 to 17.9 (identical
  output), or options. A plain struct in place of the union gives the same code. Volatile probes
  moved the head toward the image (a volatile read of vD8C6 in the D524 line gave 41, of vD8E0 43)
  but were not the answer.

### pv_boost_select (src/app/meas/dc_measure.c, sub_3EC026)

Matched in plain C under 16.9.6, with the .ebss solved again. Three things did it, none of them
volatile:

- **Explicit ifs for the max, not `__fmax`.** `if (x < g + 300.0f) x = g + 300.0f;` (still MAXF32).
  With `__fmax` the `x = g` in the other arm is a register copy (`MOV32 R0H,R1H`); with the if it
  is a reload (`MOV32 R0H,@2`), and the two `x = g` blocks of each case are cross-jumped into one,
  as in the image. The else chain is written the same way (`if (x < G2) x = G2;`), with 5 before 4.
- **One statement per line.** `fb = 0; fa = 0;` on one line comes out as `fb = fa = 0` and the
  register is copied (`MOVB AH,#0; MOVZ AR6,AH`); on two lines the two stores stay separate
  (`MOVB XAR6,#0; MOVB AH,#0`), as in the image. Ones were never merged.
- **The reset is `fb = 0; fa = 0;`** (C01E's flag first). With `fa = 0` first the allocator swaps AH
  and AR6 between the two flags (and the code grows by 6 words). In the final form `int` and
  `unsigned int` flags give the same code.

The float tests are written from the image's branch sense: case 5 is `if (G2 >= G4)` in both flag
arms, `if (G4 < D20C - 200)`, `if (G2 < D20C - 200)` and `if (G4 < D20C)`, with D20C read from
var_D200 again (not x); the flags test is `(var_C01E | var_C018) == 0`.

Before that: 85 with 15.x, 16.3 and 16.6, 148 with 16.9 and later. Writing each float test from
the image's branch condition fixed most of them at once. Under 15.12.7, reading the member through
`*(volatile float *)&var_D1C0.vD1C2` at five of the ten `x = ...` sites (cases 1, 2, 4) gave the
reload and the image's size (358 words) and took it from 85 to 29 (58 with 16.9.6). Making the
member volatile everywhere, separate externs, the file's own scalars, an array view and a pointer
view were all worse (88 to 208). What was left then: in case 5 with the flags set, the image keeps
D1C4 in R1H and D1C2 in R0H for `if (D1C4 <= D1C2)`, ours the other way round, and the two
`fb = fa = 0` pairs.

### meas_pv_power (dc_measure.c, sub_3EC6B9)

Matched with a temporary of its own for the product: the image puts `c * (b * k)` into the
register of `b * k`; written as one expression the compiler reuses c's register.
`e = b * k; ... e = c * e; var_D200.vD22A = e;` (e assigned twice, the first assignment before the
D228 statement) gives the image's code, with `vD22A` volatile (the sum reads it back after the
store). Reassigning `b` itself does not work: b lives in a callee-saved register across the limitu
calls. The products into locals before the 0.6/0.4 filters (notes.md) were the step before.
Tried earlier: tail statement orders, operand orders, the scale as a separate statement or
helper (static inline, four shapes), x and c as int, unsigned or long, volatile on any of
D222..D22A (only D22A matters). With 16.x and no volatile, `D228 + D22A` reloads both (4 off), but
in the other order.

### soc_floor (power_limits.c)

Matched through the struct view of 0xD500 (notes.md, "A non-volatile view lets the compiler place
the reads"): `var_D500.s.vD500.bit.bN` for the five bits of `CF15.b8` and
`var_D500.s.vD502.bit.b11` for `CF19.b10`. The compiler then orders the reads as the image has
them (b3 first, then the b8/b0 pair; the 0xD502 read before the CF19 read), still one load per
bit. Through the volatile array view `var_D500.w[]` they came out in source order whatever the
OR tree: 1,680 trees and every term form, 48 trees pairing b3 with (b8|b0) under 15.12.7 and
16.9.4, casts, alias pointers, helpers and condition forms.

### pv_boost_precharge (src/app/state/op_states.c, sub_3EE48D)

Exact only with 16.x (16.3 to 17.3); the 26 other functions of the file match with 15.12.7 and
16.x alike. For a while this one function was the only evidence that op_states.c was built by a
later compiler; the move to 16.9.6 for the whole tree settled it.

### pv_mppt_step (src/app/power/mppt_step.c, sub_3EF54A)

The second phase's ramp `f02 = step + f02` and the lower clamp `f02 = fmax(f02, 0)` go through
`extern float var_D298` (DP), the compare and the upper clamp `f02 = f04` through
`var_D240[1].f02` (base register XAR4 = 0xD240, AR1 = 0x58). The compiler takes the two names as
different objects and reads f02 back for the compare after the ramp's store, as the image does.
Same words with 15.12.7 and 16.9.6. It uses mppt.c's 0xD136/0xD137 and is probably the end of that
original file.

Before: 16, then 6 with the read-modify-write through a `volatile PHASE *`. Exhaustive volatile
and absolute-view combinations of the eight accesses, a `volatile PHASE *` view, an absolute
`(PHASE *)0xD240` view, an inlined helper with a PHASE pointer (30), a word-array view of var_D240
(folded back), += and `= a + b`, a pointer for the compare, array-style members and the saturation
forms stayed at 6 or more.

## Settings and presets

### preset_5kw .. preset_2kw and the settings defaults (src/app/settings/rating_presets.c)

The five preset tables (sub_3EF7CF .. preset_2kw) and init_defaults are scheduling of constant
loads into R0H..R7H. They matched only with 0xD880 .. 0xD8FF as one object, the 0xD8C0 page a
struct inside it (notes.md, "One object, not one struct per page"); with var_D8C0 separate the
scheduler put the 1000.0f store early. Before that: the stores in natural order and a
permutation climb got the first table to 16; init_defaults was at 172, then 21 with the one
object before a permutation climb. 0xD880 .. are not globals of this file: a global gets
constant-folded, and the image calls FS$$DIV. The old init_defaults draft also carried Ghidra's
MOVIZ misreadings (4828.2188 for 4800, 1002 for 1000, 0xD8A4 as 0.87 * 0xD88E).

### settings_defaults (src/app/settings/settings.c, sub_3EF6AF)

Matched at once with 0xD801 .. 0xD83F as globals of settings.c (0xD834 and 0xD838 are unused
globals there; without them the layout shifts), after a permutation climb of the struct version
stalled at 7 words.

## The converter control module and its ramfuncs

### The functions on the 0xC000 page

converter_mode, fast_fault_check, control_isr (sub_3F2F9C), tz_trip_isr (sub_3F1162) and
current_controller all matched once the 0xC000 page was written as globals of converter_ctrl.c
(notes.md, "Which object a variable belongs to"). The limit at 0xCCBE in current_controller is a
plain float of the file with a .cinit record of its own. The same finding brought most of these
too: rcmu_watch (sub_3F0323), rcmu_selftest (sub_3F01EF), rating_apply (sub_3EF78B), dcdc_ctrl_init
(sub_3F0009), pv_ctrl_init (sub_3EFCAD, PI gains in plain struct order), pv_ctrl_reset
(sub_3EFD82), pv1_stop (sub_3EFF33), pv2_stop (sub_3EFF6F), pv_stop_all (sub_3EFFAD), avg_win_step
(sub_3F1F20, 0xCF3A .. 0xCF3E as avg_win.c's globals), eps_vctl_run (sub_3F2176), grid_rms_accum
(sub_3F2783), iso_dcdc_pwm (sub_3F22FC), relay_drive (sub_3F292A), island_detect_run (sub_3F2981),
pv_bus_limit (sub_3F29C1, the missing else), pv1_ctrl (sub_3F2A67) and grid_v_check (sub_3F2E5E).
dcdc_ctrl_reset (sub_3F24FF, 42 zero stores) was found by a line-permutation climb.

### grid_ctrl_init (converter_ctrl.c, sub_3EFDF1, 314 words)

278 different in plain store order at first; 199, then 104 with the 0xC400 .. 0xC83A block as one
object; 88 after permutation climbs. Then 0 in one step: the stores to each object (the 0xC400
block with 0xC440 as one object, var_C100, var_C04E, var_C180, then the own globals
C00C/C00B/C009) written as a group, in the order the image stores that object's members, the
groups one after another. The value read from 0xD890 is a local (`float v = var_D890;`), read
once for both C436 and C438.

### inv_pwm_update (converter_ctrl.c, sub_3F221B, 89 words, ramfunc)

The phase shift of EPWM3/EPWM4 (CMPA and CMPCTL load modes) from var_C011 and var_C00C. The image
reads C00C three times (C00B's value into AR6, the sum, the sign test into AH) before it stores
C00B and C009. Matched with the three reads into locals first and the two stores written through
`*(volatile int16 *)&var_C00B` and `&var_C009`: 15.12.7 then keeps that order; with plain stores it
sinks the sign test's read below them.

Before: 11 with var_C00C volatile; 4 on converter_ctrl.c's own globals (ours read C00C into AL and
reloaded C009); 2 with the sum in a local (`var_C00B = var_C00C; s = var_C00C + var_C011;
var_C009 = s; .. else if (s <= 0)`), which gave the image's registers (AR6, AL, AH), leaving
only the sign test's read after the stores. Tried without effect: a separate local for the test
read, stores in both arms, volatile C00B/C009, C00B/C00C as one struct (23), the statement order
of the stores, the operand order of the sum, the forms of the two tests, every flag set.

### grid_ictl (src/app/control/ac_ctrl.c, sub_3F3153) and eps_vctl (ac_ctrl.c, sub_3F3229)

grid_ictl reaches a member more than 255 words into its 0x43B-word block (0xC400 .. 0xC83A, one
object) with `MOVL XAR0,#k` and `*+XARn[AR0]`. 15.12.7 and 16.9.5 and later give that; 15.12.0 to
15.12.6 and 16.9.0 to 16.9.4 do not. It matched as soon as the compiler was 15.12.7.

eps_vctl matched under 16.9.6 from a 15.12.7 draft that was 4 off: res.r stored through a pointer
(`Uint16 *pr = &p->res.r; pr[0] = p->n;`) and the sum as `p->n + p->len - p->w0F`. Under 15.12.7
the pointer was XAR4 where the image has XAR7 (the image stores res.r through XAR7 = p + 23 and
keeps AR1 = 0x441 for n). Tried under 15.12.7: statement order, member types, a RES pointer for the
block, `*(&p->res.r)`, the order of the index sum, casts, inline setters, block or function
scope, volatile qualifiers (14 at first).

### rep_filter_step (ac_ctrl.c, sub_3F2118)

The image loads the ring indices with `MOV AL,*; MOV ACC,AL<<1`. Matched with RES.w and RES.r
volatile and read into Uint16 locals (`w = p->w; pw = &buf[w];`, buf a local `p->buf`): 15.12 then
loads each index into AL where the statement is and shifts it later. The last word was the
operand order of `p->in * (1.0f - p->a)` (`MPYF32 R1H,R2H,R3H`). It sits in ac_ctrl.c in front of
grid_ictl, the other user of RES, which still matches with the volatile indices.
Before (29): array, pointer, casts, locals, bit fields, plain volatile, int16, long, int and
Uint32 index locals, a local buf pointer; all gave `MOV ACC,*<<1`.

### pv2_ctrl (converter_ctrl.c, sub_3F2B37)

The twin of pv1_ctrl (sub_3F2A67), which matched earlier. The image stores `f00 += 10` and reads
f00 back for the fmin (`MOV32 *+XAR4[0],R0H` through base + 86, then `MOV32 R0H,@0x16`); phase 1
keeps the sum in R0H. Matched with f00 reached through `volatile PHASE *ph = &var_D240[1]` only for
the ramp, the rest plain `var_D240[1].x`; 15.12 then gives exactly the image's mix of base-register
and DP accesses. PHASE.f00 volatile in the header breaks phase 1. The b0E.e saturation is the
if/else form (`if (e >= 200) e = 200; else e = __fmax(e, -200);`), where phase 1 has the ternary
with a temporary. Before: 15 to 19; pointer forms fold away; an absolute `(PHASE *)0xD240` view
gave 9 in a mixed form that is not the image's; saturation forms did not change it.

### grid_angle_update (converter_ctrl.c, sub_3F2EAF)

The grid angle wrap is `if (var_CC40.xCC7E < 0) var_CC40.xCC7E = var_CC40.xCC7E + 2pi; else if
(.. > 2pi) {..}` on a plain struct member, with no local and no volatile: 15.12 reloads it once
after the store and keeps it in R1H, the constant in R0H, as the image does. The stores to the step
count, index and wrap flag (0xCC80..0xCC82) keep their order relative to the angle only when they
are members of the same object, so 0xCC40 .. 0xCC82 is one object (control_isr's `var_CC80.xCC80`
still matches). Before: 28 to 33 with 0xCC7E reloaded through volatile or a temporary (which then
landed in R0H, not R1H); 9 with a local for the sum; +=, operand orders, function-scope
temporaries and the arms with and without a local did not help.

### grid_ictl_clear (sub_3F28CD) and eps_vctl_clear (sub_3F289F)

Zero stores. grid_ictl_clear (93 words: sixteen zero stores and the 0xC452 ring) belongs in
converter_ctrl.c (0xC016 is its own global); the image cycles the zero registers R1, R0, R1, R4,
R3, R2 ..., and a permutation climb left 7 zero registers where the image has 6 (28). With the
0xC400 block as one object it went to 19. eps_vctl_clear (46 words) clears eleven floats; the image
zeroes R0H..R5H and stores some twice (R0H to 0xCC50, 0xCC5C and 0xCC78; R1H to 0xCC60 and
0xC850; R2H to 0xCC5E and 0xCC7C). Both are proved; my notes do not record the final change
for either.

### dc_acc_step (src/app/meas/dc_avg.c, sub_3F27D5) and grid_period_isr (sub_3F2F01)

grid_period_isr copies seven sums from 0xD380 to 0xD350 and clears them: matched with all the
copies first and then all the clears. dc_acc_step copies ten sums from 0xD19C to 0xD180 and clears
them; each store to 0xD180.. had its own MOVW DP except two in a row (D192, D18E) that shared one.
Matched with 0xD180 .. 0xD197 as one volatile struct (count and sums), copies first, then clears.
Before: 39 and 47 after permutation climbs, with 0xD180 as plain externs; the image loads nine
sums into registers first and saves only XAR2.

### adc_scale (src/app/meas/adc_scale.c, sub_3F2584, 432 words)

2 words: one LSR scheduled before two MOVXI instead of after them (channel 1 of the ADC averages).
Matched with the DMA buffer at 0xDB86 as `extern volatile Uint16 var_DB86[32]`. The operand order
and form of the first two averages, a permutation of the 18 float lines and of the 16 integer
lines, and the frame's local had not moved it.

### pi_step (sub_3F1E6B) and pid_inc_step (src/app/control/pid_inc.c, sub_3F1FEF)

A PI step on a float struct with a saturation macro that copies the value into a temporary, and
an incremental PID on the same state block. Both turned on the saturation form (notes.md): pi_step
has `{ float t_ = i; if (t_ > max) i = max; else i = __fmax(t_, min); }` first and the ternary
second; pid_inc_step needs umax volatile and `t_ <= max ? __fmax(t_, min) : max`, so it is in a
file of its own with its own block type. Before: 11 and 2, the image reading p->i in the parallel
slot of the multiply and p->u before p->umax.

### rcmu_fast_check (src/app/rcmu/rcmu_fast_check.c, sub_3F32EF, 49 words)

The calibration fault check. 2 under 15.12.7: the last store (`OR @0x18,AL`, the bit field
var_CF12.w18.bit.b6) came before the frame release in the image and after it in ours. 15.12.7 moves
the SUBB of an unused frame above the last instruction of a straight block every time; 16.6.0 and
later do not, and the draft is EXACT with them unchanged. Tried under 15.12.7: the status words
volatile (worse), the operands of the OR both ways, locals, bit-field forms, the store as `&=`/`|=`,
volatile casts on the destination, an absolute 0xCF18 view, fabs forms, early returns and
duplicated tails, goto/for/do-while forms, a `float d` local, the globals defined in the file, and
every -O1 flag set.

### Other functions proved early

pll_step (sub_3F1D93), island_detect (sub_3F2046, the volatile limit), q_loop_step (sub_3F20AF),
pid_pos_step (sub_3F1EB9), grid_ictl_run (sub_3F21B5), eps_rms_accum (sub_3F2734), meas_avg8
(sub_3F2C31) and eps_vctl_init (sub_3EFBF2) were proved in the same stretch of work as grid_ictl,
after the move to 15.12.7; my notes give no single deciding change for them. Before that,
grid_ictl_run was at 12 (the image stores 0xC402 before 0xC400, = C0E8 + C098), and eps_rms_accum
moved the store of CC82 = 0 and the copies.

## RTOS and drivers

### os_task_create (src/rtos/os_task.c, 30 words)

7 words: only the register choice after the call to os_stack_init (the image keeps the TCB pointer
in XAR5 and the zero for `events` in XAR6). Matched with a separate local for the stack top: `top =
os_stack_free; os_stack_free += stk; sp = os_stack_init(task, top);`. It sits between os_start and
os_sched_isr (the .ebss solved again). Tried before: declaration order of the locals, `os_tcb[id].x`
against a pointer, the call inside the store, `os_stack_free = sp + stk`, `0L`, `events` volatile or
not (36 variants).

### task6_bms_link (src/app/battery/bms_link.c, 484 words)

Matched with three changes. The reply buffer is `rx = (unsigned int *)0xD940`, an absolute address
the compiler cannot relate to var_D940, so it stays in XAR2 for rx[1], rx[n-1], rx[n-2] and
CRC16(rx, ..), while `var_D940[i] = c` uses a fresh `MOVL XAR4,#0xD940` (the ARM request copy uses
`src = (unsigned int *)0xD60A` the same way). Two missing else arms: `else var_D919 = 0;` after the
CRC test and `else var_D90C = 0;` after `if (var_D90B == 1) {..}`. And the functions in address
order (task6, then bms_lost, then batt_warning_update), so that the DP is reloaded after the call.
Before: 65, 63 and 57 (alignment scores). The shared tail had already been fixed: the
`if (ev == 2)` at the loop head is the end of the loop's `else if (var_D906 == 2) var_D906 = 0`,
and the pass-through path is a plain `continue`. Register, const and volatile pointer variants,
casts through an integer and a static or inlined helper did not keep the pointer; a non-inlined
helper does, but is a separate function. `(var_D91A + 3) << 1` is not folded into `D91A*2 + 6` in
the image.

### sci_rx (src/drivers/sci.c, 201 words) and ring_read (sci.c, 40 words)

sci_rx: the logic and layout matched early, but the image tests SCIRXST.RXRDY and reads SCIRXBUF
through XAR5, the base it loaded for the RXERROR test, where 15.12 reloaded the data page
(`MOVW DP,#0x1c1; TBIT @0x15,#6`). Matched with `((volatile Uint16 *)&SciaRegs)[7] & 0xFF` in the
two normal receive paths (notes.md, "Register blocks read as words"). The forwarding path
(`var_D900 = 0; SciaRegs.SCITXBUF = ScibRegs.SCIRXBUF.all & 0xFF;`) is a plain member read
(`MOVB AL.LSB,@0x17`), and var_DAAC is volatile (`var_DAAC &= 8` ANDs both halves in memory, then
the test reads it back). Tried before: a local pointer to SciaRegs (propagated away),
`.all & 0xFF`, `.bit.RXDT` and casts for each read, the RXRDY test as `== 0`, `!= 1` or a mask,
other optimisation levels.

ring_read: 21 under 15.12.7, only registers (the image copies `out` to XAR7 at entry and keeps
the ring in XAR4). Its draft is EXACT unchanged with 16.3.0 and later. Tried under 15.12.7: copies
of out, `r` computed before the port test, an inlined helper, volatile members and parameter, the
ring as `sci_rx_ring[port].x` throughout (27), parameter order, body shapes.

### InitPieVectTable (src/drivers/device_init.c)

Matched 15.12.7 with `Dest` declared first; under 16.9.6 it matches as TI's own F2806x_PieVect.c
text, `Source` before `Dest`.

## The boot loader and the -Ooff helpers

### boot_flash_loader (src/boot/boot_flash_loader.c, sub_3F5004, 546 words, runs at 0x0400)

The boot loader's update routine, copied to RAM and run there. Its set-up is TI's code written
inline: XtalOscSel with the 1 ms delay as an empty `for (i = 0; i < 20000; i++)` after
`XTALOSCOFF = 0`, InitPll(9, 2) with the constants folded (no DisableDog), part of
InitPeripheralClocks, InitPieCtrl, InitFlash with OTPWAIT 5 (the application's ramfunc uses 3),
InitSciaGpio, SCI-A at 0x0124 baud, GPIO27 as an output. Flash_CPUScaleFactor (0x0D04) =
0x012000BC and Flash_CallbackPtr (0x0D02) = 0 are the boot ROM flash API's variables; Flash_Erase
and Flash_Program are called at 0x3FFEBD and 0x3FFEBB. The receive path is
`for (;;) { while (!RXRDY) {..errors, timeout..} ... }`; the frame index is an if/else-if chain,
not a switch; the received sum compares as `sum == (rx[n-1] << 8) + rx[n]`; the flash address is a
`Uint16 *` (kept in R4H, as the image does) and the block offset a `long`; c and reset are plain
locals that the compiler spills (MOV forms), c declared after reset under 15.12 (before it under
16.9.6, where their slots swap); the reply sum is `sum = tx[0] + tx[1]; sum += tx[2];`; the 'W'
erase case falls through into case 'F'; SCIRXBUF is read as a word of the register block.

Matched under 16.9.6 when the reply code is a 16-bit variable of its own and the status is not.
The image loads the reply codes as `MOVB XAR4,#0x4f` and tests the flash API's result as a 32-bit
value (`MOVZ AR4,AL; MOVL ACC,XAR4`). Both fit one source: `Uint16 r` for the reply, and the
status assigned to the 32-bit loop counter `i`, tested there and copied to `r` only when it is not
0 (`i = Flash_Erase(...); if (i != 0) { r = i; break; }`, and for 'D'
`if (i == 0) r = 'O'; else r = i;`). `r` and `i` then share XAR4, a 16-bit constant goes straight
into it, and the erase test comes out NEQ-first. Writing the status into the reply variable
itself, whatever its type, gives the ACC route.

Before: 22 to 30 for a long time (82 and 87 by match.py's count under 15.12.7 and 16.9.6), the
reply codes as `MOVB ACC,#79; MOVL XAR4,ACC` in three of the four blocks (the S block, where ACC is
busy, matched) and the erase result branching the other way. The same differences came with every
compiler from 15.9 to 18.1 and every option set (-O0 to -O3, --opt_for_speed 0..5, -ms, -mn, -g,
-mi, -md, without -mt, fp_mode, tmu/cla/vcu). Also tried: seven forms of the Flash_Erase result
test and eight forms of the W and D result tests with gotos, all 144 orders of the switch cases
(with and without the W/F fall-through), the code as a separate variable of every integer and
pointer type (54 to 232), the loop counters split from it (124 to 232), literal forms, the reply
block as a static inline function (41), 60 random local declaration orders and `register`.

### boot_main (src/boot/boot_main.asm, 0x3F5226, 102 words)

Assembly, not C with asm statements, and EXACT on the first try as assembly. Its first
instructions write GpioCtrlRegs before the frame is allocated (`ADDB SP,#4` comes after `EDIS`),
and nothing follows the final `LB 0x3E8004` (no SUBB SP, no LRETR); no TI C compiler places a
prologue after body code or drops the epilogue of a function whose last statements are asm. The
rest (the watchdog write, RPT || MOV clears, `POP ST1`, `POP ST0`, `LB` to absolute addresses) is
boot ROM style code that C cannot express. The middle part (a volatile local stored and read back,
`SBF; SB; SB` for an if/else) looks like compiler output pasted into the assembly file; it is
written out as the image has it. map/functions.tsv gives boot_main the kind `asm`, and the file's
header comment gives this evidence.

### MemCopy (src/ti/F2806x_MemCopy.c, 0x3F52BB, 9 words)

TI's F2806x_MemCopy.c unchanged. It keeps the end address in ACC for the whole loop and copies
through PL (`MOV PL,*XAR7++; MOV *XAR6++,P`); every 15.x compiler at -O0 to -O3 reloads ACC in the
loop. Exact with 16.3 to 17.9 at -O1, so 16.9.6 builds it.

### checksum_xor, isqrt_div, limitu, limiti (src/app/util/util.c)

The rest of util.c matched at -Ooff with any compiler. These four are exact with 16.9.0 to 17.9.0
at -Ooff. checksum_xor (34 words) and isqrt_div (74 words) differed in the frame layout: 15.12 gives
the arguments their slots in order first and pads the long local to an even offset (isqrt_div's
frame was 8 words, the image's 6). limitu and limiti (16 words) differed in the pointer reload: the
image reloads the pointer argument from the frame into XAR7 for each store, 15.12 stores through
XAR4. Under 15.12 every flag that could touch the frame (-g, -ms, -mf, --symdebug, -ss, C99,
relaxed or strict ANSI, K&R parameters), `*x`, `x[0]`, casts, a ternary, volatile and other
argument types changed nothing. checksum_xor's test is `if (sum == 0 || sum > 0xFFFF) sum =
0xFFFF;` (the image's `SB 6,EQ` goes to the store, not past it); the draft had `sum != 0 &&`. The
store block is a join, which is why 16.x loads 0xFFFF again there.
