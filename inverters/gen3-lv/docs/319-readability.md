# Firmware 319 source and build guide

Both 319 images rebuild exactly. The source layout and names now describe the application,
with compiler-sensitive forms documented where they occur. This pass preserves the recovered
program, including dead code and questionable behavior; it does not change firmware behavior.

## Reading the source

The [ARM tree](../a319/README.md) separates the application from CMSIS, StdPeriph, USB/FatFs,
the RTOS and runtime stand-ins. Application entry is
[application_startup.c](../a319/src/app/application_startup.c). The application directories are:

| Directory or unit | Responsibility |
| --- | --- |
| `src/app/comm/` | DSP frames, Modbus slave, meter master and CRC seed/reflection helpers |
| `src/app/control/` | measurements, battery-pack polling, faults, scheduling and energy accounting |
| `src/app/drivers/` | UART queues, EEPROM, I2C, RTC and the retained delay helper |
| `src/app/usb/` | host board glue, user callbacks, dongle and update-stick tasks |
| `src/app/board.c` | board setup, interrupt setup, empty hooks and panel outputs in native order |
| `src/include/` | shared types, settings, subsystem declarations and hardware views |

[firmware_types.h](../a319/src/include/firmware_types.h) defines the ARM integer widths and
interrupt intrinsics. [application_types.h](../a319/src/include/application_types.h) holds
the packed UART records, settings overlays and NVIC view. Subsystem headers hold the shared
declarations. Private views remain local where their signedness, packing or volatility is
required by the compiled instructions. They are documented with `match:` comments.

The [DSP tree](../d319/README.md) already has subsystem names inherited from the 316 readability
work. `src/app/` contains `battery`, `comm`, `control`, `grid`, `meas`, `power`, `rcmu`,
`settings`, `state` and `util`. Drivers, RTOS, boot and TI support are separate. Start with
[app_init.c](../d319/src/app/state/app_init.c),
[arm_link.c](../d319/src/app/comm/arm_link.c) and
[bms_link.c](../d319/src/app/battery/bms_link.c). Vendor names are retained for vendor code.

## Naming and presentation changes

Six ARM files were renamed without splitting or joining compiler units:

| Previous path under `a319/` | Current path |
| --- | --- |
| `src/app/main_p1.c` | `src/app/application_startup.c` |
| `src/app/main_p4.c` | `src/app/drivers/delay.c` |
| `src/app/control/schedule_p1.c` | `src/app/comm/crc32_helpers.c` |
| `src/app/control/schedule_p3.c` | `src/app/control/scheduler.c` |
| `src/include/ghidra_types.h` | `src/include/firmware_types.h` |
| `src/include/a316_types.h` | `src/include/application_types.h` |

Twenty-two ARM globals gained descriptive names from their reads and writes: scheduler timeouts,
RTC state, USB initialization, output permission, load estimation and Modbus readback storage.
The misleading `sys_tick_1s` is now `scheduler_periodic_update`: its caller runs every 500 ms,
and it divides RTC polling by two internally. UART receive members and settings-overlay members
now describe their roles. Private BMS mailbox/pack types and local access macros no longer use
function addresses or unexplained abbreviations. Unused decompiler helpers, four `undefinedN`
aliases and an obsolete legacy meter view were removed. Literal types, layouts and accesses
remain unchanged.

The ARM [name ledger](../a319/map/names.tsv) and [file ledger](../a319/map/filenames.tsv)
record descriptive names separately from historical vendor-name evidence. Existing provenance
rows are retained as history. Native bindings are in each target's `map/functions.tsv` and
`map/data.tsv`. ARM comments now describe the combined board unit and symbolic task entries;
DSP comments correct the swapped BMS buffers and explicit function/address pairs for 319.
The comment-only pass checks token equality before writing each file.

## Why some boundaries and names remain

Filenames affect object names; file contents affect register allocation, pool layout and data
placement. The gate discovers sources recursively and requires unique stems. Moves update
the map, includes and references. The separate delay and CRC helper units are deliberate.
The board functions remain together because of shared pools and halfword entry addresses.
See the [compiler-context audit](../a319/docs/compiler-context.md).

For the DSP, changing declarations or names can move `.ebss` and initialization records.
Its existing descriptive names and compiler boundaries are retained. The HR1 request has
ordinary startup/writer views and a volatile power-task view; these are intentional.

The subsequent [semantic investigation](../a319/docs/semantic-recovery.md) recovered seven
more global roles, four battery fields, eight likely meter quantities and both heap-hook
identities. Six DSP struct members now agree with their established RAM aliases.
Fifteen ARM globals retain address names: most have only clear/store operations and no
identified reader. Undocumented meter fields and some unused DSP settings also remain.
The investigation separates observed behavior from protocol-map inferences and records
which evidence would resolve the rest. None of these unknowns leaves unmatched bytes.

## Rebuild and acceptance

Configure each target's ignored `toolchain.local.toml` from its example, or use the environment
variables in the target README. Keep compilers, licenses and firmware images outside Git.
These commands run from the selected target directory, with the required tools configured:

```sh
# in inverters/gen3-lv/a319; use Exhume's Python environment
python3 tools/check_map.py
python3 tools/gate.py --fast --place all --jobs 4
python3 tools/gate.py --whole --jobs 4
python3 tools/check_map.py --coverage build/gate/coverage.tsv

# in inverters/gen3-lv/d319
python3 tools/gate.py --native
python3 tools/gate.py
python3 tools/coverage.py
```

The licensed ARM whole build must actually run; a skipped build does not prove whole linking.
Require every source unit to match, identical full images and zero original-byte/word blobs.
The negative controls change version 319 to 320, require rejection and restore exact source.
Run `python3 tools/repocheck.py` from the repository root for source hygiene and local links.
No gate counting or acceptance rules were changed by this readability pass.

Final source hashes and accounting are in [A319's proof](../a319/docs/rebuild.json) and
[D319's proof](../d319/docs/rebuild.json). A319 is 147,540 file bytes including its header;
D319 is 131,072 bytes. The unchanged SHA256 values are:

```text
A319  f58f027c3a971fb4260a3fa8123959584f1defe4c55e8808601bfb4c552031d7
D319  932fa4ce4ab4ffa5f36af0ac4fdcca3b3a2cd50e7086418e8fe2d41cd293e886
```

## Documentation scope

Root `docs/operation`, `docs/system` and `docs/reference` pages describe the 316 baseline.
Their formulas, addresses and complete register/link tables have not all been re-audited for
319. The [319 handover](319-handover.md) and both target READMEs identify the recovered changes:
BMS HR1/HR15 handshake, CRC-checked pack replies, the battery-power target, three-phase meter
records and added Modbus settings. Version 318 remains a survey, without a reconstructed tree.
