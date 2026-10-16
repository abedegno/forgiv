# Range C names (0x3EF54A and up): notes

File names: this is a log of its time, so it keeps the flat file names src/ had then. Step 3
of the readability pass renamed the sources and moved them into subdirectories;
map/filenames.tsv gives each old name its new path.

This goes with build/names/C.tsv. Range C is every function loaded at 0x3EF54A or above: the
ramfuncs (loaded at 0x3F1BB7, run at 0x8000), the boot loader at 0x3F5000, and the data, struct
fields and c_*.h macros that those sources use.

## How to read C.tsv

- 903 rows: 75 functions, 241 variables, 536 fields, 44 types, 7 macros.
- By basis: 2 original, 147 likely, 754 descriptive. In detail: functions 2 original, 44 likely,
  29 descriptive; variables 94 likely, 147 descriptive; fields 107 likely, 429 descriptive;
  types 1 original, 9 likely, 34 descriptive; macros 7 descriptive.
- 263 rows are marked "shared" in the note: 143 variables and 120 fields whose words range A or
  B also use. Where A or B already proposed a name for the same word I took it if it fits the
  code here, and said so in the note ("B name", "A name"). Where I disagree I give my name and
  quote theirs.
- Field rows are written `STRUCT.old` and `STRUCT.new`, both with the old struct name, so that
  each row is one mechanical rename. The type rows rename the structs. Anonymous structs are
  listed under their variable (`var_D1C0.xD1C2`).
- Page-struct fields (`PAGE_C0C0.xC0CA` and the like) are listed only where a range C source
  uses them. Unused padding fields are left out.
- App names are snake_case. Types use the `name_t` form that B uses for the same structs, so the
  two tables merge. TI code keeps TI's names: Flash_Erase, Flash_Program, FlashStatus, ECAP_REGS.
  The TI default ISRs, DMA functions, MemCopy, CsmUnlock, InitFlash and DSP28x_usDelay are
  already TI's names and have no rows.
- Every new function and variable name is unique in the table and does not clash with any
  identifier in src/ (checked). Flash_Erase and Flash_Program already exist as macros in
  boot_ram.c; the rename replaces those macros.
- Numbers in notes use the code's units: 0.1 V, 0.01 A or 0.1 A, 0.01 Hz, as stated.

## The big findings

1. **0xDB40 is the residual current monitor (RCMU), not an offset calibration.** The thresholds
   in rcmu_watch are 0.9 times the VDE 0126-1-1 levels: 0.027 A (30 mA, 10 counts), 0.054 A
   (60 mA, 3 counts), 0.135 A (150 mA, at once) and a 0.3 A continuous limit. The "relay test"
   on GPIO52 is the RCMU self-test: it closes a test circuit and expects the reading to move by
   0.02 to 0.15 A. The inputs rcmu_i[0..2] (0xD488..0xD48C) are the residual current in A: RMS,
   mean, and a fast 10-sample mean of meas.leak_i (ADC c12, mA). B calls the same channel `aux`
   and `i2`. The switch dsp_settings.rcmu_enable (0xD8D6) is HR1 bit 1, inverted.
2. **The two "vctl.c" controllers are named the wrong way round.** VCTL (sub_3F3153, block
   0xC400) is the grid-tied controller: a DC bus voltage PI sets the current amplitude, and an
   inner current loop with a repetitive filter makes the bridge voltage. ICTL (sub_3F3229, block
   0xC840..0xCC81) is the off-grid (EPS) controller: an AC voltage PI with a repetitive filter
   sets the current, and a current PI makes the duty. Proposed: grid_ictl_t / eps_vctl_t.
3. **sub_3EFBF2 is the EPS controller init, not a PLL init.** The PLL is sub_3F1D93 on 0xC100,
   set up by sub_3EFDF1. pll_init.c's comment should change.
4. **The battery path has two stages.** EPWM7/EPWM8 drive a phase-shifted isolated stage whose
   duty ramps to 1054 (dcdc_soft_duty). EPWM5 is a buck/boost between the DC/DC high side
   (meas.hv_v, ADC c6) and the DC link (meas.bus_v, c5). The current the controller regulates
   is meas.hv_i (c9), not the battery current; the battery current the ARM gets is computed
   from power. This agrees with B (hv_v, hv_i).
5. **ctrl_slot (0xC83A) is an eight-slot counter.** meas_avg8 counts it; the slow loops run on
   fixed slots: 0 PV1 MPPT and the grid bus PI, 1 PV2 MPPT, 2 PV precharge of the bus, 4 the
   battery outer loops, 6 the PV bus limiter. grid_ictl_t calls it `hold`.
6. **op_state values** (0xCF0D): 5 grid with PV, 6 grid battery only, 7 EPS with PV, 8 EPS
   battery only, 3 fault, 4 update, 0 idle. state_flags bit 13 starts the grid inverter and
   bit 12 the EPS inverter (both set by pv_mppt_step after a rescan, and by task 3).

## Subsystems and functions

### Control interrupt and fast protection (ctrl.c, ramfuncs)

- control_isr (sub_3F2F9C): EPWM4_INT at 19.2 kHz. Waits for ADCINT1, scales the ADC, runs the
  averages, PLL and fast checks, the battery converter, the bridge (grid or EPS), the PV boosts,
  and the AC sums, then loads the EPWM1 compares the PV loops left pending.
- tz_trip_isr (sub_3F1162): EPWM3 trip-zone interrupt. Forces every PWM output off, stops the
  converter, sets fault_grid1 bit 4, counts tz_trip_count, clears the one-shot trips.
- grid_period_isr (sub_3F2F01): ECAP1 interrupt. Grid period from CAP1 (clamped to 25 ms), hands
  the last period's AC sums to task 1, toggles GPIO10.
- fast_fault_check: inverter current over 40.0 A, bus over bus_ov_limit (585.0 V), HV current
  over 200.0, HV bus over 560.0 V, battery over 63.0 V (not in the aging test); also feeds the
  fast residual current mean and runs rcmu_fast_check every 10 samples.
- grid_v_check (sub_3F2E5E): grid voltage peak over 396.0 V for 77 samples, or with eps_cfg
  bit 10 (HR177) the quarter-cycle mean outside 91 to 345 V; sets fault_grid1 bit 12.
- avg_win_step (sub_3F1F20): mean of |v| over 80 or 96 samples with a window check once settled.
- relay_drive (sub_3F292A): GPIO12 (relay_a) is set at a sync sample of the PLL cycle and pulsed
  after 50 cycles; GPIO21 and GPIO53 (relay_b, relay_c) are on, pulsed or off.
- inv_off (sub_3EFF2B): clears the inverter start requests and inv_on.

### Battery converter (cur_ctrl.c, ctrl.c)

- converter_mode: copies references and limits into the current controller block and runs it
  (states 5..8), or runs the low-battery wake regulation (battery to 56.0 V on EPWM5), or turns
  EPWM5 off. Also the EPWM5 soft start of the aging test.
- current_controller: bus pre-regulation until the bus error is within 8.0 V, then either a
  ramped current command (grid states) or a bus voltage PI (EPS states), limited by bus bands,
  battery voltage limits and the charge/discharge bounds; inner current PI gives the duty.
- iso_dcdc_pwm (sub_3F22FC): EPWM7/EPWM8 compares from dcdc_soft_duty while soft_start_phase is
  4 or 50, else forced low in a counter window and the duty reset to 1.
- dcdc_ctrl_init (sub_3F0009) and dcdc_ctrl_reset (sub_3F24FF): gains and limits; clear all.

### Grid-tied inverter (vctl.c, ctrl.c, ctrl_glue.c, detect.c)

- grid_ictl_run (sub_3F21B5) fills the grid controller block and runs grid_ictl (sub_3F3153).
- grid_ictl: slot-0 bus PI to the current amplitude, active and reactive references from the PLL
  sin and cos, current error times 4.5 plus the repetitive filter plus feed-forward.
- rep_filter_step (sub_3F2118): the repetitive delay line, shared with the EPS controller.
- inv_pwm_update (sub_3F221B): inv_duty to the EPWM3 and EPWM4 compares (unipolar bridge).
- grid_pll_run (sub_3F22CA) and pll_step (sub_3F1D93): single-phase PLL with lock counting.
- grid_ictl_clear (sub_3F28CD): clears the outputs and one word of the delay line per call.
- grid_ctrl_init (sub_3EFDF1): all of the above set up, plus the PLL and the islanding detector.
- island_detect_run (sub_3F2981) and island_detect (sub_3F2046): frequency drift
  anti-islanding (HR115), its output injected into the reactive reference.
- q_loop_run (sub_3F22E5) and q_loop_step (sub_3F20AF): cycle-averaged reactive current loop.
- grid_rms_accum (sub_3F2783): per-cycle sums of V^2, I^2, P, residual current^2, DC injection.
- grid_angle_update (sub_3F2EAF): EPS angle follows the PLL in grid states, else free-runs.
- grid_v_hist lead term (in adc_scale): second difference of the grid voltage, +-10 V.

### Off-grid (EPS) inverter (vctl.c, ctrl.c, pll_init.c, pll_clear.c, rms_hand.c)

- eps_vctl_run (sub_3F2176) fills the block and runs eps_vctl (sub_3F3229): amplitude ramp,
  voltage PI plus repetitive filter, current limit, current PI.
- eps_vctl_init (sub_3EFBF2), eps_vctl_clear (sub_3F289F).
- eps_rms_accum (sub_3F2734): EPS port V^2, I^2, V*I per output cycle, to task 1.

### PV boosts and MPPT (ctrl.c, mppt_c.c)

- pv1_ctrl (sub_3F2A67), pv2_ctrl (sub_3F2B37): soft current ceiling, MPPT voltage PI (or a bus
  PI in pv_bus_mode), current PI with a gain change between 2 and 3 A, compare left pending.
- pv_bus_limit (sub_3F29C1): bus voltage PI that caps both PV currents; clamps the MPPT refs.
- pv_mppt_step (sub_3EF54A): once per grid cycle from task 3. PV current-limit ramp, Voc capture
  after a rescan, MPPT dispatch to B's mppt_pv1/mppt_pv2, rescan after 50 low runs.
- pv_ctrl_init (sub_3EFCAD), pv_ctrl_reset (sub_3EFD82), pv1_stop, pv2_stop, pv_stop_all.

### Measurement (adc_scale.c, avg8.c, ch_avg.c)

- adc_scale (sub_3F2584): DMA pairs averaged, offsets taken off, scaled into meas_fast.
- meas_avg8 (sub_3F2C31): eight-sample averages with calibration gains, the bus voltage notch
  filter (50 or 60 Hz coefficients), and the EPWM5 duty feed-forward.
- dc_acc_step (sub_3F27D5): per-cycle DC sums copied to dc_acc for task 3.

### Residual current monitor (offset_cal.c, relay_test.c, meas_watch.c, cal_fault.c)

- rcmu_init (sub_3F00E9), rcmu_step (sub_3F00FB, from a_ctl.c).
- rcmu_zero_step (sub_3F0102) and rcmu_zero_cal (sub_3F01A1): zero offset from 8 samples.
- rcmu_selftest_step (sub_3F0130) and rcmu_selftest (sub_3F01EF): GPIO52 test.
- rcmu_watch_step (sub_3F0162) and rcmu_watch (sub_3F0323): the trip checks.
- rcmu_fast_check (sub_3F32EF): fast trip, and fault_stop2 bit 6.

### Settings and presets (settings.c, presets.c, app_init.c, ctrl_mode.c, coef_init.c)

- app_init (sub_3EF77E), app_vars_init (sub_3EF687), settings_defaults (sub_3EF6AF).
- rating_apply (sub_3EF78B): model check, start delay, preset by preset_index.
- preset_5kw .. preset_2kw, preset_none, init_defaults (named already), rating_presets table.
- grid_freq_setup (sub_3EFBD7): samples per cycle and relay sync points for 50 or 60 Hz.
- arm_run_mode_set (sub_3EFBCF), run_mode_req_get (sub_3EFBD3, unreferenced).
- iq_consts_init (sub_3F1B96): constants nothing reads.
- The settings at 0xD81A..0xD832 map to the grid protection registers through B's page handlers
  and the ARM's dsp_build_page table: HR61/62 start delays, HR63..HR82 voltage and frequency
  limits and times, HR83 10-minute voltage. I used the givenergy-modbus names (v_ac_high_out...).
- Calibration gains 0xD808..0xD819 follow B's cal_gains_t fields (cal_batt_v and so on).

### PI helpers (ctrl_pi.c, pid_inc.c, pid_pos.c, rate_limit.c)

- pi_step (sub_3F1E6B), pi_inc_step (sub_3F1FB9), pid_inc_step (sub_3F1FEF, unused),
  pid_pos_step (sub_3F1EB9, unused), pi_reset (sub_3F1F11, unused), pi_reset_b (sub_3F2037,
  unused, same body), rate_limit (named).

### RTOS (os_*.c, os_cpu_*.asm, rtos_isr.c)

Already named and correct. It is a small custom RTOS, not uC/OS, though the names follow it:
eight tasks, a 16-bit ready mask with a lowest-bit table (os_unmap_tbl), 32-bit event masks per
task, periods in 1 ms ticks from CPU timer 2 (timer2_isr, os_tick), a TRAP #31 context switch.
Tasks: 0 state machine (20 ms), 1 AC measurement (20), 2 control (20), 3 DC measurement (10),
4 (20), 5 ARM link (60), 6 BMS link (40). os_sleep, os_take_events and os_post_flag are unused.

### SCI drivers (sci.c, sci_isr.c)

Already named. Port 0 is SCI-A (ARM link, 45-byte frames starting 'C'; a full frame posts task 5 event 8); port 1
is SCI-B (BMS, RS485 direction on GPIO57). In BMS pass-through mode (bms_relay_active) SCI-A
bytes after "BMS" go straight to SCI-B and back.

### BMS Modbus master (bms_link.c)

Named already except status_d903, which I rename batt_warning_update. All the 0xD900..0xD921
words now have names; docs/battery.md gives the protocol. The FC6 writes:
HR1 wake on/off after batt_cal_request (HR175) rises or falls, HR2 from bms_update_pending,
HR3 from hr200_request, HR4 = 1 from hr201_request.

### Boot loader (boot_*.c, boot_main.asm)

- boot_flash_loader (sub_3F5004): runs at 0x0400. TI's clock, PIE, flash and SCI-A set-up written
  inline, then frames 0x0A, len, cmd, data, sum. 'S' sets the address, 'D' programs 64 words
  below 0x3F4000 (Flash_Program), 'W' erases sectors B..H (Flash_Erase 0xFE) and falls into 'F',
  'F' answers 'O', 'E' answers and then drives GPIO27 high and waits (external reset).
- The boot project's linker symbols get Boot prefixes because the application already takes
  RamfuncsLoadStart and RamfuncsRunStart in this one-link tree (B's rows).

### TI code

F2806x_DefaultIsr.c (92 handlers), F2806x_Dma.c (37 functions, only channel 1 used), MemCopy,
CsmUnlock, InitFlash (OTPWAIT 3, not TI's 5), DSP28x_usDelay, PieVectTableInit: TI's names,
no rows.

## Open questions

- **One symbol, several views.** var_C000 is the scalar state_trace in ctrl.c and the PAGE_C000
  object in ctrl_mode.c and ctrl_glue.c. var_CC80 is a {n, i, flag} struct in rms_hand.c, the
  whole PAGE_CC80 (with the ADC offsets) in adc_scale.c, and part of var_CC40 in ctrl.c.
  var_D808 and var_D820 are scalars in settings.c and structs in B's meas_b.c/state_b.c.
  A rename has to pick one name per symbol; my rows name the scalar, and the page views will
  read oddly (`state_trace.grid_cycle_samples`). A linker alias per view would be cleaner, but
  the gate and the .cmd file are not mine to change.
- inv_on (0xC004): A calls it grid_trip_latch and B eps_vctl_on. The code sets it when either
  start bit (12 grid, 13 EPS) is seen and the bridge runs while it is 1, so neither fits.
- voc_wait_count and pv_both_low_count (0xD136/0xD137) are B's words (mppt_b.c). B calls them
  mppt_dispatch_count and mppt_dispatch_state; my names say what pv_mppt_step counts.
- pv_bus_mode (0xD805): nothing ever writes a non-zero value. B calls it arm_cfg_pending.
  It may be a factory mode the ARM no longer sends.
- rcmu_step_check_off (0xD8DA, HR1 bit 4 inverted): B calls it cal_gain_override. It turns off
  the RCMU jump checks and forces dci_limit and dci_limit_b to 1000.
- batt_cal_request (0xD909, HR175): A and B disagree (A batt_cal_request, B force_charge_req);
  I took A's, which is the ARM's own name. low_batt_wake (0xD90A): I took B's.
- HR26/HR27: the research README says (resolved 2026-09-27) HR26 is the charge limit; a later
  paragraph in the same file reads the cap factors the other way. I kept B's
  bms_charge_limit/bms_discharge_limit for 0xD504/0xD505 and flagged it.
- dci_limit_b (0xD82A), dci_trip_time/dci_trip_time_it (0xD835/0xD836, which the ARM labels
  "grid country byte a/b"), the 0xD833/0xD854 defaults and cfg_flag_a/b (0xD801/0xD802): no
  clear reader; names are guesses from where they are compared.
- meas_fast.aux17/aux18 (0xC0DA/0xC0DC) copy ADC channels c17 and c18, which nothing in the
  image fills, so they stay 0 after InitAdc.
- grid_ictl_t.v_ff2 (0xC42A) is a Uint32 in C400_OBJ and a float in VCTL; its writer is in
  range A. It is added into the bridge voltage.
- The power-quality settings HR300..HR316 (B's hr3xx_value names) and the PF and Q(U) curves
  belong to range A's code; I named only what settings.c defines.

## Likely bugs and oddities

1. **sub_3F29C1 (pv_bus_limit) leaves its offset unset** when op_state is neither 5 nor 7 and
   mppt_flags bit 14 is clear: `if (CF0D == 7) off = 100; else if (CF0D == 5 || b14) off = 200;`
   with no else, so the image adds whatever was left in R0H. control_isr only calls it when
   op_state is 5 or 7 or bit 14 is set, and no task can change those inside the interrupt, so
   in this image the missing case is never reached. It is a latent bug, live if the function
   is ever called from another place or the caller's test changes.
2. **Short BMS poll reads past the reply.** bms_parse_hr reads HR26/HR27 at base 0 + 21/23 when
   bms_fw_version > 3010, but the short poll (bms_full_poll 0) has only HR17..HR25. The values
   come from beyond the 23-byte frame (the harness got 368.86 A and 655.35 A).
3. **An unknown model runs the 2 kW preset.** rating_apply flags preset_index 0 in fault_stop1
   bit 10 and then limits the index to 1..6, so index 0 runs preset_2kw.
4. **Wake mode computes a bus PI it never uses.** converter_mode's low-battery path runs
   wake_bus_pi every sample, but only wake_batt_pi drives EPWM5.
5. **The aging test turns off the 63.0 V battery check** (fast_fault_check needs
   aging_test_enable == 0). By design perhaps, but the battery has no DSP over-voltage trip then.
6. presets.c says sub_3EF6AF calls through rating_presets; it is sub_3EF78B (rating_apply).
7. grid_period_isr computes the period from a stale CAP1 when CEVT1 is not set (the value is
   only kept when CEVT1 is set, so this is harmless).
8. bms_link.c has `#define volatile volatile`, which does nothing.
9. ctrl_pi.c, pll.c, vctl.c, detect.c, pid_*.c each declare their own copy of the PI block
   layout (PI_CTRL, PID_CTRL, local PI_BLK) because the image needs different volatile members.
   Merging them into PI_BLK needs care: only the volatile qualifiers differ.
10. CAL_B and RELAY_TEST, CAL_C and WATCH, C04E_BLK and DET, PAGE_C180 and DET2, PAGE_C100
    and PLL, PAGE_C400/C440 and VCTL/RES, PAGE_C840/CC40 and ICTL describe the same memory with
    different field names. The table gives each pair the same new field names so they can merge.
