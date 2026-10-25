# Batteries

The GivEnergy LV (Gen 2) battery's BMS firmware, reconstructed so that each version rebuilds
the vendor's image byte for byte:

- [LV BMS 3020](lv/bms-3020/README.md)
- [LV BMS 3022](lv/bms-3022/README.md), built from 3020. It adds staged battery heating and
  some protection and balancing changes.

In both versions everything is C apart from one function, the ASCII protocol dispatcher,
which is still in assembler pending its C match
([issue #4](https://github.com/abedegno/forgiv/issues/4)). Firmware 3019 and 3021 have the
same dispatcher. These trees publish the sources and the battery documentation.

Battery images are in the same public archive as the inverter's
(github.com/DJBenson/giv-firmware), and a battery image also reaches the BMS through the
inverter: the A316 ARM relays an `ARMStore.bin` with a version above 1000 through the DSP
(inverters/gen3-lv/a316/docs/subsystems/usb.md).

What the inverter expects of the battery is already in the inverter trees:
[a316/docs/battery.md](../inverters/gen3-lv/a316/docs/battery.md) for the ARM and
[d316/docs/battery.md](../inverters/gen3-lv/d316/docs/battery.md) for the DSP, which is the Modbus
master to the BMS.

For an outside view of the battery firmware and its protocols, see the open analysis at
[open-giv/bms-analysis](https://github.com/open-giv/bms-analysis), which uses Ghidra and emulation.
