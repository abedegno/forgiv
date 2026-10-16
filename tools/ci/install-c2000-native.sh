#!/bin/sh
# CI only (docs/development/ci.md): TI's C2000 code generation tools 18.12.8.LTS for Linux x86-64,
# the D316 tools' native object tools (ofd2000, dis2000, hex2000, lnk2000, ar2000, cl2000). TI's
# public installer, checked against its SHA-256 and run unattended.
#
#   sh tools/ci/install-c2000-native.sh DIR
#
# installs into DIR/ti-cgt-c2000_18.12.8.LTS and prints C2000_NATIVE=DIR/ti-cgt-c2000_18.12.8.LTS/bin
# (appended to $GITHUB_ENV when that is set). An installation already in DIR is kept: the
# workflow caches DIR, keyed on this file.
set -eu
dir=${1:?usage: install-c2000-native.sh DIR}
ver=18.12.8.LTS
file=ti_cgt_c2000_${ver}_linux-x64_installer.bin
url=https://downloads.ti.com/codegen/esd/cgt_public_sw/C2000/$ver/$file
sha=21b3eb6a5443b4c282c2c8a7717736a334db3c8ab45772f4e8eef8779313f75c
mkdir -p "$dir"; dir=$(cd "$dir" && pwd)
bin=$dir/ti-cgt-c2000_$ver/bin
if [ ! -x "$bin/ofd2000" ]; then
  tmp=$(mktemp -d)
  curl -fsSL --retry 3 -o "$tmp/$file" "$url"
  echo "$sha  $tmp/$file" | sha256sum -c --quiet
  chmod +x "$tmp/$file"
  "$tmp/$file" --mode unattended --prefix "$dir" >/dev/null
  rm -rf "$tmp"
fi
for t in ofd2000 dis2000 hex2000 lnk2000 ar2000 cl2000; do
  [ -x "$bin/$t" ] || { echo "install-c2000-native: no $t in $bin"; exit 1; }
done
echo "C2000_NATIVE=$bin"
[ -z "${GITHUB_ENV:-}" ] || echo "C2000_NATIVE=$bin" >> "$GITHUB_ENV"
