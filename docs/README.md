# The forgiv documentation

This is the place to start reading about the GivEnergy Hybrid Gen3 LV inverter and its two
firmwares, as the byte-exact sources in this repository show them. Every statement in these
pages is read from the matched code and names the function and file it comes from. Where I go
beyond what the code shows (what a pin is wired to, what a standard calls a setting, what an
owner would see), I say "inference".

The two firmware trees have their own, more detailed notes:

- [inverters/gen3-lv/a316](../inverters/gen3-lv/a316/README.md): the ARM firmware A316 (STM32F105/107),
  with `docs/subsystems/*.md`, `docs/battery.md` and `docs/findings.md`;
- [inverters/gen3-lv/d316](../inverters/gen3-lv/d316/README.md): the DSP firmware D316 (TMS320F2806x),
  with `docs/subsystems/*.md`, `docs/battery.md` and `docs/findings*.md`.

The pages here are the view across both chips. They summarise and connect; for every formula
and threshold they link to the tree that holds it.

## Reading paths

**If you own an inverter** and want to know what it does:

1. [system/architecture.md](system/architecture.md): the two chips and what each one decides.
2. [operation/energy-modes.md](operation/energy-modes.md): how eco mode, timed charge and
   discharge, export limits and remote control reach the power stage.
3. [operation/battery.md](operation/battery.md): what the inverter does with the battery and
   its BMS.
4. [operation/faults.md](operation/faults.md) and [reference/fault-bits.md](reference/fault-bits.md):
   what the fault numbers in the app come from.
5. [findings.md](findings.md): the likely bugs and surprises, by confidence.

**If you want to rebuild or change the firmware:**

1. [development/building.md](development/building.md): toolchains, images and the gates.
2. [development/method.md](development/method.md) and
   [development/conventions.md](development/conventions.md): how the sources were matched and how
   to read their notes.
3. [development/modding.md](development/modding.md): what a changed build needs, chip by chip,
   and why nothing here is meant to be flashed.
4. [reference/arm-dsp-link.md](reference/arm-dsp-link.md) and
   [reference/registers.md](reference/registers.md): the two interfaces most changes touch.

**If you are a researcher:**

1. [system/hardware.md](system/hardware.md) and [operation/control.md](operation/control.md):
   the power stages and control loops as the DSP code shows them.
2. [operation/grid.md](operation/grid.md): protection, grid codes, P(f), Q(U), anti-islanding.
3. [system/update-packages.md](system/update-packages.md): the update files and what the boot
   loaders check.
4. [findings.md](findings.md), then each tree's findings.

## Map of the pages

| Page | What it covers |
|---|---|
| [system/architecture.md](system/architecture.md) | the two chips and their jobs; the ARM-DSP link; the dongle, RS485, meter and battery buses |
| [system/hardware.md](system/hardware.md) | the MCUs, the peripherals each uses, the power stages as the code drives them |
| [system/update-packages.md](system/update-packages.md) | `ARMStore.bin`, `DSPStore.bin`, the USB stick flow, the DSP boot loader protocol |
| [operation/startup.md](operation/startup.md) | power-on to running, on both chips |
| [operation/states.md](operation/states.md) | the DSP's operating states and the ARM's run modes |
| [operation/energy-modes.md](operation/energy-modes.md) | ARM settings and schedules turned into DSP commands |
| [operation/battery.md](operation/battery.md) | the battery across both chips |
| [operation/grid.md](operation/grid.md) | grid codes, protection, grid support, RCMU, anti-islanding |
| [operation/control.md](operation/control.md) | the DSP's interrupt schedule, control loops and MPPT |
| [operation/faults.md](operation/faults.md) | the fault words and how they travel to Modbus |
| [operation/comms.md](operation/comms.md) | the Modbus slave, the meter master, the ARM-DSP frames |
| [reference/registers.md](reference/registers.md) | holding and input registers |
| [reference/arm-dsp-link.md](reference/arm-dsp-link.md) | every byte of the 45-byte frames, both directions |
| [reference/fault-bits.md](reference/fault-bits.md) | every fault and status bit the code names |
| [findings.md](findings.md) | the main findings across both chips |
| [development/building.md](development/building.md) | the byte-exact rebuild of each chip |
| [development/toolchains.md](development/toolchains.md) | installing the toolchains |
| [development/modding.md](development/modding.md) | changing behaviour: what each chip's build allows |
| [development/method.md](development/method.md) | how byte-exact matching works here |
| [development/conventions.md](development/conventions.md) | the notes in the sources, the maps, the readability rules |
| [development/adding-a-firmware.md](development/adding-a-firmware.md) | adding another firmware to the repository |

The toolchain page sits in `development/` beside the build page that needs it, and
`adding-a-firmware.md` holds the steps for a new firmware; CONTRIBUTING.md links to it.

## Names, units and numbers

- "HRn" and "IRn" are the inverter's Modbus holding and input registers, as the ARM serves them.
  "BMS HRn" is a register of the battery, which only the DSP reads.
- "CF12" to "CF19" are the DSP's eight fault and status words at 0xCF12 to 0xCF19
  ([reference/fault-bits.md](reference/fault-bits.md)).
- Byte numbers in the ARM-DSP frames count from 0, the frame type byte.
- Units are the code's: the DSP keeps voltages in 0.1 V, powers in 0.1 W, BMS currents in
  0.01 A; the ARM's battery voltage from the DSP is in 0.01 V. Each page says which.
- Function names link to the file that defines them. Names come from `map/names.tsv` of each
  tree, which gives the evidence for each.

Nothing in these pages is advice to change or flash an inverter. See
[SECURITY.md](../SECURITY.md).
