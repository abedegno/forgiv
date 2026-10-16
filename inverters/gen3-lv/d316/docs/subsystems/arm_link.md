# The ARM link

Files: `src/app/comm/arm_link.c`, `src/app/comm/arm_page_write.c`, `src/app/comm/crc16.c`,
`src/include/arm_link.h`. The serial port is in `src/drivers/sci.c` (drivers.md).

The ARM (the A316 firmware, decompiled in [../../../a316](../../../a316/README.md)) is the
inverter's main controller. It talks to the DSP over a UART: SCI-A on the DSP (GPIO28 RX, GPIO29 TX), UART4 on the ARM,
9600 baud, 8N1. The ARM sends settings and commands. The DSP sends back measurements, faults and
the data it reads from the battery's BMS, which only the DSP can reach.

The names in brackets below are the A316 decomp's (`src/app/comm/dsp_link.c` and
`docs/battery.md` there). They agree with the corrected names `bms_charge_limit_a`,
`bms_discharge_limit_a`, `dsp_fault_words` and `bms_serial_bytes`.

## Timing

- The ARM sends a 0x43 frame every 60 ms (task_dsp_tx), with the settings pages 0..7 in turn, so
  each page comes every 480 ms.
- The DSP's task 5 (task5_arm_link) wakes every 60 ms and on EV5_FRAME. It sends a 0x44 frame on
  every 4th tick, so every 240 ms, with the telemetry pages 0..6 in turn (each page every 1.68 s).
- sci_rx posts EV5_FRAME when 45 bytes that start with 0x43 are in the receive ring.

## Frame format

Both directions use 45-byte frames. Each byte travels in its own 16-bit word in RAM. Bytes 43-44
hold the CRC-16/MODBUS of bytes 0..42, high byte first (the reverse of Modbus RTU's wire order;
both ends do the same). CRC16 in crc16.c returns the standard value.

### ARM to DSP: 0x43 ('C', ARM_FRAME_FROM_ARM)

| Byte | Meaning | A316 source |
|---|---|---|
| 0 | 0x43 | |
| 1 | run command 0..10, to arm_run_command: 4 = update (the 6th frame with it posts EV0_UPDATE), 10 = stop (EV0_STOP) | inv_run_mode, or 0x0A for grid code 4 with HR53 = 0 |
| 2 | ARM fault flags: bit 2 sets CF18 bit 8, bit 4 sets CF18 bit 9 | fault_flags >> 4 |
| 3 | active_power_rate %, ignored at 105 and above; aging_test_mode 2 forces 70, 1 forces 85 (PV2 above 3000) or 41 | HR50, capped by the DRM limit |
| 4 | batt_state_cmd 0..2 (0 idle, 1 charge, 2 discharge) | battery state |
| 5-6 | arm_batt_setpoint = signed word x 10 | HR495 when HR494 = 1, else -batt_power_calc / 10 |
| 7 | settings page 0..7 (arm_page_no; above 7 counts as 7) | dsp_tx_page |
| 8-42 | the page | dsp_build_page |
| 43-44 | CRC16 | |

arm_frame_parse calls the page handler for every byte index 1..42; each handler picks its fields
by index. The A316's offsets in dsp_build_page count from byte 7, so A316 offset k is byte k + 7.

### DSP to ARM: 0x44 ('D', ARM_FRAME_TO_ARM)

| Byte | Meaning | A316 name |
|---|---|---|
| 0 | 0x44 | |
| 1 | arm_run_mode(): 0 idle or starting, 1 running, 3 fault, 4 update | run mode |
| 2 | arm_charge_status(): 7 battery fault (CF15 bit 1), 6 any batt_warning, 0 no converter mode, 5 discharging, 4 SOC 98+, 3 SOC above 90, 2 SOC above 4, else 1 | charge_status |
| 3-6 | arm_fault_word, high byte first | 32-bit fault word |
| 7 | bms_alarm, low byte | warning word (IR57), high byte |
| 8 | batt_warning, low byte | warning word, low byte |
| 9 | run_status_bits bit 3 | g_200001D2 (0 keeps the output off) |
| 10-11 | the RAM word at peek_addr (0xC000..0xE000) | peek_value (HR25) |
| 12 | telemetry page 0..6 | |
| 13-42 | the page | dsp_parse_page |
| 43-44 | CRC16 | |

arm_fault_word: CF17 bit 11 to bit 17, CF18 bit 6 (calibration) to 19, CF18 bit 2 (ARM link lost)
to 21, CF17 bit 3 (bus over-voltage) to 22, CF17 bit 1 (insulation) to 25, CF17 bit 0 (DC
injection) to 27, CF17 bit 4 (PV over-voltage) to 28, CF17 bit 7 (heatsink over-temperature) to 31.

## Telemetry pages (DSP to ARM)

Bytes not listed are 0. Words are high byte first. The A316 offset is byte - 12.

| Page | Byte | Field | A316 name |
|---|---|---|---|
| 0 | 13 | 0 | (skipped) |
| 0 | 14-15 | pv1_voltage | pv1_voltage |
| 0 | 16-17 | pv1_current | pv1_current |
| 0 | 18-21 | pv1_power x 0.01, 32 bits | pv1_power |
| 0 | 22-23 | pv2_voltage | pv2_voltage |
| 0 | 24-25 | pv2_current | pv2_current |
| 0 | 26-27 | inv_countdown_ticks / 50 | inv_countdown |
| 0 | 28 | 1 when has_grid_fault() is 0 | sys_mode |
| 0 | 29-30 | load_percent | dsp_p0_w17 |
| 0 | 31-32 | eps_power x 0.1 in the EPS states, else inv_power x 0.1 | inverter power / 10 |
| 0 | 33 | bms_soc_raw | batt_soc_dsp |
| 0 | 34-35 | batt_voltage x 10 | batt_voltage (0.01 V) |
| 0 | 36-37 | batt_power x 100 / batt_voltage in states 5..8, else 0 | batt_current |
| 0 | 38-39 | batt_power x 0.1, x 1.03 when 0 or more, else x 0.97 | p_battery_dsp |
| 0 | 40-41 | bms_temp | batt_temp_dsp |
| 1 | 13-14 | eps_current x 10 in the EPS states, else grid_current x 10 | grid_current |
| 1 | 15-16 | power_factor | pf_dsp |
| 1 | 17-18 | grid_freq | f_ac_dsp |
| 1 | 19-20 | grid_voltage | v_ac_dsp |
| 1 | 21-24 | 0 | (skipped) |
| 1 | 25-26 | bus_voltage | bus_p_voltage |
| 1 | 27-30 | apparent_power, 32 bits | p_apparent_dsp |
| 1 | 31-32 | 0x01 0x3C, the constant 316 | dsp_version_hr19 |
| 1 | 33 | 0 | dsp_p1_b21 |
| 1 | 34-35 | bus_hv_voltage | bus_hv_voltage |
| 1 | 36-37 | eps_voltage (EPS states or eps_active), else 0 with fault_grid2 bit 15, else grid_voltage | eps_voltage |
| 1 | 38-39 | grid_freq_nominal (EPS states), else 0 with fault_grid2 bit 15 or eps_active, else grid_freq | eps_frequency |
| 2 | 13 | pv_input_mode | dsp_p2_b01 |
| 2 | 14-17 | pv2_power x 0.01, 32 bits | pv2_power |
| 2 | 18-21 | grid_power_max_pv, 32 bits | dsp_hr4_hr5 |
| 2 | 22-23 | bms_capacity when battery_type is 1 and enable_bms_read, else 0 | batt_capacity_bms |
| 2 | 24-27 | 0 | dsp_p2_w12, dsp_p2_w14 |
| 2 | 28 | batt_blocks bit 8 (battery full) | batt_full_flag |
| 2 | 29-32 | reactive_power, signed 32 bits | dsp_p2_d17 |
| 2 | 33-34 | bms_hr0 | bms_chip_version (BMS HR46) |
| 2 | 35-36 | bms_fw_version | bms_fw_version (BMS HR18) |
| 3 | 13-22 | 0 | dsp_fault_data2, dsp_p3_w03, w05, w07, dsp_fault_data1 |
| 3 | 23-24 | rms_d3d2 | dsp_p3_w11 |
| 3 | 25 | 0 | fault_code |
| 3 | 26-27 | abs(dc_injection) | dsp_p3_w14 |
| 3 | 28-29 | version (316) | dsp_fw_version |
| 3 | 30 | 0 | batt_state_dsp |
| 3 | 31 | 0 | dsp_p3_b19 |
| 3 | 32-41 | bms_serial[0..9], low bytes | bms_serial_bytes (served as HR8-12) |
| 4 | 13 | auto_test_state; 14-27 stay 0 while it is 0 | auto_test_state |
| 4 | 14 | autotest_step | dsp_p4_b02 |
| 4 | 15-16 | autotest_ramp_cnt | dsp_p4_w03 |
| 4 | 17-18 | autotest_value | dsp_p4_w05 |
| 4 | 19-20 | autotest_trip_ms | dsp_p4_w07 |
| 4 | 21 | autotest_result | dsp_p4_b09 |
| 4 | 22-23 | autotest_limit | dsp_p4_w10 |
| 4 | 24-25 | autotest_trip_value | dsp_p4_w12 |
| 4 | 26-27 | autotest_expect_time | dsp_p4_w14 |
| 4 | 28 | 1 when bms_relay_request (0xD8E5, the BMS's HR14) is 1; sets or clears bms_relay_active | dsp_p4_b16 (the battery relay may start) |
| 4 | 29-30 | bms_present | bms_present |
| 4 | 31-32 | autotest_trip_value2 | dsp_p4_w19 |
| 4 | 33-42 | fault_grid1, fault_grid2, fault_grid_eps, fault_batt, fault_pv (CF12..CF16), low byte first | dsp_fault_words bytes 0-9 |
| 5 | 13 | bms_fc4_ready | dsp_bms_page_valid |
| 5 | 14 | bms_fc4_subpage 0..4 (only when 13 is 1) | dsp_bms_subpage |
| 5 | 15-34 | 20 bytes of bms_fc4_buf at sub-page x 20 | dsp_bms_raw |
| 5 | 35 | bms_lim_hr26 x 0.01 | bms_charge_limit_a |
| 5 | 36 | bms_lim_hr27 x 0.01 | bms_discharge_limit_a |
| 6 | 13-18 | fault_stop1, fault_stop2, batt_blocks (CF17..CF19), low byte first | dsp_fault_words bytes 10-15 |

Page 4 side effects: autotest_result 5 clears auto_test_state after 3 such pages; 3 or 4 clears
it at once and sets CF17 bit 9. Page 5 side effect: after sub-page 4, bms_fc4_ready goes back to
0, so each BMS reply goes out once.

## Settings pages (ARM to DSP)

Fields not listed are not used by the DSP. The A316 offset is byte - 7.

| Page | Byte | DSP field (clamp) | A316 source |
|---|---|---|---|
| 0 | 8 | grid_code 0..28; 20 and 21 become 3 | HR2 high byte |
| 0 | 9 | 50, 46, 36 give model_power_code 5, 4, 3 and clear CF17 bit 10; other values set it | HR2 low byte |
| 0 | 10-11 | inverter_start_time | HR61 |
| 0 | 12-13 | pv_start_voltage 1200..1500 | HR60 (1550 during HR175) |
| 0 | 14 | grid_func_flags bits 3 (1), 12 (2), both (3), none (else) | HR125 |
| 0 | 15-16 | inverted bits: iso_check_enable, rcmu_enable, dci_comp_enable, hr1_check3, rcmu_step_check_off | HR1 |
| 0 | 17 | not used | HR50 |
| 0 | 18 | reactive_power_rate, below 61 | HR51 |
| 0 | 19-20 | power_factor_set | HR52 |
| 0 | 21 | batt_min_reserve | HR114 |
| 0 | 22 | charge_active | charge_active |
| 0 | 23 | discharge_active | discharge_active |
| 0 | 24 | battery_power_mode | HR27 |
| 0 | 25 | inverter_state | HR53 |
| 0 | 26 | not used | g_200001B0 |
| 0 | 27 | q_mode | HR122 |
| 0 | 28 | any value clears pv_bus_mode | g_200001D7 |
| 0 | 29-30 | inverter_restart_delay | HR62 |
| 0 | 31-33 | not used | 0, HR33, settings +0x74 |
| 0 | 34 | battery_type | HR54 |
| 0 | 35-36 | battery_capacity; bms_capacity instead with a BMS that has answered 50 times | HR55 |
| 0 | 37-38 | charger_temp | charger |
| 0 | 39-40 | heatsink_temp | heatsink |
| 0 | 41-42 | batt_temp | battery temperature |
| 1 | 8-11 | p_eps_sample, 32 bits | p_eps_sample |
| 1 | 12-13 | batt_v_low_limit = word x 0.1 (x 0.5 in calibration request, x 0.86 in calibration stage), 200..480 | HR97 |
| 1 | 14-15 | batt_v_high_limit = word x 0.1 (x 1.05 in calibration stage), 540..630 | HR98 |
| 1 | 16-17 | v_ac_high_out 2300..3150 | HR64 |
| 1 | 18-19 | v_ac_low_out 0..2300 | HR63 |
| 1 | 20-21 | f_ac_high_out | HR66 |
| 1 | 22-23 | f_ac_low_out | HR65 |
| 1 | 24-25 | v_ac_high_in 2300..3150 | HR72 |
| 1 | 26-27 | v_ac_low_in 0..2300 | HR71 |
| 1 | 28-29 | f_ac_high_in | HR74 |
| 1 | 30-31 | f_ac_low_in | HR73 |
| 1 | 32-33 | v_ac_high_c | HR80 |
| 1 | 34-35 | v_ac_low_c | HR79 |
| 1 | 36-37 | v_ac_high_out_time | HR68 |
| 1 | 38-39 | v_ac_low_out_time | HR67 |
| 1 | 40-41 | f_ac_high_out_time | HR70 |
| 1 | 42 | auto_test_state, only while it is 0 | auto test state |
| 2 | 8-9 | f_ac_low_out_time | HR69 |
| 2 | 10-11 | v_ac_high_in_time | HR76 |
| 2 | 12-13 | v_ac_low_in_time | HR75 |
| 2 | 14-15 | f_ac_high_in_time | HR78 |
| 2 | 16-17 | f_ac_low_in_time | HR77 |
| 2 | 18-19 | f_ac_high_c | HR82 |
| 2 | 20-21 | f_ac_low_c | HR81 |
| 2 | 22 | cfg_changed_flags | cfg_changed_flags |
| 2 | 23 | dci_trip_time | grid_country_byte_a |
| 2 | 24 | not used | HR128 |
| 2 | 25 | ofr_droop 1..100 | HR123 |
| 2 | 26-27 | v_10_min_protection 2300..2900 | HR83 |
| 2 | 28 | freq_stage_sel | HR121 |
| 2 | 29 | qu_lock_out_p | HR143 |
| 2 | 30 | qu_lock_in_p | HR142 |
| 2 | 31-32 | pfp_lock_in_v | HR144 |
| 2 | 33-34 | pfp_lock_out_v | HR145 |
| 2 | 35-36 | grid_port_max_power = word x 10, at least 1200 | HR26 |
| 2 | 37 | 0 = 50 Hz, 1 = 60 Hz (below OP_UPDATE): grid_freq_nominal, grid_omega_step, eps_cycle_len | HR28 |
| 2 | 38 | batt_cal_stage | HR29 |
| 2 | 39 | aging_test_enable; 0 to 1 posts EV0_STOP | HR332 |
| 2 | 40 | not used | ct_cycle_count |
| 2 | 41-42 | ofr_recover_freq 5000..5400 (6000..6400 at 60 Hz) | HR328 |
| 3 | 8-23 | pf_curve_pf1..pf4 at 10, 14, 18, 22; 8, 12, 16, 20 not used | HR130..HR137 |
| 3 | 24-25 | qu_v_high1 | HR138 |
| 3 | 26-27 | qu_v_high2 | HR139 |
| 3 | 28-29 | qu_v_low1 | HR140 |
| 3 | 30-31 | qu_v_low2 | HR141 |
| 3 | 32 | dci_trip_time_it | grid_country_byte_b |
| 3 | 33-34 | ofr_start_freq 5000..5400 (6000..6400 at 60 Hz) | HR321 |
| 3 | 35-36 | peek_addr, clamped to 0xC000..0xE000 | HR24 |
| 3 | 37-38 | not used | 0 |
| 3 | 39-40 | ufr_start_freq 4600..5000 (5600..6000 at 60 Hz) | HR326 |
| 3 | 41 | ufr_droop 1..100 | HR327 |
| 4 | 8 | not used | ct_cycle_count |
| 4 | 9 | grid_func_flags bit 4 | HR124 |
| 4 | 10-11 | not used | g_200001A4 |
| 4 | 12-13 | dci_limit 30..1100 (1000 with rcmu_step_check_off) | settings +0x69 |
| 4 | 14-15 | dci_limit_b, the same | settings +0x6B |
| 4 | 16 | batt_cal_request and low_batt_wake (see arm_page4) | HR175 |
| 4 | 17 | enable_bms_read | HR109 |
| 4 | 18 | grid_func_flags bits 15, 6, 8 from bits 0, 1, 2 | HR176 |
| 4 | 19 | battery_charge_limit 1..100 | HR111 |
| 4 | 20 | battery_discharge_limit 1..100 | HR112 |
| 4 | 21 | discharge_min_soc 1..100 | discharge_min_soc_now |
| 4 | 22 | grid_func_flags bit 11 | HR115 |
| 4 | 23-24 | not used | pv_power_override |
| 4 | 25 | eps_cfg_bits bit 10 | HR177 |
| 4 | 26 | bms_update_pending | bms_update_pending |
| 4 | 27 | hr200_request | HR200 |
| 4 | 28 | batt_blocks bit 2 | meter link state |
| 5 | 8-9 | cal_pv1_v 870..1178 | calibration gains |
| 5 | 12-13 | cal_pv1_i and cal_pv1_p 870..1178 | |
| 5 | 16-17 | cal_pv2_v 870..1178 | |
| 5 | 20-21 | cal_pv2_i and cal_pv2_p 870..1178 | |
| 5 | 24-25 | cal_bus_v 870..1178 | |
| 5 | 28-29 | cal_grid_v 870..1178 | |
| 5 | 42 | hr201_request | HR201 |
| 6 | 12-13 | cal_power 870..1178 | calibration gains 6-8 (bytes 8-19) |
| 6 | 16-17 | cal_batt_v | |
| 6 | 20 | 1: copy bytes 21-26 to arm_bms_request and set arm_bms_request_pending | BMS request flag |
| 6 | 21-26 | the ARM's own BMS request (device, function 4, start, count) | staged by bms_poll |
| 6 | 28-29 | cal_d819 870..1178 | |
| 6 | 30-31 | ofr_delay | HR300 |
| 6 | 32-33 | q_pt1_tau | HR301 |
| 6 | 34-35 | power_ramp_rate_reconnect 10..30000 | HR302 |
| 6 | 36-37 | ofr_recover_delay | HR303 |
| 6 | 38 | grid_func_flags bit 0 (0 or 1 only) | HR178 |
| 6 | 39 | grid_func_flags bit 2 (0 or 1 only) | HR179 |
| 6 | 40 | pv_mode_cfg 0..4 | HR305 |
| 7 | 8-9 | power_ramp_rate 10..30000 | HR306 |
| 7 | 10-11 | grid_v_nominal, if 2500 or less and below OP_UPDATE | HR307 |
| 7 | 12-13 | batt_power_rating.chg_max = word x 10, 10000..60000; dis_max = chg_max + 3000 | HR308 |
| 7 | 14-15 | batt_max_charge_current = word x 100, 1000..8000 | HR309 |
| 7 | 16 | charge_target_soc_now 20..100 | charge target SOC now |
| 7 | 17 | grid_import_amps 1..255 | HR101 |
| 7 | 18-19 | ufr_delay | HR312 |
| 7 | 20 | force_charge_pct 0..100 | HR313 |
| 7 | 21 | force_discharge_pct 0..100 | HR314 |
| 7 | 22-23 | fast_trip_v_low_set | HR315 |
| 7 | 24-25 | fast_trip_v_high_set | HR316 |
| 7 | 26-27 | eps_cfg_bits: bit 6 from bit 6, bit 9 from bit 0 (with conditions), bits 12 and 14 set | HR102 |
| 7 | 28 | sched_window | sched_window_out |
| 7 | 29-30 | hr4000_value | HR4000 |
| 7 | 31 | enable_charge_target (0 during batt_cal_stage) | HR20 |
| 7 | 32 | charge_target_soc, at most 100 | HR116 |

## Link health

- A good frame clears arm_rx_fail_cnt. After 40 good frames CF18 bit 2 (FS2_ARM_LINK_LOST, also
  set at start-up) clears.
- A bad CRC counts in arm_crc_err_cnt (over 5 empty the ring) and arm_rx_fail_cnt.
- A wake-up without a frame counts in arm_rx_idle_cnt (over 5 empty the ring) and arm_rx_fail_cnt.
- At 300 in arm_rx_fail_cnt, CF18 bit 2 is set again (cleared instead while the BMS relay runs).
- In OP_UPDATE the DSP sends one more frame and then stops sending for good, so the boot loader
  can take SCI-A (boot.md).

## The battery relay

When the BMS's HR14 (bms_relay_request at 0xD8E5) reads 1, page 4 byte 28 tells the ARM, and
bms_relay_active is set. From then on task 5 sends no frames and sci_rx joins SCI-A and SCI-B so
that the ARM can send a battery firmware image straight to the BMS (drivers.md).

## Open points

See docs/findings-comm.md. The main ones: sci_rx clears task 5's pending events on every byte
(the word called arm_rx_block is os_tcb[5].events), and the ARM reads several page fields that
this DSP version always sends as 0.
