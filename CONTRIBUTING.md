# Contributing

Corrections, better names and comments, answers to the open questions in the findings, and new
firmwares are all welcome. A change is accepted when the gate of every tree it touches still says
IDENTICAL. How to build and run the gates is in
[docs/development/building.md](docs/development/building.md), and the notes and rules the sources
follow are in [docs/development/conventions.md](docs/development/conventions.md).

## The rules

- **The gate stays IDENTICAL.** Run the tree's `tools/gate.py` (the full run, not only `--fast`)
  before a pull request, and paste its last lines into it. A change that alters one byte of the
  rebuilt image is not a readability change; if you think the source is wrong, open an issue.
- **One firmware per directory.** Each tree holds its own `src/`, `map/`, `tools/`, `docs/`,
  configuration and README, and builds without the others. Shared code between trees is fine
  when it is copied in, not imported across trees.
- **No binaries.** No firmware images (`.bin`, `.hex`), packages (`.zip`), objects or executables,
  and nothing extracted from a vendor's toolchain or library. Images are fetched by
  `tools/fetch_firmware.py` from `firmware/manifest.toml`; toolchains are installed by each user
  ([docs/development/toolchains.md](docs/development/toolchains.md)). `tools/repocheck.py` checks
  this by file extension and by header, and CI runs it.
- **No flashing claims.** Nothing here is tested on hardware. Do not describe a change as safe to
  flash, and do not add instructions for flashing a modified image to an inverter or battery
  ([SECURITY.md](SECURITY.md)).
- **No machine paths or personal details.** No home directories, user names, serial numbers of
  your own equipment, or capture files that carry them. Tool paths go through the tree's
  environment variables or `toolchain.local.toml`.

## Adding a firmware

A new firmware goes in its own directory, `inverters/<model>/<firmware>/` or
`batteries/<model>/<firmware>/`, named in lower case after the version (`a316`, `d316`). The
stages (fingerprint, map, match, gate, readability, with [Exhume](https://github.com/abedegno/Exhume)'s
skills for each) and the checklist (the manifest entry, the README and status row, NOTICE.md, the
toolchain page) are in
[docs/development/adding-a-firmware.md](docs/development/adding-a-firmware.md).

A firmware may come in before it is complete, if its README says so plainly and its gate passes
for what is there (blobs of original bytes counted and reported). The new-firmware issue template
asks for the package's source and sha256 first, so that it can be checked before work starts.

## Commits and pull requests

- Small commits, each with a subject line in the imperative ("Name the BMS poll state words"),
  under about 72 characters, and a body that says what and why when it is not obvious.
- One topic per pull request. Fill in the template: which trees, the gate's last lines, and
  that no binaries are added.
- Write in plain English, ASCII only, and say "inference" where a statement goes beyond what the
  code shows.

## Conduct

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md).
