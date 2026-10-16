# Installing the toolchains

The gates rebuild each image with the toolchain that built the original, so each needs that
exact toolchain. None of it is in this repository: Keil's and TI's tools come from their own
public download servers, ST's library from st.com. [building.md](building.md) says how to run
the gates once everything is installed. I run everything on macOS (Apple silicon) with Wine
Devel. CI runs the same Windows tools under Wine on Linux (Debian 12's Wine 8.0), with TI's Linux
build of CGT 18.12.8 ([ci.md](ci.md)).

What each firmware needs:

| | A316 (ARM) | D316 (DSP) |
|---|---|---|
| Compiler | Keil MDK 5.21 (armcc 5.06), Windows, under Wine | TI C2000 CGT 16.9.6.LTS, Windows, under Wine |
| Library | Keil MDK 5.16's C library | TI CGT 15.12.3.LTS's `rts2800_fpu32.lib` |
| Object tools | MDK's own (armlink, fromelf) | TI CGT 18.12.8.LTS, native macOS build |
| Other | ST's StdPeriph library V3.5.0; Exhume | none |
| Python | Exhume's venv (3.11 or later) | 3.11 or later, no packages |
| Settings | `EXHUME`, `MDK_TOOLCHAIN` (or `MDK_HOME`, `MDK_LIBHOME`), `WINE_BIN` | `C2000_WINEPREFIX`, `C2000_CGT`, `C2000_RTSLIB`, `C2000_NATIVE`, `WINE_BIN` |

Each setting is an environment variable, or a key in the tree's `toolchain.local.toml`
(gitignored; copy the tree's `toolchain.example.toml`). The defaults are in each tree's README.

## Wine

On macOS, Wine Devel from WineHQ, through Homebrew:

```sh
brew install --cask wine@devel
```

If Gatekeeper refuses to run it, clear the quarantine flag:
`xattr -dr com.apple.quarantine "/Applications/Wine Devel.app"`.

It installs `/Applications/Wine Devel.app`; the tools look for `wine` in
`/Applications/Wine Devel.app/Contents/Resources/wine/bin` when it is not on `PATH`, or in
`WINE_BIN`. I used Wine 11.18. On Apple silicon, Wine runs the x86 Windows tools through Rosetta 2.

Each gate makes Wine prefixes of its own under its `build/` directory. The A316 gate creates
them with `wineboot` on its first run (a few minutes once). The D316 tools clone a master prefix
(`C2000_WINEPREFIX`, the one the TI compilers are installed in) with `cp -Rc`, which is an APFS
clone on macOS.

## Exhume (for A316)

The A316 tools import the armcc5 profile of [Exhume](https://github.com/abedegno/Exhume):

```sh
git clone https://github.com/abedegno/Exhume
cd Exhume
python3 -m venv .venv && .venv/bin/pip install -r tools/requirements.txt
export EXHUME=$PWD
```

I ran the gate with Exhume at commit 63f1b97. Without `EXHUME`, the tools look for `exhume` or
`Exhume` beside this repository.

## Keil MDK (for A316)

Two MDK releases, both from Keil's public server, used as MDK-Lite (no licence needed):

| Home | Installer | Size | Used for |
|---|---|---|---|
| `MDK521` | https://armkeil.blob.core.windows.net/eval/MDK521a.EXE | 634,123,808 bytes, sha256 `757327a72c74df3cafbb4a50dcd9e6d7fdf5ea7d613b1fa2134c5153bc2605fa` | the compiler, armcc 5.06 (build 80) |
| `MDK516` | https://armkeil.blob.core.windows.net/eval/MDK516.EXE | 383,419,512 bytes, sha256 `17a6ecf5159baab7ec799a64ad7b4c4a1fb6277734f3c7c23353297c353826a2` | the C library the image links |

MDK 5.21 (`MDK521.EXE`) has the same armcc.exe as 5.21a; Keil's server has the 5.21a installer.
By the A316 fingerprint, a later MDK should do for the compiler and any MDK from 5.16 to 5.36 for
the library; I ran the gate with these two (a316 README, "Fingerprint").

A home is the installer's `ARM/ARMCC` and `ARM/sw` directories. Exhume's
`profiles/armcc5/setup-mdk.sh` unpacks them from MDK 5.36 (it checks 5.36's sha256 and also
builds a Docker image); for these two releases, run its extraction step by hand, with 7-Zip
(`brew install p7zip`):

```sh
mkdir -p ~/mdk-homes && cd ~/mdk-homes
7z x -oMDK521 MDK521a.EXE 'ARM/ARMCC/*' 'ARM/sw/*'
7z x -oMDK516 MDK516.EXE  'ARM/ARMCC/*' 'ARM/sw/*'
export MDK_TOOLCHAIN=~/mdk-homes
```

`MDK_TOOLCHAIN` is a directory of homes named `MDKnnn`; the gate takes `MDK521` and `MDK516`
from it. Or link it as `inverters/gen3-lv/a316/toolchain`, or name the two homes with `MDK_HOME`
and `MDK_LIBHOME`. More releases can sit beside these two: Exhume's armcc5 profile tools, which
fingerprint a build, can try every home in it.

## ST StdPeriph library V3.5.0 (for A316)

The twelve files in `inverters/gen3-lv/a316/src/stdperiph` include ST's driver sources unchanged,
from `inverters/gen3-lv/a316/lib/stdperiph35`. Get ST's package STSW-STM32054 ("STM32F10x
standard peripheral library", V3.5.0) from st.com (free; ST asks you to log in), and unpack it so
that this path exists:

    inverters/gen3-lv/a316/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/misc.c

A symbolic link at `lib/stdperiph35` to an unpacked copy elsewhere works too. Another release of
the library fails the gate's verify stage.

## TI C2000 code generation tools (for D316)

Three releases, all from TI's public server, at
`https://downloads.ti.com/codegen/esd/cgt_public_sw/C2000/<version>/`:

| Release | File | Used for |
|---|---|---|
| 16.9.6.LTS | `ti_cgt_c2000_16.9.6.LTS_windows_installer.exe` | the compiler (cl2000) and the gate's linker (lnk2000), under Wine |
| 15.12.3.LTS | `ti_cgt_c2000_15.12.3.LTS_windows_installer.exe` | its `lib/rts2800_fpu32.lib` only, under Wine |
| 18.12.8.LTS | `ti_cgt_c2000_18.12.8.LTS_osx_installer.app.zip` | the native object tools: ofd2000, dis2000, hex2000, lnk2000 for `match.py` |

Install the two Windows releases into one Wine prefix, the master prefix the tools clone:

```sh
export C2000_WINEPREFIX=~/c2000-cgt/wine
WINEPREFIX=$C2000_WINEPREFIX wineboot -i
WINEPREFIX=$C2000_WINEPREFIX wine ti_cgt_c2000_16.9.6.LTS_windows_installer.exe
WINEPREFIX=$C2000_WINEPREFIX wine ti_cgt_c2000_15.12.3.LTS_windows_installer.exe
```

The installers ask for a directory. The defaults in `inverters/gen3-lv/d316/toolchain.toml`
expect `C:\ti\ti-cgt-c2000_16.9.6.LTS` and `C:\ti\c2000_15.12.3.LTS`, which are
`$C2000_WINEPREFIX/drive_c/ti/...`. Anywhere else, name them:

```sh
export C2000_CGT=$C2000_WINEPREFIX/drive_c/<dir>/ti-cgt-c2000_16.9.6.LTS
export C2000_RTSLIB=$C2000_WINEPREFIX/drive_c/<dir>/c2000_15.12.3.LTS/lib/rts2800_fpu32.lib
```

The native tools: unzip the macOS installer, run it, and point `C2000_NATIVE` at its `bin`
directory (default `~/c2000-cgt/ti-cgt-c2000_18.12.8.LTS/bin`):

```sh
export C2000_NATIVE=~/c2000-cgt/ti-cgt-c2000_18.12.8.LTS/bin
```

`tools/match.py --all-cgt` compiles with every compiler installed in the prefix (the glob
`C2000_OTHERS`, default `{prefix}/drive_c/ti/*/bin/cl2000.exe`). The gate needs only the three
above.

## Checking the set-up

```sh
python3 tools/fetch_firmware.py                       # from the repository root
cd inverters/gen3-lv/a316 && $EXHUME/.venv/bin/python tools/gate.py
cd ../d316 && python3 tools/gate.py
```

The A316 gate ends with `-- gate: PASS`, after `bin: ... IDENTICAL` and `hex: ... IDENTICAL`;
the D316 gate with `IDENTICAL: the linked image is ... word for word` and `PASS`. Both stop
their Wine servers when they end.
