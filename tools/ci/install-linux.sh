#!/bin/sh
# CI only (docs/development/ci.md): the Debian or Ubuntu packages the gates need on Linux x86-64.
# Wine with its 32-bit half (armcc 5.06 and TI CGT 16.9.6 are 32-bit Windows programs), age and
# zstd for the bundle, git and ssh to fetch it, Python 3.11 or later, patch for the mod builds.
# Run as root (the workflow's Debian container) or with sudo available.
set -eu
s=; [ "$(id -u)" = 0 ] || s=sudo
export DEBIAN_FRONTEND=noninteractive
$s dpkg --add-architecture i386
$s apt-get update -qq
$s apt-get install -y -qq --no-install-recommends wine wine64 wine32:i386 age zstd patch \
  python3 python3-venv git openssh-client ca-certificates curl >/dev/null
echo "install-linux: $(wine --version 2>/dev/null), $(python3 --version)"
