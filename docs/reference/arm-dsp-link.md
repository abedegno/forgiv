# The ARM-DSP link, byte by byte

Every byte of the two 45-byte frames, from both ends. The DSP side is
[`arm_link.c`](../../inverters/gen3-lv/d316/src/app/comm/arm_link.c) and
[`arm_page_write.c`](../../inverters/gen3-lv/d316/src/app/comm/arm_page_write.c); the ARM side is
[`dsp_link.c`](../../inverters/gen3-lv/a316/src/app/comm/dsp_link.c) (`task_dsp_tx`,
`dsp_build_page`, `task_dsp_rx`, `dsp_parse_page`). I checked the two against each other byte by
byte; they agree on every position, and the names below are the current ones in both trees. Where
one side sends a byte the other ignores, or always sends 0 where the other reads a value, the
table says so.

## The physical link

- ARM UART4 (PC10 TX, PC11 RX) to DSP SCI-A (GPIO28 RX, GPIO29 TX), 9600 baud, 8N1.
- The ARM sends a 0x43 frame every 60 ms, pages 0 to 7 in turn (each page every 480 ms).
- The DSP sends a 0x44 frame every 240 ms, pages 0 to 6 in turn (each page every 1.68 s).
- On the DSP each byte sits in its own 16-bit word in RAM.

## Common format

| Bytes | Content |
|---|---|
| 0 | type: 0x43 (`DSP_FRAME_TX` on the ARM, `ARM_FRAME_FROM_ARM` on the DSP) or 0x44 (`DSP_FRAME_RX`, `ARM_FRAME_TO_ARM`) |
| 1-6 or 1-11 | the header |
| 7 or 12 | the page number |
| 8-42 or 13-42 | the page |
| 43-44 | CRC-16/MODBUS of bytes 0-42, high byte first |

Multi-byte values are big-endian (high byte first) unless a row says otherwise. The A316 code
counts page offsets from the page number byte, so an A316 offset k is frame byte k + 7 in a 0x43
frame and k + 12 in a 0x44 frame.

## ARM to DSP: 0x43

### Header

| Byte | ARM sends | DSP takes |
|---|---|---|
| 1 | `inv_run_mode`; 10 instead for grid code 4 with HR53 = 0 outside run mode 4 | `arm_run_command` (0..10): 4 on the 6th frame posts `EV0_UPDATE`; 10 posts `EV0_STOP`; stored as `arm_run_mode` |
| 2 | `fault_flags >> 4` | bit 2 sets CF18 bit 8, bit 4 sets CF18 bit 9 |
| 3 | HR50 `active_power_rate`, capped by `drm_power_limit` for grid codes 2, 3 with HR41 = 1 | `active_power_rate`; 105 and above ignored; aging mode 2 forces 70, mode 1 forces 85 (PV2 over 300.0 V) or 41 |
| 4 | the state: 2 under HR329, else 1 for `charge_active` or HR175, else 2 for `discharge_active`, else 0 | `batt_state_cmd` 0..2, the battery force mode |
| 5-6 | HR495 when HR494 = 1, else `-batt_power_calc / 10`, signed | `arm_batt_setpoint` = word x 10 |
| 7 | `dsp_tx_page` 0..7 | `arm_page_no` (above 7 counts as 7) |

### Page 0

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8 | HR2 high byte (`inverter_module_l >> 8`) | `grid_code` 0..28; 20 and 21 become 3 |
| 9 | HR2 low byte | the model: 50, 46, 36 give `model_power_code` 5, 4, 3; other values set CF17 bit 10 |
| 10-11 | HR61 `inverter_start_time` | `inverter_start_time` |
| 12-13 | HR60 `v_pv_input_start`; 1550 while HR175 = 1 | `pv_start_voltage` 1200..1500 |
| 14 | HR125 | `grid_func_flags`: 1 bit 3, 2 bit 12, 3 both, else neither |
| 15-16 | HR1 `inverter_module_h` | each bit inverted: 0 `iso_check_enable`, 1 `rcmu_enable`, 2 `dci_comp_enable`, 3 `hr1_check3`, 4 `rcmu_step_check_off` |
| 17 | HR50 | not used |
| 18 | HR51 `reactive_power_rate` | `reactive_power_rate`, below 61 |
| 19-20 | HR52 `power_factor` | `power_factor_set` |
| 21 | HR114 | `batt_min_reserve` |
| 22 | `charge_active` | `charge_active` |
| 23 | `discharge_active` | `discharge_active` |
| 24 | HR27 `battery_power_mode`; 1 while `hold_active` | `battery_power_mode` |
| 25 | HR53 `inverter_state`; `drm0_active` for grid codes 2, 3, 20, 21 with HR41 = 1 and HR53 = 1 | `inverter_state` (`inverter_enable`) |
| 26 | `g_200001B0` | not used |
| 27 | HR122 | `q_mode` |
| 28 | `g_200001D7` | any value clears `pv_bus_mode` |
| 29-30 | HR62 `inverter_restart_delay` | `inverter_restart_delay` |
| 31 | 0 | not used |
| 32 | HR33 `user_code` | not used |
| 33 | settings +0x74 | not used |
| 34 | HR54 `battery_type` | `battery_type` |
| 35-36 | HR55 `battery_capacity` | `battery_capacity`; `bms_capacity` instead with `battery_type` 1, HR109 set and 50 or more BMS replies |
| 37-38 | `charger_temp` | `charger_temp` |
| 39-40 | `heatsink_temp` | `heatsink_temp` |
| 41-42 | `batt_temp` | `batt_temp`, signed |

### Page 1

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-11 | `p_eps_sample`, 32 bits | `p_eps_sample` |
| 12-13 | HR97 `battery_low_voltage_limit` | `batt_v_low_limit` = word x 0.1 (x 0.5 with HR175, x 0.86 in a calibration), 200..480 |
| 14-15 | HR98 `battery_high_voltage_limit` | `batt_v_high_limit` = word x 0.1 (x 1.05 in a calibration), 540..630 |
| 16-17 | HR64 `v_ac_high_out` | `v_ac_high_out` 2300..3150 |
| 18-19 | HR63 `v_ac_low_out` | `v_ac_low_out` 0..2300 |
| 20-21 | HR66 `f_ac_high_out` | `f_ac_high_out` |
| 22-23 | HR65 `f_ac_low_out` | `f_ac_low_out` |
| 24-25 | HR72 `v_ac_high_in` | `v_ac_high_in` 2300..3150 |
| 26-27 | HR71 `v_ac_low_in` | `v_ac_low_in` 0..2300 |
| 28-29 | HR74 `f_ac_high_in` | `f_ac_high_in` |
| 30-31 | HR73 `f_ac_low_in` | `f_ac_low_in` |
| 32-33 | HR80 `v_ac_high_c` | `v_ac_high_c` |
| 34-35 | HR79 `v_ac_low_c` | `v_ac_low_c` |
| 36-37 | HR68 `v_ac_high_out_time` | `v_ac_high_out_time` |
| 38-39 | HR67 `v_ac_low_out_time` | `v_ac_low_out_time` |
| 40-41 | HR70 `f_ac_high_out_time` | `f_ac_high_out_time` |
| 42 | `auto_test_state` when 0 or 1, else 0 | `auto_test_state`, taken only while it is 0 |

### Page 2

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-9 | HR69 `f_ac_low_out_time` | `f_ac_low_out_time` |
| 10-11 | HR76 `v_ac_high_in_time` | `v_ac_high_in_time` |
| 12-13 | HR75 `v_ac_low_in_time` | `v_ac_low_in_time` |
| 14-15 | HR78 `f_ac_high_in_time` | `f_ac_high_in_time` |
| 16-17 | HR77 `f_ac_low_in_time` | `f_ac_low_in_time` |
| 18-19 | HR82 `f_ac_high_c` | `f_ac_high_c` |
| 20-21 | HR81 `f_ac_low_c` | `f_ac_low_c` |
| 22 | `cfg_changed_flags` | `cfg_changed_flags`: run mode request 0 while set; GPIO27 after 15 s |
| 23 | `grid_country_byte_a` (from the country table) | `dci_trip_time` |
| 24 | HR128 | not used |
| 25 | HR123 | `ofr_droop` 1..100 |
| 26-27 | HR83 `v_10_min_protection` | `v_10_min_protection` 2300..2900 |
| 28 | HR121 | `freq_stage_sel` |
| 29 | HR143 | `qu_lock_out_p` |
| 30 | HR142 | `qu_lock_in_p` |
| 31-32 | HR144 | `pfp_lock_in_v` |
| 33-34 | HR145 | `pfp_lock_out_v` |
| 35-36 | HR26 `grid_port_max_power`; 6000 while HR29 is not 0 | `grid_port_max_power` = word x 10, at least 1200 |
| 37 | HR28 `enable_60hz_freq_mode` | 0 = 50 Hz, 1 = 60 Hz (not in state 4): `grid_freq_nominal`, `grid_omega_step`, `eps_cycle_len` |
| 38 | HR29 `soc_force_adjust` | `batt_cal_stage` |
| 39 | settings +0xD2 (set by HR332) | `aging_test_enable`; 0 to 1 posts `EV0_STOP` |
| 40 | `ct_cycle_count` | not used |
| 41-42 | HR328 | `ofr_recover_freq` 5000..5400 (6000..6400 at 60 Hz) |

### Page 3

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-23 | HR130 to HR137 | `pf_curve_pf1`..`pf4` from bytes 10, 14, 18, 22 (HR131, 133, 135, 137); bytes 8, 12, 16, 20 (HR130, 132, 134, 136) not used |
| 24-25 | HR138 | `qu_v_high1` |
| 26-27 | HR139 | `qu_v_high2` |
| 28-29 | HR140 | `qu_v_low1` |
| 30-31 | HR141 | `qu_v_low2` |
| 32 | `grid_country_byte_b` | `dci_trip_time_it` |
| 33-34 | HR321 | `ofr_start_freq` 5000..5400 (6000..6400 at 60 Hz) |
| 35-36 | HR24 `peek_addr` | `peek_addr`, 0xC000..0xE000 |
| 37-38 | 0 | not used |
| 39-40 | HR326 | `ufr_start_freq` 4600..5000 (5600..6000 at 60 Hz) |
| 41 | HR327 | `ufr_droop` 1..100 |
| 42 | 0 | not used |

### Page 4

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8 | `ct_cycle_count` | not used |
| 9 | HR124 | `grid_func_flags` bit 4 |
| 10-11 | `g_200001A4` | not used |
| 12-13 | settings +0x69 | `dci_limit` 30..1100 (1000 with `rcmu_step_check_off`) |
| 14-15 | settings +0x6B | `dci_limit_b`, the same |
| 16 | HR175 `batt_cal_request`; the ARM clears it after more than 300 page-4 frames with `batt_voltage` > 540 and `batt_current` > 1000 | `batt_cal_request`, and from it `low_batt_wake` |
| 17 | HR109 | `enable_bms_read` (1 = full BMS poll) |
| 18 | HR176 | `grid_func_flags` bits 15, 6, 8 from bits 0, 1, 2 |
| 19 | HR111 `battery_charge_limit` | `battery_charge_limit` 1..100 |
| 20 | HR112 `battery_discharge_limit` | `battery_discharge_limit` 1..100 |
| 21 | `discharge_min_soc_now`; 4 under HR329 | `discharge_min_soc` 1..100 |
| 22 | HR115 | `grid_func_flags` bit 11 |
| 23-24 | `pv_power_override` | not used |
| 25 | HR177 | `eps_cfg_bits` bit 10 |
| 26 | `bms_update_pending` | `bms_update_pending`: run mode request 0; FC6 BMS HR2 |
| 27 | `hr200_request` (HR200) | `hr200_request`: FC6 BMS HR3 |
| 28 | the meter fault flag with meters (HR7 not 0), else 1 on the aux link fault | CF19 bit 2 |
| 29-42 | 0 | not used |

### Page 5

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-9 | `cal_gains.gain0` | `cal_pv1_v` 870..1178 |
| 10-11 | `offset0` | not used |
| 12-13 | `gain1` | `cal_pv1_i` and `cal_pv1_p` 870..1178 |
| 14-15 | `offset1` | not used |
| 16-17 | `gain2` | `cal_pv2_v` 870..1178 |
| 18-19 | `offset2` | not used |
| 20-21 | `gain3` | `cal_pv2_i` and `cal_pv2_p` 870..1178 |
| 22-23 | `offset3` | not used |
| 24-25 | `gain4` | `cal_bus_v` 870..1178 |
| 26-27 | `offset4` | not used |
| 28-29 | `gain5` | `cal_grid_v` 870..1178 |
| 30-41 | `gain11`, `offset11`, `gain9`, `offset9`, `gain10`, `offset10` | not used |
| 42 | `hr201_request` (HR201); sent in 5 frames, then cleared | `hr201_request`: FC6 BMS HR4 (D316 findings-power item 3) |

A gain of 1024 (0x400) is 1.0 on both sides.

### Page 6

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-11 | `gain6`, `offset6` | not used |
| 12-13 | `gain7` | `cal_power` 870..1178 |
| 14-15 | `offset7` | not used |
| 16-17 | `gain8` | `cal_batt_v` (no clamp) |
| 18-19 | `offset8` | not used |
| 20 | the pack poll request flag (`dsp_bms_raw` + 0x98); cleared after sending | 1: copy bytes 21-26 to `arm_bms_request` and set `arm_bms_request_pending` |
| 21-27 | the request staged by `bms_poll`: device, 4, start (2), count (2), and a byte that is always 0 | `arm_bms_request` from bytes 21-26 |
| 28-29 | 0 | `cal_d819` 870..1178, so always 870; I found no reader of it |
| 30-31 | HR300 | `ofr_delay` |
| 32-33 | HR301 | `q_pt1_tau` |
| 34-35 | HR302 | `power_ramp_rate_reconnect` 10..30000 |
| 36-37 | HR303 | `ofr_recover_delay` |
| 38 | HR178 | `grid_func_flags` bit 0 (0 or 1), the export limit switch |
| 39 | HR179 | `grid_func_flags` bit 2 (0 or 1) |
| 40 | HR305 | `pv_mode_cfg` 0..4 |
| 41-42 | 0 | not used |

### Page 7

| Byte | ARM sends | DSP field (clamp) |
|---|---|---|
| 8-9 | HR306 | `power_ramp_rate` 10..30000 |
| 10-11 | HR307 | `grid_v_nominal`, if 2500 or less and not in state 4 |
| 12-13 | HR308 | `batt_power_rating.chg_max` = word x 10, 10000..60000; `dis_max` = `chg_max` + 3000 |
| 14-15 | HR309 | `batt_max_charge_current` = word x 100, 1000..8000 |
| 16 | `charge_target_soc_now` | `charge_target_soc_now` 20..100 |
| 17 | HR101 | `grid_import_amps` 1..255 |
| 18-19 | HR312 | `ufr_delay` |
| 20 | HR313 | `force_charge_pct` 0..100 |
| 21 | HR314 | `force_discharge_pct` 0..100 |
| 22-23 | HR315 | `fast_trip_v_low_set` |
| 24-25 | HR316 | `fast_trip_v_high_set` |
| 26-27 | HR102 | `eps_cfg_bits`: bit 6 from bit 6, bit 9 from bit 0 (with conditions), bits 12 and 14 set |
| 28 | `sched_window_out` (HR318 between HR319 and HR320) | `sched_window` |
| 29-30 | `hr4000_value` (HR4000) | `hr4000_value`; I found no reader on the DSP |
| 31 | HR20 `enable_charge_target` | `enable_charge_target` (0 while `batt_cal_stage` is not 0) |
| 32 | HR116 `charge_target_soc` | `charge_target_soc`, at most 100 |
| 33-42 | 0 | not used |

The products in the HR308 and HR309 handlers are 16-bit, so a word above 6553 or 655 wraps
before the clamp (D316 findings-comm item 6); the ARM accepts at most 6000 and 80, so this does
not happen from Modbus.

## DSP to ARM: 0x44

### Header

| Byte | DSP sends | ARM takes |
|---|---|---|
| 1 | `arm_run_mode()`: 0 in states 0 and 1, 1 in states 5 to 8, 3 in state 3, 4 in state 4 | `inv_run_mode` (IR0) |
| 2 | `arm_charge_status()` 0..7 ([operation/battery.md](../operation/battery.md#charge-status)) | `charge_status` (IR14) |
| 3-6 | `arm_fault_word`, 32 bits | `fault_code_word` with the ARM's bits 6-11 (IR39, IR40) |
| 7 | `bms_alarm` low byte (BMS HR20) | `dsp_warning_word` high byte (IR57) |
| 8 | `batt_warning` low byte | `dsp_warning_word` low byte |
| 9 | `run_status_bits` bit 3 (set by the relay test) | `g_200001D2`: 0 keeps the output off (`inv_output_enable_ctrl`) |
| 10-11 | the RAM word at `peek_addr` | `peek_value` (HR25 while HR23 = 0) |
| 12 | the page 0..6 | |

### Page 0

| Byte | DSP sends | ARM field |
|---|---|---|
| 13 | 0 | skipped |
| 14-15 | `pv1_voltage` | `pv1_voltage` |
| 16-17 | `pv1_current` | `pv1_current` |
| 18-21 | `pv1_power` x 0.01, 32 bits | `pv1_power` (replaced by `pv_power_override / 2` above 300) |
| 22-23 | `pv2_voltage` | `pv2_voltage` |
| 24-25 | `pv2_current` | `pv2_current` |
| 26-27 | `inv_countdown_ticks / 50` | `inv_countdown` (IR38) |
| 28 | 1 when `has_grid_fault()` is 0, else 0 | `sys_mode` (IR49) |
| 29-30 | `load_percent` | `dsp_p0_w17` |
| 31-32 | `eps_power` x 0.1 in the EPS states, else `inv_power` x 0.1, signed | `p_inverter_dsp` = value x 10 |
| 33 | `bms_soc_raw` | `batt_soc_dsp` |
| 34-35 | `batt_voltage` x 10 | `batt_voltage`, 0.01 V (IR50) |
| 36-37 | `batt_power` x 100 / `batt_voltage` in states 5 to 8, else 0 | `batt_current` (IR51) |
| 38-39 | `batt_power` x 0.1, x 1.03 when 0 or more, else x 0.97 | `p_battery_dsp` |
| 40-41 | `bms_temp` | `batt_temp_dsp` |
| 42 | 0 | |

### Page 1

| Byte | DSP sends | ARM field |
|---|---|---|
| 13-14 | `eps_current` x 10 in the EPS states, else `grid_current` x 10 | `grid_current` |
| 15-16 | `power_factor` | `pf_dsp` |
| 17-18 | `grid_freq` | `f_ac_dsp` |
| 19-20 | `grid_voltage` | `v_ac_dsp` |
| 21-24 | 0 | skipped |
| 25-26 | `bus_voltage` | `bus_p_voltage` |
| 27-30 | `apparent_power`, 32 bits | `p_apparent_dsp` |
| 31-32 | 0x01 0x3C, the constant 316 | `dsp_version_hr19` (HR19) |
| 33 | 0 | `dsp_p1_b21` |
| 34-35 | `bus_hv_voltage` | `bus_hv_voltage` |
| 36-37 | `eps_voltage` in the EPS states or while `eps_active`; else 0 while CF13 bit 15; else the grid voltage | `eps_voltage` (IR53) |
| 38-39 | `grid_freq_nominal` in the EPS states; else 0 while CF13 bit 15 or `eps_active`; else `grid_freq` | `eps_frequency` (IR54) |
| 40-42 | 0 | |

### Page 2

| Byte | DSP sends | ARM field |
|---|---|---|
| 13 | `pv_input_mode` | `dsp_p2_b01` |
| 14-17 | `pv2_power` x 0.01, 32 bits | `pv2_power` |
| 18-21 | `grid_power_max_pv`, 32 bits | `dsp_hr4_hr5` (HR4, HR5) |
| 22-23 | `bms_capacity` with `battery_type` 1 and HR109 set, else 0 | `batt_capacity_bms` (HR55 reads it when not 0) |
| 24-27 | 0 | `dsp_p2_w12`, `dsp_p2_w14` |
| 28 | CF19 bit 8 (battery full) | `batt_full_flag` |
| 29-32 | `reactive_power`, signed 32 bits | `dsp_p2_d17` |
| 33-34 | `bms_hr0` (BMS HR0) | `bms_chip_version` (HR46) |
| 35-36 | `bms_fw_version` (BMS HR13) | `bms_fw_version` (HR18) |
| 37-42 | 0 | |

### Page 3

| Byte | DSP sends | ARM field |
|---|---|---|
| 13-22 | 0 | `dsp_fault_data2`, `dsp_p3_w03`, `dsp_p3_w05`, `dsp_p3_w07`, `dsp_fault_data1` |
| 23-24 | `rms_d3d2` | `dsp_p3_w11` |
| 25 | 0 | `fault_code` low byte (the ARM overwrites it from its own flags) |
| 26-27 | abs(`dc_injection`) | `dsp_p3_w14` |
| 28-29 | the version, 316 | `dsp_fw_version` (used by the DSP update check) |
| 30 | 0 | `batt_state_dsp` |
| 31 | 0 | `dsp_p3_b19` |
| 32-41 | `bms_serial[0..9]`, low bytes (BMS HR5 to HR9) | `bms_serial_bytes` (HR8 to HR12) |
| 42 | 0 | |

The ARM reads several fields this DSP version always sends as 0 (bytes 13-22, 25, 30, 31 here,
page 2 bytes 24-27, page 1 byte 33). `batt_state_dsp` is one of them, so the ARM's lead-acid rule
"SOC 0 when the DSP's battery state is 3 or more" never fires with D316 (D316 findings-comm).

### Page 4

| Byte | DSP sends | ARM field |
|---|---|---|
| 13 | `auto_test_state`; bytes 14-27 stay 0 while it is 0 | `auto_test_state` |
| 14 | `autotest_step` | `dsp_p4_b02` |
| 15-16 | `autotest_ramp_cnt` | `dsp_p4_w03` |
| 17-18 | `autotest_value` | `dsp_p4_w05` |
| 19-20 | `autotest_trip_ms` | `dsp_p4_w07` |
| 21 | `autotest_result` | `dsp_p4_b09` |
| 22-23 | `autotest_limit` | `dsp_p4_w10` |
| 24-25 | `autotest_trip_value` | `dsp_p4_w12` |
| 26-27 | `autotest_expect_time` | `dsp_p4_w14` |
| 28 | 1 when BMS HR14 (`bms_relay_request`) is 1, else 0; also sets or clears `bms_relay_active` | `dsp_p4_b16`: 5 passes of `task_schedule` start the battery relay |
| 29-30 | `bms_present` | `bms_present` (read only by dead code) |
| 31-32 | `autotest_trip_value2` | `dsp_p4_w19` |
| 33-42 | CF12 to CF16, each low byte first | `dsp_fault_words[0..9]` (IR217 to IR221, bytes swapped); bytes 39-40 (CF15) stored as 0 with HR351 = 1 |

Side effects on the DSP: `autotest_result` 5 clears `auto_test_state` after 3 such pages; 3 or 4
clears it at once and sets CF17 bit 9.

### Page 5

| Byte | DSP sends | ARM field |
|---|---|---|
| 13 | `bms_fc4_ready` | `dsp_bms_page_valid` |
| 14 | `bms_fc4_subpage` 0..4, when 13 is 1 | `dsp_bms_subpage` |
| 15-34 | 20 bytes of `bms_fc4_buf` at sub-page x 20 | `dsp_bms_raw + 2 + 20 x sub-page`; sub-page 4 sets the reply-ready byte for `bms_poll` |
| 35 | `bms_lim_hr26` x 0.01 (A) | `bms_charge_limit_a` (IR62 of slave 0x31), read only when 13 is 1 |
| 36 | `bms_lim_hr27` x 0.01 (A) | `bms_discharge_limit_a` (IR63 of slave 0x31) |
| 37-42 | 0 | |

After sub-page 4 `bms_fc4_ready` goes back to 0, so each BMS reply is sent once.

### Page 6

| Byte | DSP sends | ARM field |
|---|---|---|
| 13-18 | CF17, CF18, CF19, each low byte first | `dsp_fault_words[10..15]` (IR222 to IR224, bytes swapped) |
| 19-42 | 0 | |

## Other traffic on the same port

| Frames | Direction | When |
|---|---|---|
| `0x0A, len, cmd, data, sum16` | ARM to DSP boot loader, and replies | DSP firmware update ([update-packages.md](../system/update-packages.md)) |
| `"BMS", 1, "HED", 1024 bytes, sum16, "E"` and `"BMS", 1, "END", CRC-32` | ARM to BMS through the DSP | battery firmware relay |
| `"OK"` | BMS to ARM through the DSP | after each relay block |
