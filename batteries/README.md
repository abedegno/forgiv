# Batteries

No battery firmware is decompiled here yet.

A GivEnergy battery's BMS firmware would go in its own directory, `batteries/<model>/<firmware>/`,
like the inverter trees: its sources, map, tools with a gate that rebuilds the vendor's image
byte for byte, notes, and a package entry in `firmware/manifest.toml`
([docs/development/adding-a-firmware.md](../docs/development/adding-a-firmware.md)). Battery images are in the same public archive as the inverter's
(github.com/DJBenson/giv-firmware), and a battery image also reaches the BMS through the
inverter: the A316 ARM relays an `ARMStore.bin` with a version above 1000 through the DSP
(inverters/gen3-lv/a316/docs/subsystems/usb.md).

What the inverter expects of the battery is already in the inverter trees:
[a316/docs/battery.md](../inverters/gen3-lv/a316/docs/battery.md) for the ARM and
[d316/docs/battery.md](../inverters/gen3-lv/d316/docs/battery.md) for the DSP, which is the Modbus
master to the BMS.

For the battery side, see the existing open analysis of the GivEnergy LV battery firmware and its
protocol at [open-giv/bms-analysis](https://github.com/open-giv/bms-analysis) (Ghidra and
emulation, not byte-exact). The first step for a byte-exact tree is the same as for any firmware:
identify the BMS's microcontroller and the exact toolchain that built its image.
