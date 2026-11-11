#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

# Builds a pinned system-header sysroot (glibc 2.39 + Linux 6.8 UAPI headers,
# from the Ubuntu 24.04 "noble" release pocket) for argv-c's `sysroot` config
# key, so generated .i files don't depend on the host's glibc.
#
# Usage: scripts/make-sysroot.sh [outdir]   (default: sysroot/ubuntu-24.04-amd64)
#
# Needs curl, ar, sha256sum (or shasum), and a tar that can read zstd (GNU tar >= 1.31, or
# the zstd binary). Run it anywhere with network access and copy the result to
# the machines that generate benchmarks; the output is host-independent.

set -euo pipefail

OUT="${1:-sysroot/ubuntu-24.04-amd64}"
MIRROR="${UBUNTU_MIRROR:-http://archive.ubuntu.com/ubuntu}"

# libc6-dev and the header-bearing packages it depends on. Release-pocket
# versions, which the archive never removes for the life of the release.
PACKAGES=(
  "pool/main/g/glibc/libc6-dev_2.39-0ubuntu8_amd64.deb 1f03b79223d9cf1a6ed3a648f6942619dcb97b2884dba8fd12e262b6771e247a"
  "pool/main/l/linux/linux-libc-dev_6.8.0-31.31_amd64.deb 21c631ba4da3d38d92a133e28a8a14278e1e00174ed135a20661d1e532e05ca5"
  "pool/main/libx/libxcrypt/libcrypt-dev_4.4.36-4build1_amd64.deb 2edff420ef80b4a3f3751e65c33423ef30e563122a58b759e4854ea8d84ba1b1"
  "pool/main/r/rpcsvc-proto/rpcsvc-proto_1.4.2-0ubuntu7_amd64.deb 7eb710fe148d224c159ddec1ceb0ba53ead52a80a6793dcdae1474acf20d8f71"
)

if [[ -e "$OUT" ]]; then
  echo "error: $OUT already exists; remove it first" >&2
  exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/root"

for entry in "${PACKAGES[@]}"; do
  read -r path sha <<<"$entry"
  deb="$WORK/$(basename "$path")"
  echo "fetching $(basename "$path")"
  curl -sfL "$MIRROR/$path" -o "$deb"
  if command -v sha256sum >/dev/null 2>&1; then
    checker=(sha256sum)
  else
    checker=(shasum -a 256) # macOS ships shasum, not sha256sum
  fi
  echo "$sha  $deb" | "${checker[@]}" -c --status - ||
    { echo "error: checksum mismatch for $(basename "$path")" >&2; exit 1; }
  (cd "$WORK" && ar x "$deb" data.tar.zst)
  if tar --zstd -tf "$WORK/data.tar.zst" >/dev/null 2>&1; then
    tar --zstd -xf "$WORK/data.tar.zst" -C "$WORK/root" ./usr/include
  else
    zstd -dc "$WORK/data.tar.zst" | tar -xf - -C "$WORK/root" ./usr/include
  fi
  rm "$WORK/data.tar.zst" "$deb"
done

mkdir -p "$(dirname "$OUT")"
mv "$WORK/root" "$OUT"
echo "sysroot written to $OUT (set sysroot=$OUT under [File Settings])"
