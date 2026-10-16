#!/bin/sh
# Make the encrypted CI bundle (docs/development/ci.md). Run by the owner on the Mac that holds
# the toolchains; nothing here runs in CI.
#
#   sh tools/ci/make-bundle.sh [OUT]
#
# It copies into a private temporary directory:
#   mdk/MDK521, mdk/MDK516   the two MDK-Lite homes the A316 gate uses (TOOLS.INI, ARM/ARMCC, ARM/sw)
#   keil/TOOLS.INI           the licensed MDK's TOOLS.INI
#   keil/licence.reg         the two registry keys of the licensed prefix that its licence check
#                            reads, in Wine's registry file format (appended to a new prefix's
#                            system.reg by tools/ci/setup-toolchains.sh)
#   stdperiph35              ST's StdPeriph V3.5.0: Libraries/ and the template's include directory
#   c2000/ti-cgt-c2000_16.9.6.LTS   TI's Windows CGT 16.9.6 (bin/ and include/ only)
#   c2000/rts2800_fpu32.lib  the runtime library of CGT 15.12.3
#   MANIFEST.txt, SHA256SUMS
# then tars it, compresses it with zstd, encrypts it with age to the recipient in
# ~/.forgiv-ci-age.pub, and splits the result into parts under 95 MB (GitHub refuses files over
# 100 MB): OUT/forgiv-ci-assets.tar.zst.age.00, .01 ... OUT defaults to ~/forgiv-ci-assets-staging.
# Commit those parts, and only those, to the storage repository in place of the old ones.
#
# Where the inputs are (environment variables, with these defaults):
#   MDK_TOOLCHAIN   ~/seplos-decomp/toolchain        (holds MDK521 and MDK516)
#   KEIL_PREFIX     ~/keil-wine                      (the licensed MDK's Wine prefix)
#   C2000_CGTS      ~/giv-d316-cgt/wine/drive_c/cgt   (holds 16.9.6.LTS and 15.12.3.LTS)
#   STDPERIPH       ~/giv-a316-decomp/lib/stdperiph35
#   AGE_RECIPIENT   ~/.forgiv-ci-age.pub              (a file holding the age1... recipient)
#   LICENCE_KEYS    the registry keys to take from KEIL_PREFIX/system.reg (regular expressions)
#
# The temporary tree holds the licence: it is made with umask 077 under TMPDIR and removed on
# exit. Nothing in it is printed; the script reports file counts and sizes only.
set -eu
out=${1:-$HOME/forgiv-ci-assets-staging}
mdk=${MDK_TOOLCHAIN:-$HOME/seplos-decomp/toolchain}
keil=${KEIL_PREFIX:-$HOME/keil-wine}
cgts=${C2000_CGTS:-$HOME/giv-d316-cgt/wine/drive_c/cgt}
sp=${STDPERIPH:-$HOME/giv-a316-decomp/lib/stdperiph35}
rcpt=${AGE_RECIPIENT:-$HOME/.forgiv-ci-age.pub}
# The licence check of armcc, armasm and armlink 5.06 (MDK 5.21) reads these two keys under
# HKLM\Software\Classes; with them, the licensed TOOLS.INI and the MDK's ARM directory, a new
# prefix links as the licensed MDK. Found by adding the keys of the licensed prefix's system.reg
# that a new prefix lacks until the whole link passed (docs/development/ci.md).
# The registry keys that hold the licence (regex per key, space-separated) are not published:
# set LICENCE_KEYS, or keep them in tools/ci/licence-keys.local (gitignored; see docs/development/ci.md).
if [ -z "${LICENCE_KEYS:-}" ] && [ -f "$(dirname "$0")/licence-keys.local" ]; then
  LICENCE_KEYS=$(cat "$(dirname "$0")/licence-keys.local")
fi
[ -n "${LICENCE_KEYS:-}" ] || { echo "make-bundle: set LICENCE_KEYS or create tools/ci/licence-keys.local" >&2; exit 1; }
keys=$LICENCE_KEYS
name=forgiv-ci-assets.tar.zst.age

for d in "$mdk/MDK521/ARM/ARMCC" "$mdk/MDK516/ARM/ARMCC" "$keil/drive_c/Keil_v5/ARM/ARMCC" \
         "$cgts/16.9.6.LTS/ti-cgt-c2000_16.9.6.LTS/bin" "$sp/Libraries"; do
  [ -d "$d" ] || { echo "make-bundle: no $d"; exit 1; }
done
for f in "$keil/system.reg" "$keil/drive_c/Keil_v5/TOOLS.INI" "$rcpt" \
         "$cgts/15.12.3.LTS/c2000_15.12.3.LTS/lib/rts2800_fpu32.lib"; do
  [ -f "$f" ] || { echo "make-bundle: no $f"; exit 1; }
done
for t in age zstd python3; do command -v $t >/dev/null || { echo "make-bundle: needs $t"; exit 1; }; done

umask 077
work=$(mktemp -d "${TMPDIR:-/tmp}/forgiv-bundle.XXXXXX")
trap 'rm -rf "$work"' EXIT INT TERM
t=$work/tree
mkdir -p "$t"
# cp -c clones on APFS (no space used); a plain copy elsewhere
cpr() { cp -Rc "$@" 2>/dev/null || cp -R "$@"; }

for v in MDK521 MDK516; do
  mkdir -p "$t/mdk/$v/ARM"
  cp "$mdk/$v/TOOLS.INI" "$t/mdk/$v/"
  cpr "$mdk/$v/ARM/ARMCC" "$mdk/$v/ARM/sw" "$t/mdk/$v/ARM/"
done

# The licensed MDK: its TOOLS.INI and the registry keys. Its ARM/ARMCC is MDK521's plus cpplib (C++
# libraries, which nothing here links), so the CI prefix links MDK521's ARM directory instead of
# carrying a second copy; check that it is still the same.
mkdir -p "$t/keil"
cp "$keil/drive_c/Keil_v5/TOOLS.INI" "$t/keil/TOOLS.INI"
if ! diff -rq -x cpplib "$keil/drive_c/Keil_v5/ARM/ARMCC" "$mdk/MDK521/ARM/ARMCC" >/dev/null; then
  echo "make-bundle: the licensed MDK's ARM/ARMCC is not MDK521's (cpplib aside): bundle it instead"; exit 1
fi
# shellcheck disable=SC2086
python3 -I - "$keil/system.reg" "$t/keil/licence.reg" $keys <<'EOF'
import re, sys
src, dest, pats = sys.argv[1], sys.argv[2], [re.compile(p) for p in sys.argv[3:]]
text = open(src, encoding='latin1').read()
found = {}
for part in re.split(r'\n(?=\[)', text):
    m = re.match(r'\[([^\]]*)\]', part)
    if m:
        for p in pats:
            if p.search(m.group(1)): found[p.pattern] = part.rstrip('\n')
missing = [p.pattern for p in pats if p.pattern not in found]
if missing: sys.exit(f'make-bundle: {len(missing)} licence key(s) not in system.reg')
with open(dest, 'w', encoding='latin1', newline='\n') as f:
    f.write('\n' + '\n\n'.join(found[p.pattern] for p in pats) + '\n')
print(f'make-bundle: {len(found)} registry keys for the licence')
EOF

mkdir -p "$t/stdperiph35/Project"
cpr "$sp/Libraries" "$t/stdperiph35/"
cpr "$sp/Project/STM32F10x_StdPeriph_Template" "$t/stdperiph35/Project/"

mkdir -p "$t/c2000/ti-cgt-c2000_16.9.6.LTS"
cpr "$cgts/16.9.6.LTS/ti-cgt-c2000_16.9.6.LTS/bin" "$cgts/16.9.6.LTS/ti-cgt-c2000_16.9.6.LTS/include" \
    "$t/c2000/ti-cgt-c2000_16.9.6.LTS/"
cp "$cgts/15.12.3.LTS/c2000_15.12.3.LTS/lib/rts2800_fpu32.lib" "$t/c2000/"

cat > "$t/MANIFEST.txt" <<EOF
forgiv CI bundle (docs/development/ci.md), made $(date -u +%Y-%m-%dT%H:%MZ) by tools/ci/make-bundle.sh.
Private: encrypted with age; never cached, uploaded or printed by CI.

mdk/MDK521            Keil MDK 5.21 (armcc 5.06 update 3): TOOLS.INI, ARM/ARMCC, ARM/sw.
                      Keil's public MDK521.EXE, used as MDK-Lite. The A316 compile home.
mdk/MDK516            Keil MDK 5.16: the same three. The A316 library home.
keil/TOOLS.INI        the licensed MDK's TOOLS.INI (holds the licence).
keil/licence.reg      the two HKLM\\Software\\Classes keys the licence check reads, from the
                      licensed MDK's Wine prefix. Keil confirmed the licence may be used on the
                      CI runner. tools/ci/setup-toolchains.sh makes a new prefix with these,
                      TOOLS.INI and MDK521's ARM directory (the licensed ARM/ARMCC is MDK521's
                      plus cpplib), for gate.py --whole and modbuild.py.
stdperiph35           ST's STM32F10x StdPeriph library V3.5.0 (en.stsw-stm32054), Libraries/ and
                      Project/STM32F10x_StdPeriph_Template only. Linked at a316/lib/stdperiph35.
c2000/ti-cgt-c2000_16.9.6.LTS   TI C2000 CGT 16.9.6.LTS, Windows build, bin/ and include/:
                      the D316 compiler (and its lnk2000, run under Wine).
c2000/rts2800_fpu32.lib         rts2800_fpu32.lib of TI C2000 CGT 15.12.3.LTS: the D316 runtime library.

Not in the bundle: TI CGT 18.12.8.LTS for Linux (public; the workflow downloads it), the
firmware images (public; tools/fetch_firmware.py), Exhume (public; checked out at a pinned commit).
EOF

( cd "$t" && find . -type f ! -name SHA256SUMS -print | sed 's|^\./||' | LC_ALL=C sort | \
  while IFS= read -r f; do shasum -a 256 "$f"; done > SHA256SUMS )
n=$(grep -c . "$t/SHA256SUMS")
echo "make-bundle: $n files, $(du -sk "$t" | cut -f1) KB before compression"

mkdir -p "$out"
rm -f "$out/$name".[0-9][0-9]
( cd "$t" && COPYFILE_DISABLE=1 tar --no-xattrs --uid 0 --gid 0 --uname root --gname root -cf - \
    mdk keil stdperiph35 c2000 MANIFEST.txt SHA256SUMS ) \
  | zstd -q -19 --long=30 -T0 \
  | age -R "$rcpt" \
  | (cd "$out" && split -b 94m -a 2 - "$name.tmp.")
# split names the parts .tmp.aa, .tmp.ab ...: number them .00, .01 ...
i=0
for p in "$out/$name".tmp.*; do
  mv "$p" "$out/$name.$(printf %02d $i)"; i=$((i + 1))
done
ls -l "$out/$name".[0-9][0-9] | awk '{print "make-bundle: " $NF " " $5 " bytes"}'
echo "make-bundle: $i part(s) in $out; commit them to the storage repository (docs/development/ci.md)"
