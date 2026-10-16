#!/bin/sh
# CI only (docs/development/ci.md): point both trees at the decrypted bundle. Run from the
# repository root after tools/ci/ci-assets.sh.
#
#   [EXHUME=...] [C2000_NATIVE=...] sh tools/ci/setup-toolchains.sh ASSETS WORK
#
# ASSETS is the decrypted bundle; WORK a directory for the Wine prefixes this makes (the
# workflow's clean-up step removes both). It
#   - links inverters/gen3-lv/a316/lib/stdperiph35 to ASSETS/stdperiph35;
#   - makes WORK/keil-wine, the licensed MDK's prefix: a new prefix, the bundle's two licence
#     registry keys added to its system.reg, drive_c/Keil_v5 with the licensed TOOLS.INI and
#     links to MDK521's ARM/ARMCC and ARM/sw (the licensed ARMCC is MDK521's plus C++ libraries);
#   - makes WORK/c2000-wine, the D316 master prefix (a new prefix; the compilers stay in ASSETS);
#   - writes each tree's toolchain.local.toml (gitignored), refusing to replace one it did not write.
# EXHUME (an Exhume checkout) sets up A316, the first three; C2000_NATIVE (the bin directory of TI
# CGT 18.12.8 for this system) sets up D316, the last two of each. At least one must be given.
set -eu
assets=${1:?usage: setup-toolchains.sh ASSETS WORK}
work=${2:?usage: setup-toolchains.sh ASSETS WORK}
exhume=${EXHUME:-}
native=${C2000_NATIVE:-}
[ -n "$exhume$native" ] || { echo "setup-toolchains: set EXHUME (A316), C2000_NATIVE (D316) or both"; exit 1; }
a316=inverters/gen3-lv/a316
d316=inverters/gen3-lv/d316
[ -d "$a316" ] && [ -d "$d316" ] || { echo "setup-toolchains: run it from the repository root"; exit 1; }
need=
[ -z "$exhume" ] || need="mdk/MDK521/ARM/ARMCC/bin/armlink.exe mdk/MDK516/ARM/ARMCC/bin/armlink.exe keil/TOOLS.INI keil/licence.reg stdperiph35/Libraries"
[ -z "$native" ] || need="$need c2000/ti-cgt-c2000_16.9.6.LTS/bin/cl2000.exe c2000/rts2800_fpu32.lib"
for f in $need; do
  [ -e "$assets/$f" ] || { echo "setup-toolchains: the bundle has no $f"; exit 1; }
done
[ -z "$native" ] || [ -x "$native/ofd2000" ] || { echo "setup-toolchains: no ofd2000 in $native"; exit 1; }
wine=$(command -v wine) || { echo "setup-toolchains: no wine on PATH"; exit 1; }
winebin=$(dirname "$wine")
assets=$(cd "$assets" && pwd)
mkdir -p "$work"; work=$(cd "$work" && pwd)
mark='# written by tools/ci/setup-toolchains.sh'

ours() {    # true when $1 is absent or was written by this script
  [ ! -e "$1" ] || head -n 1 "$1" | grep -qF "$mark"
}

export WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
newprefix() {
  rm -rf "$1"
  WINEPREFIX=$1 wineboot -i >/dev/null 2>&1 || true
  WINEPREFIX=$1 wineserver -w
  [ -f "$1/system.reg" ] || { echo "setup-toolchains: wineboot made no prefix at $1"; exit 1; }
}

if [ -n "$exhume" ]; then
  # ST's library where the sources' opts lines name it ({root}/lib/stdperiph35)
  sp=$a316/lib/stdperiph35
  if [ -e "$sp" ] || [ -L "$sp" ]; then
    [ -L "$sp" ] || { echo "setup-toolchains: $sp exists and is not a link; leaving it"; exit 1; }
    rm -f "$sp"
  fi
  mkdir -p "$(dirname "$sp")"
  ln -s "$assets/stdperiph35" "$sp"

  # the licensed MDK's prefix: private (it holds the licence)
  umask 077
  kp=$work/keil-wine
  newprefix "$kp"
  cat "$assets/keil/licence.reg" >> "$kp/system.reg"
  mkdir -p "$kp/drive_c/Keil_v5/ARM"
  cp "$assets/keil/TOOLS.INI" "$kp/drive_c/Keil_v5/TOOLS.INI"
  ln -s "$assets/mdk/MDK521/ARM/ARMCC" "$kp/drive_c/Keil_v5/ARM/ARMCC"
  ln -s "$assets/mdk/MDK521/ARM/sw" "$kp/drive_c/Keil_v5/ARM/sw"
  umask 022

  ours "$a316/toolchain.local.toml" || { echo "setup-toolchains: $a316/toolchain.local.toml is not ours; leaving it"; exit 1; }
  cat > "$a316/toolchain.local.toml" <<EOF
$mark (CI)
exhume = "$exhume"
mdk_toolchain = "$assets/mdk"
wine_bin = "$winebin"
mdk_licensed_home = "$kp/drive_c/Keil_v5"
mdk_licensed_prefix = "$kp"
EOF
fi

if [ -n "$native" ]; then
  cp_=$work/c2000-wine
  newprefix "$cp_"

  ours "$d316/toolchain.local.toml" || { echo "setup-toolchains: $d316/toolchain.local.toml is not ours; leaving it"; exit 1; }
  cat > "$d316/toolchain.local.toml" <<EOF
$mark (CI)
[compiler]
cgt = "$assets/c2000/ti-cgt-c2000_16.9.6.LTS"
rtslib = "$assets/c2000/rts2800_fpu32.lib"
others = "$assets/c2000/*/bin/cl2000.exe"
[native]
bin = "$native"
[wine]
bin = "$winebin"
prefix = "$cp_"
EOF
fi
echo "setup-toolchains: done:${exhume:+ A316}${native:+ D316} (Wine $(wine --version 2>/dev/null || echo '?'), prefixes in $work)"
