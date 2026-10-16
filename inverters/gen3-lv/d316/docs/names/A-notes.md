# Range A (0x3E8000 to 0x3EBAB2): names, notes and questions

File names: this is a log of its time, so it keeps the flat file names src/ had then. Step 3
of the readability pass renamed the sources and moved them into subdirectories;
map/filenames.tsv gives each old name its new path.

Range A is two original files. `a_ctl.c` (0x3E8004 to 0x3EAABA) is RTOS task 4 (20 ms) and the
grid-code functions it runs: power limits, reactive power, grid protection, the Italian auto test
and the frequency and voltage responses for storage. `a_armlink.c` (0x3EAABB to 0x3EBAB1) is RTOS
task 5 (60 ms), the UART link to the ARM on SCI-A. `app_ver.c` holds the version word 316 and the
application valid flag 0xAA (both already named).

Frame terms used in A.tsv: the **C frame** is the ARM to DSP frame (0x43, `var_CFC0`, the ARM's
`dsp_tx_frame`). Its frame byte k is offset k-7 from the page number byte in the ARM's
`dsp_build_page`. The **D frame** is the DSP to ARM frame (0x44, `var_DB00`, the ARM's
`dsp_rx_frame`). Its frame byte k is offset k-12 in the ARM's `dsp_parse_page`. I matched every
field by page and offset, and the two sides agree everywhere the ARM has a name.

## Functions

### a_ctl.c

| Address | Proposed name | What it does |
|---|---|---|
| 3E8004 | app_entry | Calls start (already named). |
| 3E8008 | task4 | Runs the 13 functions below every 20 ms (already named). |
| 3E802B | grid_reconnect_check | Sets CF14 bit 7 while grid V or f is outside the reconnect window (HR79 to HR82). Clears it after 100 calls inside the window. |
| 3E8075 | ctl_timers_tick | Counts the uptime to 500, the auto test's elapsed ms and the Italian over-frequency delay (HR300). |
| 3E809D | active_power_rate_limit | Gives HR50 % of the rated power, with a gradient limit that depends on the grid code. |
| 3E8155 | p_limit_by_q | Gives sqrt(S^2 - Q^2). |
| 3E819A | ofr_power_limit | Over-frequency power reduction in state 5 (droop HR123 from HR321, recovery at HR328 after HR303). |
| 3E8575 | freq_response_it | Italy only. In state 6 it raises output on under-frequency and cuts it on over-frequency, and steps the power down above 253.0 V. |
| 3E8881 | grid_support_update | Picks the Italian response or the four generic storage responses, and gives CE1E. |
| 3E88FD | dc_side_power_limit | Derates when the DC bus is above 520.0 V. In charge mode, two PI loops cap PV power and PV current. |
| 3E8A67 | temp_derate_limit | Derates by 8 %/ degrees C above 87.0 degrees C (heatsink or charger), down to a 10 % floor. |
| 3E8ABA | volt_watt_limit | Cuts power at high grid voltage (code 3: from 253 V to 20 % at 260 V). Below 220 V it holds the current constant. |
| 3E8C91 | export_power_limit | HR178 export limit. The allowed output is the inverter power plus the setpoint plus HR26. |
| 3E8D5F | grid_import_current_limit | HR102 bit 6: an import cap of V x HR101 amps. It returns a peak current bound. |
| 3E8DED | soft_start_ramp | Ramps the power ceiling to rated at HR306, or at HR302 after a fault. |
| 3E8EBF | power_limits_update | Takes the minimum of all the limits into CF2C, and writes the current bounds C412 and C414 with interrupts off. |
| 3E8FAF | reactive_power_update | Chooses Q by HR122 mode and adds the filter capacitor term. Writes C408. |
| 3E90D7 / 3E9183 / 3E9401 / 3E957B / 3E95C1 | q_fixed_pf / q_cosphi_p / q_pf_curve / q_fixed_percent / q_of_u_fixed | The Q modes. |
| 3E9290 | pf_curve_interp | Interpolates PF codes, which are encoded as 0..2000 and 18000..20000 on either side of 1.0. |
| 3E9688 / 3E96C4 | autotest_ramp / autotest_run | The CEI 0-21 protection auto test: 8 steps that ramp V or f and time the trip. The result goes to the ARM on D frame page 4. |
| 3E985C | grid_v_10min_update | 10-minute moving mean of the grid voltage (200 x 3 s). |
| 3E98D7 | dci_control | DC injection: a correction to C42A, and a trip (CF17 bit 0). |
| 3E99A9 | rocof_detect | Sum of df over 25 samples, which flags when it is above 1.10 Hz. The type name a_osc is wrong: this is a RoCoF check. |
| 3E99F8 | grid_freq_protect | Two-stage over- and under-frequency trips (CF12 bit 15), with the RoCoF check for some codes. |
| 3E9BC3 / 3E9C82 | grid_volt_trip_std / grid_volt_protect | Two-stage voltage trips and the 10-minute mean (CF12 bit 12). The Italian path also serves the auto test. |
| 3E9E0E | grid_v_fast_trip | Instant trip at HR315/HR316. It calls sub_3EFF2B with interrupts off. |
| 3E9EA4 / 3E9F03 | q_of_u_lockin / q_of_u_curve | Q(U): lock-in by power (HR142/143), and the curve HR138..HR141. |
| 3E9FF1 | ufr_power_limit | Under-frequency power increase in state 5. |
| 3EA161 / 3EA3E8 / 3EA5AC / 3EA6A7 | ofr_/ufr_/ovr_/uvr_storage_response | The state 6 storage responses. They write CE20, CE22, CE24 and CE26. |
| 3EA792 / 3EA846 / 3EAA22 | pf_of_u_adjust / q_pt1_response / q_restore_response | Grid code 7 only: PF by voltage, and PT1 smoothing of Q (time constant HR301). |
| 3EA951 | export_overload_check | Export over the HR26 limit for too long sets CF14 bit 5. |

### a_armlink.c

| Address | Proposed name | What it does |
|---|---|---|
| 3EAABB | task5_arm_link | Assembles the C frames and checks the CRC. 300 bad periods set CF18 bit 2 (link lost). Every 4th tick it builds a D frame and sends it with sci_send (already named). |
| 3EABE1 | arm_fault_word_build | Builds the 32-bit fault word in D frame bytes 3-6 from CF17 and CF18. |
| 3EAC1D | arm_run_mode | D frame byte 1 (ARM inv_run_mode), from op_state. |
| 3EAC2F | arm_charge_status | D frame byte 2 (ARM charge_status), from the faults, the converter state and the SoC. |
| 3EAC73 | arm_link_frame | D frame pages 0-6 (already named). |
| 3EB0E8 | arm_frame_parse | C frame header bytes 1-7, then the page handler for every byte. |
| 3EB17E | arm_run_command | The run command byte (events 6 and 4). Stores it in D8EB through sub_3EFBCF. |
| 3EB1A9..3EB977 | arm_page0..5 | Already named. Each field is listed in A.tsv with its HR number. |

## What the ARM cross-match shows

- **D frame page 4 offsets 21-30 and page 6 offsets 1-6 are the DSP's fault words CF12 to CF19.**
  The ARM calls them `bms_info_bytes` and serves them as IR217-224, inferring a BMS version or
  serial. They are not BMS data.
- **D frame page 3 offsets 20-29 carry the BMS serial** (`var_DAD0`, from BMS HR5-HR9 on the full
  poll). The ARM calls them `dsp_version_bytes` (HR8-12).
- **D frame page 5 bytes 35 and 36 are the BMS current limits** D504 and D505 (HR26 and HR27, in A).
  The ARM calls them `bms_module_count` and `bms_module_info`. The README already says this.
- **D8E0 is HR55, the battery capacity in Ah** (C frame page 0 offset 28), or the BMS's HR11 when
  BMS mode is on and 50 replies have arrived. The README's "maximum current?" guess is wrong. So
  the cap in `FUN_003ECA0A` is capacity x HR111/HR112 % + 150, which is a C-rate cap.
- `dsp_p2_d17` (ARM, unnamed) is the reactive power D410. `dsp_p0_w17` is the load in % (D484).
  `dsp_p3_w14` is |DC injection| (D3FA). `dsp_p2_b01` is the PV input mode (D8E9).
- The ARM's `grid_country_byte_a/b` (C frame page 2 offset 16, page 3 offset 25) are DCI trip
  times on the DSP (D835, D836).
- D34A, the grid code, comes from the high byte of HR2, which the ARM names `inverter_module_l`.
  The low byte of HR2 is the model's power class: 36, 46 or 50 -> D8F1 = 3, 4 or 5.
- C frame page 0 offset 21 (`g_200001D7`) only clears D805. D805 picks the first-start time
  (HR61) over the restart delay (HR62).

## Open questions

- What op_state 5 and op_state 6 mean. Both are on-grid run states. The storage responses run in 6
  and the generator-style responses run in 5, so 6 may be "with battery". The state machine is
  range B's.
- The grid code numbers. 4 is Italy (CEI 0-21 auto test). 3 and 19 look like AS4777 regions, and
  20/21 -> 3 fits the ARM's Australian DRM handling. 7 is a code with PT1 Q smoothing. The others
  are not identified.
- D7C0, D7C1 and D7C2 bit meanings beyond those in A.tsv. The C frame sources are HR102, HR115,
  HR124, HR125, HR176, HR177, HR178 and HR179. The ARM gives no meanings for these.
- D3D2 (dsp_p3_w11), D3F6, D854, D8CA (HR101 as an import current in A) and the HR1 bits D8D5..D8DA
  are only partly understood.
- `var_CDD4` is the first word of struct a_cdd4. grid_v_fast_trip writes it (250) and nothing
  reads it. It may not belong with the RoCoF block.

## Possible bugs and oddities

- **The PF(P) curve (HR122 mode 3) can never act from the ARM.** Its power points D846..D849 have
  the default 255. The page 3 handler throws away the HR130/132/134/136 words that would set them,
  so `D846 <= 100` is always false and the mode gives Q = 0.
- `arm_frame_parse` sets `D806 = 438` once for every frame byte. Any other value of D806 (the Q(U)
  maximum, settings.c default 438) is overwritten at once.
- task5 calls `sub_3F1487` without a declaration. That function is `sci_send(port, len, buf, src)`
  in sci.c. The call `sub_3F1487(0, &var_CF80, p, 45)` works only because the C28x ABI puts ints in
  AL/AH and pointers in XAR4/XAR5. a_funcs.h declares a different order for `sci_send` (port, ring,
  data, n). It matches by the same accident. CF80 is the port 0 transmit buffer.
- sci.c defines `sci_flag` at 0xCF40 and never uses it. a_armlink.c uses that word as the byte
  count of the incoming C frame (proposed `arm_rx_len`). The two names should be reconciled.
- `sub_3E985C` tests grid code 11 twice in its list. `sub_3E9EA4` tests code 6 twice.
- `sub_3E9C82`, Italian under-voltage stage 2 branch: `if (var_CD19 == 6) { ...; if (var_CD19 == 1 ||
  ... == 6) var_CDE3 = 0; }`. The inner test is always true.
- `q_restore_response` sets `var_CD3C = 0` and then uses it as the target, so the PT1 always decays
  to 0.
- `var_CE28` (the restore point of the storage responses) is written only by control_b.c (-D4FA).
  If that path does not run, the responses ramp back to 0.
- D frame page 1 sends a fixed version of 1, 60 (316) as "HR19". Page 3 sends `version` again at
  offset 16.
