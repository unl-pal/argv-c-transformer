#!/bin/sh

# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

# No-fuss install: detects the platform, installs the pinned LLVM/Clang 20
# toolchain via apt or brew, builds argv-c, and installs it.
#
# There is no prebuilt binary because argv-c dynamically links libclang-cpp/
# libLLVM at runtime *and* shells out to a bare `clang` for preprocessing
# (see README.md, "`clang` on `PATH` must match the build") - a downloaded
# binary would still require the user to separately install a matching LLVM
# 20, so building from source with the right toolchain is the actual
# no-friction path.
#
# Usage: ./scripts/install.sh [--prefix <dir>]
#
# Run from a clone of this repo. Requires sudo (Linux) for apt-get and for
# installing to the default prefix (/usr/local).

set -eu

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

prefix=""
if [ "${1:-}" = "--prefix" ]; then
  prefix="${2:?--prefix requires a directory}"
fi

os="$(uname -s)"

case "$os" in
  Linux)
    if ! command -v apt-get >/dev/null 2>&1; then
      echo "error: apt-get not found. This script only supports Debian/Ubuntu on Linux." >&2
      echo "See README.md's Build section for manual instructions on other distros." >&2
      exit 1
    fi

    echo "==> Installing LLVM/Clang 20 toolchain via apt"
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build \
      clang-20 libclang-20-dev libclang-cpp20-dev llvm-20-dev lld-20 \
      zlib1g-dev libzstd-dev libedit-dev

    export CC=clang-20
    export CXX=clang++-20
    versioned_bin="/usr/lib/llvm-20/bin"
    export PATH="$versioned_bin:$PATH"
    configure_extra="-DLLVM_DIR=$(llvm-config-20 --cmakedir)"
    ;;
  Darwin)
    if ! command -v brew >/dev/null 2>&1; then
      echo "error: Homebrew not found. Install it from https://brew.sh first." >&2
      exit 1
    fi

    echo "==> Installing LLVM/Clang 20 toolchain via Homebrew"
    brew install cmake ninja llvm@20 lld@20

    versioned_bin="$(brew --prefix llvm@20)/bin"
    export PATH="$versioned_bin:$PATH"
    configure_extra="-DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)"
    ;;
  *)
    echo "error: unsupported platform '$os'. This script only supports Linux (apt) and macOS (brew)." >&2
    echo "See README.md's Build section for manual instructions." >&2
    exit 1
    ;;
esac

echo "==> Configuring"
# shellcheck disable=SC2086
cmake -B build -S . -G Ninja $configure_extra

echo "==> Building"
ninja -C build

echo "==> Installing argv-c"
if [ -n "$prefix" ]; then
  cmake --install build --prefix "$prefix"
else
  sudo cmake --install build
fi

echo "==> Done. Verifying argv-c runs:"
if [ -n "$prefix" ]; then
  "$prefix/bin/argv-c" || true
else
  argv-c || true
fi

cat <<EOF

Note: argv-c shells out to a bare 'clang' at runtime and requires it to
resolve to Clang 20+ on PATH. This script put the versioned toolchain first
on PATH for this session only - add the line below to your shell rc file
to make it permanent:

  export PATH="$versioned_bin:\$PATH"
EOF
