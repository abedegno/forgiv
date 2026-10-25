# Remaining-name investigation, firmware 319

This pass follows producers and consumers rather than assigning names from a field's shape.
Names marked **observed** describe code or a linked symbol. **Likely** means a protocol-map
inference. No algorithm, type, packing, volatile qualifier or acceptance rule is changed.

## Battery reply: current and cell extrema

Observed: `bms_t.f35` is now `pack_current_raw`. In the battery 3020
[reply builder](../../../../batteries/lv/bms-3020/src/app/comm/register_protocol.c), the
four-byte field immediately after the pack voltages is made from `measurement_current` using
signed float-to-int conversion. The inverter's [pack decoder](../src/app/control/measure.c)
reads those same four bytes at offsets 13-16 of its block-21 reply. Its
[register readback](../src/app/comm/modbus_slave.c) divides the signed result by ten for IR95.
The absolute physical unit is not independently calibrated here; the name deliberately says raw.

Observed: `f4F`/`f51` are now `maximum_cell_voltage`/`minimum_cell_voltage`. The battery's
block-40 producer places those measurements after the sixteen cell voltages and two temperature
extrema; the inverter decodes that exact order. `f00` is now `pack_status_word`; bit zero
participates in the online-pack count. The meanings of its other bits remain unestablished.

Likely firmware defect: the inverter accepts the cell extrema only below 700, although its
individual cell-voltage acceptance is 2200..3700. This rejects normal millivolt-scale extrema.
It can still store zero or other low values; the previous comment claiming it never stores
anything was too strong. The range tests are preserved.

## Extended meter: likely SDM-compatible fields

The request is FC4, start zero, 80 registers; floats are big-endian. Thus a response byte
offset `b` corresponds to register `(b - 3) / 2`. The surrounding voltage, current, active
power, power-factor, frequency and import/export slots agree with the manufacturer's
[SDM630 protocol, section 1.2.1](https://www.eastroneurope.com/images/uploads/products/protocol/SDM630_MODBUS_Protocol.pdf).
That supports the following **likely** names; it does not establish the attached meter model.

| Former member | Wire register | New member | Stored scale |
| --- | --- | --- | --- |
| `extended_4a` | `0x24` | `phase_angle_l1_x10` | degrees x10 |
| `extended_4e` | `0x26` | `phase_angle_l2_x10` | degrees x10 |
| `extended_52` | `0x28` | `phase_angle_l3_x10` | degrees x10 |
| `extended_5a` | `0x2A` | `average_voltage_x10` | volts x10 |
| `extended_62` | `0x2E` | `average_current_x10` | amps x10 |
| `extended_66` | `0x30` | `sum_current_x100` | amps x100 |
| `extended_76` | `0x4C` | `reactive_import_energy_x10` | kvarh x10 |
| `extended_7a` | `0x4E` | `reactive_export_energy_x10` | kvarh x10 |

The code's scales differ between average and sum current and are preserved. These reactive
energy fields are not swapped by the direction flag, unlike the active-energy counters.

Likely labeling/routing discrepancy: in this map the fields called `q_*` receive apparent
power, while `s_*` receive reactive power, for phases and totals. The code also applies
direction to the latter group. This could be inherited naming debt or a firmware routing
quirk; a supported-meter trace is needed to distinguish them. Existing fields and routing
are retained rather than silently correcting behavior.

`extended_5e` and `extended_6a` read undocumented holes `0x2C`/`0x32` in that map;
`extended_56` has no named decoder assignment. The integer FC3 block at `0xF100`, trailing
legacy fields, and the physical identity of the supported meter remain unresolved.

## ARM state and frame fields

The page-relative offsets in `dsp_build_page` start at frame byte 7. Confusing them with
absolute frame offsets gives the wrong receiving field. Comparing with DSP `arm_page0` and
`arm_page4` resolves these roles:

| Old name | New name | Observed evidence and limit |
| --- | --- | --- |
| `g_200001B0` | `dsp_ignored_mode_byte` | Page 0 byte 26, explicitly ignored by the DSP. ARM sets 255 at startup, 2 while running, and has a 100-to-1 timeout branch. Original command intent unknown. |
| `g_200001D7` | `dsp_pv_bus_mode_byte` | Page 0 byte 28. The receiver forces `pv_bus_mode` to zero without using the value. |
| `g_200001A4` | `dsp_reserved_page4_word` | Page 4 bytes 10-11. Neither index has a receiver case. Only a zero initialization was found. |
| `g_20000266` | `eeprom_transaction_state` | Pending 1 becomes 2 on save success, 3 after retry exhaustion. No named assignment of pending 1 was found. |
| `g_2000018C` | `settings_commit_flags` | Initialized zero; bit 1 is set following a successful settings save. No named reader found. |
| `g_200003C4` | `load_power_unclamped` | Computed from inverter/CT or inverter/meter power before load clamping. No named reader found. |
| `g_20000152` | `modbus_ir243_value` | Copied into slave-0 IR243; no named application writer found. |

Fifteen `g_` names remain. These are the named-source accesses found during this investigation:

| Globals | Observed accesses |
| --- | --- |
| `g_200000D0` | Cleared by monitor task; no named reader |
| `g_2000019E`, `g_200001AF`, `g_200001D6` | Cleared at board initialization; no other named use |
| `g_200001B2` | Cleared at initialization and outside running mode; no named reader |
| `g_2000022C` | Cleared on DSP receive handling; no named reader |
| `g_20000261`..`g_20000265`, `g_20000267` | Cleared at initialization; no other named use |
| `g_2000028A`, `g_2000028B`, `g_20000297` | Cleared at initialization; no other named use |

These observations do not exclude bulk clears, overlapping memory views or external debugger
access. They provide no evidence for a physical setting name, so address names are retained.

## DSP aliases and runtime hooks

Six apparent DSP unknowns were already understood through a second RAM view. Shared struct
members now agree with that evidence without colliding with accessor macros:

| Address | Former member | Recovered member / evidence |
| --- | --- | --- |
| `D8D1` | `cfg_d8d1` | `charge_active_setting`; `charge_active` RAM alias, received on page 0 byte 22 |
| `D8D2` | `cfg_d8d2` | `discharge_active_setting`; corresponding alias, page 0 byte 23 |
| `D8A4` | `cfg_d8a4` | `grid_voltage_87_percent`; existing RAM alias |
| `D8B6` | `cfg_d8b6` | `grid_low_voltage_derate_threshold`; grid voltage/watt limiter consumer |
| `D8BC` | `cfg_d8bc` | `export_target_unfiltered`; export limiter computes and clamps it |
| `D8BE` | `cfg_d8be` | `export_target_filtered`; first-order update with coefficient 0.03 |

The remaining unused/padded DSP configuration members have no established physical meaning.
`cfg_d8c4` has a preset value of 10, but that alone does not identify its purpose.

The licensed ARM link symbol table confirms `heapauxi.o` exports:

```text
080370E9  __use_two_region_memory
080370EB  __rt_heap_escrow$2region
080370ED  __rt_heap_expand$2region
```

These are Thumb addresses. The latter two C stand-ins are now
`keil_heap_escrow_two_region` and `keil_heap_expand_two_region`; their native even addresses
are `080370EA` and `080370EC` respectively. The dollar
suffix is documented rather than used as a C identifier. All three remain empty functions.

## What would resolve the rest

The device at address `0x11` receives an export-related power command and returns registers
which are relayed to Modbus. No model string or independently identified register specification
was found. Its role suggests a controllable power device; a specific inverter or meter model
would be a guess. A bus capture or manufacturer register map would resolve this more reliably.
The same applies to undocumented meter holes, the FC3 configuration block and unused flags.

Acceptance remains complete-unit matching, identical full images and zero blobs. Updated source
hashes are in [rebuild.json](rebuild.json) and the [DSP proof](../../d319/docs/rebuild.json).
