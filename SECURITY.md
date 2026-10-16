# Safety and security

## Nothing here is meant to be flashed

The sources in this repository rebuild the vendor's firmware images, byte for byte. That is what
they are for and all I have tested: no image built from them, changed or unchanged, has been run
on an inverter or a battery.

The Gen3 LV inverter is grid-connected, certified equipment that handles mains voltage and a
large battery. Its firmware carries the grid protection the inverter was certified with, and the
battery protection the battery relies on. A modified image could damage the inverter, the battery
or the installation, start a fire, or stop the inverter disconnecting from the grid when it must,
which endangers people working on the network. Do not flash anything built from this repository
to a grid-connected inverter, or to a battery.

The notes describe how the firmware behaves, including its protections and some likely bugs
(each tree's `docs/findings*.md`). They are a reading of the code, marked "inference" where they go
further, and not advice on how to operate or modify equipment.

## Reporting a problem

- **A mistake in the sources or the notes** (a wrong name, a wrong reading of the code, a broken
  tool): open an issue with the bug template.
- **A safety problem in the inverter's own firmware** that the notes reveal (for example a
  protection that cannot act): open an issue, unless publishing the details would let someone
  cause harm before owners can protect themselves; in that case report it privately with
  GitHub's "Report a vulnerability" on this repository's Security tab, and I will reply there.
- **A security problem in this repository's tools** (for example `tools/fetch_firmware.py`
  writing outside `firmware/`): report it privately the same way.

I maintain this in my own time, and reply to private reports there as soon as I can.
