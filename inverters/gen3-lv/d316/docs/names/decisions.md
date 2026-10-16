# Name reconciliation: decisions

File names: this is a log of its time, so it keeps the flat file names src/ had then. Step 3
of the readability pass renamed the sources and moved them into subdirectories;
merged.tsv here keeps the old names too. map/filenames.tsv gives each old name its new path.

build/names/merge.py makes build/names/merged.tsv from A.tsv, B.tsv and C.tsv with the decisions
below. tools/rename.py applies merged.tsv; build/names/hold.tsv lists the rows it skipped.

## Rules

- One new name per old identifier. Where the tables disagree, the name follows the code and the
  notes' evidence. When the evidence is even, a name two tables share wins, then the name of the
  range that owns the code, then the ARM's own name.
- A struct field that describes the same word as a variable takes the variable's name (B gave a
  var and its field the same name so that they can merge later). So when a var decision overrides
  B or C, their field for that word follows.
- VCTL and ICTL: C's correction holds. VCTL (0xC400, sub_3F3153) is the grid-tied current
  controller, now grid_ictl_t and grid_ictl. ICTL (0xC840, sub_3F3229) is the EPS voltage
  controller, now eps_vctl_t and eps_vctl. No A or B name relied on the old meaning (B's
  eps_vctl_enable and eps_vctl_out are in the ICTL block already).
- TI code keeps TI's names. TI's files (F2806x_*, CodeStartBranch.asm, f2806x_flash.h) are not
  touched. B's renames of its own copies of TI register types to TI's names are applied where they
  do not meet TI's header in one translation unit.
- Include guards are not renamed (B_COMMON_H and the others). The header files keep their names,
  and B's F2806X_* guards would make B's partial register headers look like TI's.
- a_word is not renamed: it is unused and should be deleted later.

## Symbols that are one thing in some files and another in others

A row with a scope in merged.tsv applies only to the files it names. The unscoped row is the
symbol's main name and its map/data.tsv row. A scoped var row is a struct view of the same words;
tools/rename.py gave each view an `alias` row in map/data.tsv, so the linker resolves it to the same
address. A scoped macro row is a_ram.h's alias of one word, used only by a_ctl.c and a_armlink.c.

| Old | Main name | Other names (scope) | Why |
|---|---|---|---|
| var_C000 | state_trace (scalar in ctrl.c, state_b.c) | ctrl_globals (c_ctrl.h, ctrl_mode.c, ctrl_glue.c) | The view is ctrl.c's globals from 0xC000 seen as one struct. state_trace.grid_cycle_samples would read wrongly. |
| var_CC80 | eps_cycle (the page view in adc_scale.c, pll_init.c, rms_hand.c, c_ctrl.h) | eps_cycle_len (a_ram.h macro) | a_armlink.c writes only the first word, the cycle length (C's field cycle_len). |
| var_CE00 | freq_resp (B's struct of the block) | uf_start_freq (a_ram.h macro) | a_ram.h's macro is the first word of the block. ram_CE00 (a_ctl.c's definition) is also renamed freq_resp: same object. |
| var_CF12 | fault_words (B array, C struct) | fault_grid1 (a_ram.h macro) | The macro is only the first fault word. |
| var_D808 | cal_batt_v (settings.c, arm_page_b.c) | cal_gains (b_meas.h, meas_b.c) | settings.c defines the gains one by one; B reads them as one struct. |
| var_D820 | f_ac_high_out (settings.c, a_ram.h) | grid_limits (state_b.c) | As var_D808. grid_limits' fields take the scalars' names. |
| var_D892 | batt_power_rating (B's {charge, discharge} pair) | batt_chg_power_max (a_ram.h macro) | The macro is the first float; the name is B's field for that word. |
| var_D8C0 | dsp_settings (the settings page) | inv_power_abs (a_ram.h macro) | The macro is the float at 0xD8C0, which a_ctl.c writes as abs(inverter power). B's and C's first field follows. |

## Decisions where the tables disagreed, or where a name was changed

| Old | Chosen | Proposals | Why |
|---|---|---|---|
| sub_3EF54A | pv_mppt_step | B:mppt_pv_dispatch, C:pv_mppt_step | C owns the code. It is the per-cycle PV step: current-limit ramp, Voc capture and the MPPT dispatch, not only a dispatch. |
| sub_3EFBD7 | grid_freq_setup | B:set_grid_freq_params, C:grid_freq_setup | C's name. It sets the samples per cycle and the relay sync points for 50 or 60 Hz. |
| sub_3EFD82 | pv_ctrl_reset | B:pv_boost_stop, C:pv_ctrl_reset | C's name. It clears the PV loops, ramps and boost flags, which is more than stopping the boosts. |
| sub_3F1162 | tz_trip_isr | B:epwm3_tzint_isr, C:tz_trip_isr | C names the job, B names the vector. The vector is in the notes. |
| sub_3F170C | os_post | B:os_post_delayed | B proposed os_post_delayed, but 0x3F170C is os_post itself (map/functions.tsv). The call is the same function, so it takes os_post. This is the rename that docs/notes.md says moved rating_b.c's .ebss; tools/ebss_solve.py put it back in one round. |
| sub_3F2F01 | grid_period_isr | B:ecap1_isr, C:grid_period_isr | As above: the job (grid period from ECAP1), not the vector. |
| sub_3F2F9C | control_isr | B:epwm4_isr, C:control_isr | As above: the 19.2 kHz control interrupt. |
| var_C004 | inv_on | A:grid_trip_latch, B:eps_vctl_on, C:inv_on | Flagged. ctrl.c sets it to 1 when start bit 12 (grid) or 13 (EPS) is seen; inv_off and the fast fault clear it; the bridge runs while it is 1 (control_isr). So it is neither a trip latch (A) nor EPS only (B). |
| var_C020 | ecap_valid_count | A:ctl_settle_cnt, B:ecap_valid_count, C:ecap_valid_count | Two tables agree; ctrl.c counts grid period captures to 100. |
| var_C030 | relay_ctl | B:ctl_c030, C:relay_ctl | C's name (count and the three relay commands). |
| var_C0C0 | meas_fast | B:adc_buf_c0c0, C:meas_fast | C's name. |
| var_C1C0 | adc_ch | B:adc_buf_c1c0, C:adc_ch | C's name. |
| var_C408 | q_current_ref | A:q_current_ref, B:q_cmd | A writes it (the reactive current reference, rate limited, under DINT). |
| var_CF13 | fault_grid2 | A:fault_w13 | A's macro only; named to match B's fault word names (fault_grid1, fault_grid2 ...). |
| var_CF14 | fault_grid_eps | A:fault_w14, B:fault_grid_eps | B's fault word names describe the bits; A's fault_wNN are placeholders. |
| var_CF16 | fault_pv | A:fault_w16, B:fault_pv | As var_CF14. |
| var_CF17 | fault_stop1 | A:fault_w17, B:fault_stop1 | As var_CF14. |
| var_CF18 | fault_stop2 | A:fault_w18, B:fault_stop2 | As var_CF14. |
| var_CF19 | batt_blocks | A:batt_block, B:batt_blocks | As var_CF14 (plural, as B). |
| var_CF1C | lim_soft_start | A:lim_soft_start, B:batt_power_max | A writes it: soft_start_ramp's power ceiling. B only reads it as a cap. B's field power_limits_t.f1C follows. |
| var_CF2C | out_power_limit | A:p_limit, B:out_power_limit | Both describe the same thing; B's name says what it limits. |
| var_CFC0 | arm_rx_frame | A:arm_rx_frame, B:arm_rx_page | It holds the whole 0x43 frame, indexed by frame byte. |
| var_D136 | voc_wait_count | B:mppt_dispatch_count, C:voc_wait_count | Flagged. mppt_c.c counts runs after a rescan before the Voc is taken (25). C names what is counted. |
| var_D137 | pv_both_low_count | B:mppt_dispatch_state, C:pv_both_low_count | Flagged. mppt_c.c counts runs with both PV inputs under 120.0 V (50 starts a rescan). |
| var_D14C | pv_detect_done | A:pv_mode_set, B:pv_detect_done, C:pv_detect_done | Two tables agree. |
| var_D309 | grid_freq_detected | A:grid_freq_detected, B:grid_freq_valid | rating_b.c sets it once 50 or 60 Hz has been detected. |
| var_D340 | q_setpoint | A:q_setpoint, B:reactive_power | A writes it: the reactive power setpoint. |
| var_D34A | grid_code | A:grid_code, B:grid_country | A's evidence: the high byte of HR2, the grid code (4 is Italy, CEI 0-21). |
| var_D484 | load_percent | A:p_percent, B:load_percent | The ARM serves it as the load in percent (dsp_p0_w17). |
| var_D7C0 | eps_cfg_bits | A:ctl_option_flags, B:eps_cfg_bits, C:eps_cfg_bits | Two tables agree. It is HR102 and HR177 bits. |
| var_D7C1 | run_status_bits | A:run_flags, B:run_status_bits | B's name; C's struct field run_status matches. |
| var_D7C2 | grid_func_flags | A:grid_func_flags, B:arm_option_bits | Its bits come from HR178, HR179, HR125, HR124, HR176 and HR115, which are all grid functions. C's struct field becomes grid_func to match. |
| var_D805 | pv_bus_mode | A:first_start, B:arm_cfg_pending, C:pv_bus_mode | Flagged. Nothing writes a non-zero value (a_armlink.c and state_init write 0). Non-zero would run the PV boosts on the bus voltage (ctrl.c), set the grid controller's mode word, move the bus reference and skip the ISO check. C's name says the clearest effect; A's first_start and B's arm_cfg_pending describe the clearing only. B's field cfg_d800_t.v805 follows. |
| var_D806 | qu_q_max | A:qu_q_max, C:arm_frame_const | a_ctl.c uses it as the Q(U) maximum Q; that the frame handler resets it to 438 does not make it a constant. |
| var_D807 | q_mode | A:q_mode, B:hr122_mode, C:hr122_mode | HR122, the reactive power mode. Named by what it selects. |
| var_D80A | cal_pv1_v | A:cal_pv1_v_gain, C:cal_pv1_v | C's names match B's cal_gains_t fields. D80E and D80F are the PV power gains (B: pv1_p, pv2_p), not second current gains. |
| var_D80B | cal_pv1_i | A:cal_pv1_i_gain, C:cal_pv1_i | As var_D80A. |
| var_D80C | cal_pv2_v | A:cal_pv2_v_gain, C:cal_pv2_v | As var_D80A. |
| var_D80D | cal_pv2_i | A:cal_pv2_i_gain, C:cal_pv2_i | As var_D80A. |
| var_D80E | cal_pv1_p | A:cal_pv1_i_gain2, C:cal_pv1_p | As var_D80A. |
| var_D80F | cal_pv2_p | A:cal_pv2_i_gain2, C:cal_pv2_p | As var_D80A. |
| var_D810 | cal_grid_v | A:cal_grid_v_gain, B:cal_grid_v, C:cal_grid_v | Two tables agree. |
| var_D818 | cal_bus_v | A:cal_gain4, B:cal_bus_v, C:cal_bus_v | Two tables agree. |
| var_D819 | cal_d819 | B:cal_d819, C:cal_spare_19 | B notes that the ARM sends 0 there and the gain clamps to 870, so it is not unused; the neutral name stays. |
| var_D837 | ofr_start_freq | A:ofr_start_freq, C:ofr_f_start | A's code uses the over/under-frequency settings; its names follow the function (ofr/ufr). |
| var_D83A | ofr_recover_delay | A:ofr_recover_delay, B:hr303_value, C:hr303_value | A's functional name instead of the hr303_value placeholder. |
| var_D83B | ofr_delay | A:ofr_delay, B:hr300_value, C:hr300_value | As var_D83A. |
| var_D83C | ofr_recover_freq | A:ofr_recover_freq, C:ofr_f_stop | As var_D837. |
| var_D83D | ufr_start_freq | A:ufr_start_freq, C:ufr_f_start | As var_D837. |
| var_D83F | ufr_delay | A:ufr_delay, B:hr312_value, C:hr312_value | As var_D83A. |
| var_D841 | power_ramp_rate | A:p_ramp_rate, B:power_ramp_rate, C:power_ramp_rate | Two tables agree. |
| var_D845 | q_pt1_tau | A:q_pt1_tau, B:hr301_value, C:hr301_value | As var_D83A (the PT1 time constant). |
| var_D846 | pf_curve_p1 | A:pfp_p1, C:pf_curve_p1 | C's pf_curve_ names read better than pfp_. |
| var_D847 | pf_curve_p2 | A:pfp_p2, C:pf_curve_p2 | As var_D846. |
| var_D848 | pf_curve_p3 | A:pfp_p3, C:pf_curve_p3 | As var_D846. |
| var_D849 | pf_curve_p4 | A:pfp_p4, C:pf_curve_p4 | As var_D846. |
| var_D84A | pf_curve_pf1 | A:pfp_pf1, C:pf_curve_pf1 | As var_D846. |
| var_D84B | pf_curve_pf2 | A:pfp_pf2, C:pf_curve_pf2 | As var_D846. |
| var_D84C | pf_curve_pf3 | A:pfp_pf3, C:pf_curve_pf3 | As var_D846. |
| var_D84D | pf_curve_pf4 | A:pfp_pf4, C:pf_curve_pf4 | As var_D846. |
| var_D854 | ofr_it_param | A:ofr_it_param, C:cfg_spare_24 | a_ctl.c reads it (Italian band 1 acts only while it is 20..50), so it is not spare. |
| var_D855 | power_ramp_rate_reconnect | A:p_ramp_rate_reconnect, B:power_ramp_rate_soft, C:power_ramp_rate_soft | HR302 is the ramp rate after a fault (soft_start_ramp), so reconnect fits better than soft. |
| var_D856 | fast_trip_v_low_set | A:fast_trip_v_low_set, B:hr315_low, C:hr315_low | grid_v_fast_trip trips at HR315/HR316. |
| var_D857 | fast_trip_v_high_set | A:fast_trip_v_high_set, B:hr316_high, C:hr316_high | As var_D856. |
| var_D85D | enable_charge_target | A:enable_charge_target, B:charge_target_enable | The ARM's own name (enable_charge_target). |
| var_D886 | grid_omega_step | A:grid_theta_step, C:grid_omega_step | B's field name; C agrees. |
| var_D88A | rated_ac_power | A:rated_ac_power, B:grid_power_max | The presets set it to the rated AC power (36000/46000/50000). B's field follows. |
| var_D88C | grid_power_max_pv | A:rated_apparent_power, B:grid_power_max_pv | B: state 5 (grid with PV) uses it as the output power base. |
| var_D890 | grid_current_max | A:rated_current, C:grid_current_max | B's field name; C agrees. B's rated_current is the field at 0xD888. |
| var_D89C | pv_power_rated | A:pv_power_max, B:pv_power_rated | B's field name. |
| var_D8B8 | arm_batt_setpoint | A:batt_power_setpoint | A proposed batt_power_setpoint, which is now the function forced_charge_discharge. B's field name is used instead. |
| var_D8CA | grid_import_amps | A:grid_import_amps, B:grid_current_limit | HR101, an import current limit in amps. B's field follows. |
| var_D8CC | cfg_changed_flags | A:cfg_changed_flags, B:arm_cfg_changed | The ARM's own name (cfg_changed_flags). B's field follows. |
| var_D8D5 | iso_check_enable | A:hr1_check0, B:iso_check_enable | HR1 bit 0 inverted gates the ISO check (task3_b.c). |
| var_D8D6 | rcmu_enable | A:hr1_check1 | C's finding: HR1 bit 1 inverted switches the residual current monitor. B's field follows (was standby_gate). |
| var_D8DA | rcmu_step_check_off | A:hr1_check4 | Flagged. HR1 bit 4 inverted. When 1, a_armlink.c forces dci_limit and dci_limit_b to 1000 and offset_cal.c turns the RCMU jump checks off. It does not touch the calibration gains, so B's cal_gain_override is wrong. B's field follows. |
| var_D8E1 | battery_type | A:battery_type, B:batt_type_lithium | The ARM's own name (HR54 battery_type). B's and C's fields follow. |
| var_D8EF | aging_test_enable | A:lead_acid_mode, B:aging_test_enable | The DSP uses it for the aging (burn-in) mode, whatever the ARM docs say about lead acid. |
| var_D8F0 | aging_test_mode | A:factory_test_mode, B:aging_test_mode | As var_D8EF. |
| var_D900 | bms_relay_idle | B:bms_hr14_timer, C:bms_relay_idle | Pairs with bms_relay_active (0xD901): 15000 idle passes end the pass-through. |
| var_D901 | bms_relay_active | A:bms_relay_active, B:bms_hr14_flag, C:bms_relay_active | Two tables agree. |
| var_D909 | batt_cal_request | A:batt_cal_request, B:force_charge_req, C:batt_cal_request | Flagged. HR175; batt_cal_request is the ARM's own name. |
| var_D90A | low_batt_wake | A:batt_cal_lowv, B:low_batt_wake, C:low_batt_wake | Two tables agree; the converter then regulates the battery to 56.0 V from PV (wake mode). |
| var_DACE | arm_fault_word | A:arm_fault_word, B:fault_record | D frame bytes 3-6, the fault word sent to the ARM. |
| sci_flag | arm_rx_len | none (extra row) | Flagged (0xCF40). sci.c defines the word and never uses it; a_armlink.c counts the incoming frame bytes in it. One name for one object. |
| B_CE00 | freq_resp_t | B:remote_cmd_t | The 0xCE00 page is the storage frequency/voltage response block of a_ctl.c, not a remote command (A). Its fields take A's names. |
| CH_AVG | dc_acc_task_t | C:dc_acc_t | C gave CH_SUMS and CH_AVG the same name, but both are defined in ch_avg.c with different members. CH_SUMS takes dc_acc_t (as B_CH); CH_AVG, the task copy at 0xD180, takes dc_acc_task_t. |
| B_C208.vC208 | batt_v_max | B:isr_limits_t.v_max | Follows the variable decision for the same word. |
| B_C208.vC20A | batt_v_min | B:isr_limits_t.v_min | Follows the variable decision for the same word. |
| B_CE00.vCE0A | uf_active | B:remote_cmd_t.cmd_a | Follows the variable decision for the same word. |
| B_CE00.vCE18 | of_active | B:remote_cmd_t.cmd_b | Follows the variable decision for the same word. |
| B_CE00.vCE1C | ov_resp_active | B:remote_cmd_t.cmd_c | Follows the variable decision for the same word. |
| B_CE00.vCE1D | uv_resp_active | B:remote_cmd_t.cmd_d | Follows the variable decision for the same word. |
| B_CE00.vCE1E | grid_resp_power | B:remote_cmd_t.power | Follows the variable decision for the same word. |
| B_CE00.vCE28 | resp_restore_power | B:remote_cmd_t.setpoint_echo | Follows the variable decision for the same word. |
| B_CF1A.f1C | lim_soft_start | B:power_limits_t.batt_power_max | Follows the variable decision for the same word. |
| B_D800.v805 | pv_bus_mode | B:cfg_d800_t.arm_cfg_pending | Follows the variable decision for the same word. |
| B_D820.vD820 | f_ac_high_out | B:grid_limits_t.page1_14 | Follows the variable decision for the same word. |
| B_D820.vD821 | f_ac_low_out | B:grid_limits_t.page1_16 | Follows the variable decision for the same word. |
| B_D820.vD822 | f_ac_high_in | B:grid_limits_t.page1_1C | Follows the variable decision for the same word. |
| B_D820.vD823 | f_ac_low_in | B:grid_limits_t.page1_1E | Follows the variable decision for the same word. |
| B_D820.vD824 | v_ac_high_c | B:grid_limits_t.v_high | Follows the variable decision for the same word. |
| B_D820.vD825 | v_ac_low_c | B:grid_limits_t.v_low | Follows the variable decision for the same word. |
| B_D820.vD826 | f_ac_high_c | B:grid_limits_t.f_high | Follows the variable decision for the same word. |
| B_D820.vD827 | f_ac_low_c | B:grid_limits_t.f_low | Follows the variable decision for the same word. |
| B_D880.vD886 | grid_omega_step | B:rating_params_t.grid_omega_step | Follows the variable decision for the same word. |
| B_D880.vD88A | rated_ac_power | B:rating_params_t.grid_power_max | Follows the variable decision for the same word. |
| B_D8C0.vD8C0 | inv_power_abs | B:dsp_settings_t.cfg_d8c0 | Follows the variable decision for the same word. |
| B_D8C0.vD8CA | grid_import_amps | B:dsp_settings_t.grid_current_limit | Follows the variable decision for the same word. |
| B_D8C0.vD8CC | cfg_changed_flags | B:dsp_settings_t.arm_cfg_changed | Follows the variable decision for the same word. |
| B_D8C0.vD8D6 | rcmu_enable | B:dsp_settings_t.standby_gate | Follows the variable decision for the same word. |
| B_D8C0.vD8DA | rcmu_step_check_off | B:dsp_settings_t.cal_gain_override | Follows the variable decision for the same word. |
| B_D8C0.vD8E1 | battery_type | B:dsp_settings_t.batt_type_lithium | Follows the variable decision for the same word. |
| B_RT.f08 | leak_i_sq | B:ac_acc_t.i2_sq | B's i2_sq and C's leak_i_sq name the same member (the tool found the conflict). C's finding: the channel is the residual current, so leak_i_sq. |
| PARAMS_D8C0.xD8E1 | battery_type | C:PARAMS_D8C0.batt_type_lithium | Follows the variable decision for the same word. |
| STATUS_D7C0.w2 | grid_func | C:STATUS_D7C0.arm_option | Follows the variable decision for the same word. |

## Rows held back (build/names/hold.tsv)

| Old | Proposed | Why it stays |
|---|---|---|
| B_AQCTLB_BITS | AQCTL_BITS | B_AQCTLA_BITS takes the same name and both are defined side by side in b_epwm.h. One definition would have to go, which is an edit, not a rename. |
| B_AQCTLB_REG | AQCTL_REG | As above (B_AQCTLA_REG). |
| B_GPA32_BITS | GPADAT_BITS | B_GPADAT_BITS (b_gpio.h) takes the same name, and init_b.c includes both headers. |
| B_GPB32_BITS | GPBDAT_BITS | As above (B_GPBDAT_BITS). |

No name had to stay because of the image: after each batch every file's .ebss was re-solved by
tools/ebss_solve.py and the gate stayed IDENTICAL.

## Task 5 and sci_send

task5_arm_link called `sub_3F1487(0, &var_CF80, p, 45)` with no declaration. sci.c defines
`unsigned int sci_send(unsigned int port, unsigned int len, unsigned int *buf, unsigned int *src)`.
The C28x ABI passes the two ints in AL and AH and the two pointers in XAR4 and XAR5, so the old call
passed port 0, length 45, buffer 0xCF80 and source p. a_funcs.h declared a different sci_send
(port, ring, data, n) that nothing called. Now a_funcs.h has sci.c's prototype and the call is
`sci_send(0, 45, &sci_tx_buf0, p)`. The function compiles to the same words; the new declaration
moved seven of a_armlink.c's .ebss variables, which tools/ebss_solve.py put back in one round. The
gate stayed IDENTICAL.

