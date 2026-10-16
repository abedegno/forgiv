# GivEnergy Hybrid Gen3 LV inverter

The Gen3 LV hybrid inverter (G3) has two processors, each with its own firmware. GivEnergy
shipped them together in update packages named after both versions; this directory holds version
316 of each, from the package `A316-D316.zip`.

| Directory | Firmware | Chip | Update file |
|---|---|---|---|
| [a316/](a316/README.md) | A316 | STMicroelectronics STM32F105/107 (Cortex-M3, connectivity line) | `ARMStore.bin` |
| [d316/](d316/README.md) | D316 | Texas Instruments TMS320F2806x (C28x with FPU) | `DSPStore.bin` |

## What each chip does

The **ARM** (A316) is the inverter's main controller and its link to the outside:

- the Modbus slave on RS485 and, through the WiFi or LAN dongle on its USB port, for the cloud
  and the app (`src/app/comm/modbus_slave.c`);
- the settings (holding registers) and their storage in EEPROM, the clock and the schedules
  (charge and discharge windows);
- the energy meter on its own bus, the panel LEDs, the event and fault logs;
- USB in host mode: the dongle (CDC class) and update sticks (mass storage, FatFs);
- the decisions above the power stage: when to charge, discharge or export, from the settings,
  the schedule and the meter.

The **DSP** (D316) runs the power conversion:

- the PWM control loops of the battery converter, the inverter bridge (grid or EPS) and the PV
  boosts, from a fast interrupt (`control_isr`);
- the measurements (ADC scaling, RMS, power), the grid protection and the grid-code responses;
- the battery: it is the Modbus master to the battery's BMS on its own serial port, and applies
  the BMS's current limits, the state-of-charge floors and its own voltage trips;
- a small boot loader that takes a new DSP image over the link from the ARM.

So the ARM never talks to the battery directly: what it knows of the battery comes through the
DSP.

## The link between them

The ARM's UART4 and the DSP's SCI-A, at 9600 baud, 8N1. Both directions use 45-byte frames: a
type byte (0x43 from the ARM, 0x44 from the DSP), a few command or status bytes, a page number
and a page of data, and a CRC-16/MODBUS over the first 43 bytes. The ARM sends a settings page every 60 ms (pages 0 to 7
in turn); the DSP answers with a telemetry page every 240 ms (pages 0 to 6), which carries the
measurements, the fault words and the BMS data. Both sides of each page are described in
[a316/docs/subsystems/comm.md](a316/docs/subsystems/comm.md) and
[d316/docs/subsystems/arm_link.md](d316/docs/subsystems/arm_link.md).

## Update packages

A package such as `A316-D316.zip` holds `ARMStore.bin` and `DSPStore.bin`. Copied to a USB stick
in the inverter's USB port, the ARM finds them (a316/docs/subsystems/usb.md):

- `DSPStore.bin`: the ARM stops normal work and sends the image to the DSP over the link, in
  128-byte frames; the DSP's boot loader writes it to flash and never overwrites itself
  (d316/docs/subsystems/boot.md).
- `ARMStore.bin` with a version below 1000: the ARM marks its image for replacement and its own
  boot loader, which is not in this image, takes the new one on the next start (inference).
- `ARMStore.bin` with a version above 1000: a battery image, relayed through the DSP to the BMS
  (the last step is inference).

`ARMStore.bin` is a 4-byte header (the version as a 16-bit word, 0x013C for 316, then 0x5566)
and the Cortex-M3 image for 0x08014000. `DSPStore.bin` is 64K 16-bit words, big-endian, for
word addresses 0x3E8000 to 0x3F7FFF; its first word is the version.

## Documentation

The pages across both chips are in [docs/](../../docs/README.md) at the repository root:
[architecture](../../docs/system/architecture.md), [start-up](../../docs/operation/startup.md),
[states](../../docs/operation/states.md), [energy modes](../../docs/operation/energy-modes.md),
[the battery](../../docs/operation/battery.md), and the
[ARM-DSP link byte by byte](../../docs/reference/arm-dsp-link.md).
