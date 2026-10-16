#!/bin/sh
# CI only (docs/development/ci.md): join the parts of the encrypted toolchain bundle (cloned from
# the private storage repository), decrypt it into DEST, and check every file against the
# bundle's SHA256SUMS. After Exhume's tools/ci-assets.sh (docs/ci-bundle.md there).
#
#   FORGIV_ASSETS_AGE_KEY=... sh tools/ci/ci-assets.sh CLONE DEST
#
# CLONE holds forgiv-ci-assets.tar.zst.age.00, .01 ... (tools/ci/make-bundle.sh makes them). The
# age secret key comes from the environment and goes to age on its standard input, never to a
# file. Nothing in the bundle is listed or printed: the check says only how many files passed.
# On success it prints FORGIV_ASSETS=DEST, and appends it to $GITHUB_ENV when that is set.
# DEST is removed by the workflow's clean-up step; nothing under it may be cached or uploaded.
set -eu
clone=${1:?usage: ci-assets.sh CLONE DEST}
dest=${2:?usage: ci-assets.sh CLONE DEST}
name=forgiv-ci-assets.tar.zst.age
key=${FORGIV_ASSETS_AGE_KEY:-}
[ -n "$key" ] || { echo "ci-assets: FORGIV_ASSETS_AGE_KEY is not set"; exit 1; }
for t in age zstd tar; do command -v $t >/dev/null || { echo "ci-assets: needs $t"; exit 1; }; done
set -- "$clone/$name".[0-9][0-9]
[ -f "$1" ] || { echo "ci-assets: no $name.00 in $clone"; exit 1; }
umask 077
rm -rf "$dest"; mkdir -p "$dest"; dest=$(cd "$dest" && pwd)
# the parts in order (the glob sorts them), joined into one ciphertext beside DEST: age reads the
# key on its standard input, so the ciphertext must be a file
joined="$dest.age"
cat "$@" > "$joined"
# GNU tar warns about the macOS extended attributes a bundle made on a Mac might carry
quiet=; tar --version 2>/dev/null | grep -q GNU && quiet=--warning=no-unknown-keyword
if ! printf '%s\n' "$key" | age -d -i - "$joined" | zstd -q -d --long=30 | tar $quiet -xf - -C "$dest"; then
  rm -f "$joined"; echo "ci-assets: the bundle does not decrypt and unpack"; exit 1
fi
rm -f "$joined"
[ -f "$dest/SHA256SUMS" ] || { echo "ci-assets: the bundle has no SHA256SUMS"; exit 1; }
n=$(grep -c . "$dest/SHA256SUMS")
if command -v sha256sum >/dev/null 2>&1; then sum="sha256sum"; else sum="shasum -a 256"; fi
( cd "$dest" && $sum -c --quiet SHA256SUMS >/dev/null 2>&1 ) || { echo "ci-assets: SHA256SUMS does not verify"; exit 1; }
echo "ci-assets: $# part(s), $n files verified against SHA256SUMS"
echo "FORGIV_ASSETS=$dest"
[ -z "${GITHUB_ENV:-}" ] || echo "FORGIV_ASSETS=$dest" >> "$GITHUB_ENV"
