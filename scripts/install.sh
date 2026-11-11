#!/bin/sh

# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

# No-fuss install: if cmake, ninja, and a compatible clang (LLVM 20+) are
# already on PATH, builds argv-c against them as-is. Otherwise installs them
# via apt (Debian/Ubuntu) or Homebrew (macOS). On any other platform, or if
# apt/brew aren't available, it just reports what's missing rather than
# guessing how to install it
#
# Usage: ./scripts/install.sh [--prefix <dir>] [--clang20] [--sysroot]
#   --clang20  build against exactly Clang 20, installing it via apt/brew unless
#              the clang on PATH already is 20
#   --sysroot  also fetch the pinned SV-COMP system headers (scripts/make-sysroot.sh)
#              into <prefix>/share/argv-c/sysroot; enable per run via ARGV_SYSROOT

set -eu

usage() {
  sed -n 's/^# \{0,1\}//; /^Usage:/,/ARGV_SYSROOT$/p' "$0"
}

prefix=""
want_clang20=0
want_sysroot=0
while [ $# -gt 0 ]; do
  case "$1" in
    --prefix)
      prefix="${2:?--prefix requires a directory}"
      case "$prefix" in /*) ;; *) prefix="$PWD/$prefix" ;; esac # before the cd below
      shift 2
      ;;
    --clang20) want_clang20=1; shift ;;
    --sysroot) want_sysroot=1; shift ;;
    -h | --help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 1 ;;
  esac
done

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

os="$(uname -s)"

have_prereqs() {
  command -v cmake >/dev/null 2>&1 || return 1
  command -v ninja >/dev/null 2>&1 || return 1
  command -v clang >/dev/null 2>&1 || return 1
  command -v clang++ >/dev/null 2>&1 || return 1
  major="$(clang -dumpversion 2>/dev/null | cut -d. -f1)"
  [ -n "$major" ] && [ "$major" -ge 20 ] 2>/dev/null || return 1
  [ "$want_clang20" = 0 ] || [ "$major" = 20 ]
}

print_missing() {
  if [ "$want_clang20" = 1 ]; then
    echo "error: --clang20 needs cmake, ninja, and Clang 20 on PATH." >&2
  else
    echo "error: missing prerequisites (need cmake, ninja, and clang/LLVM 20 or newer)." >&2
  fi
  echo "This script only knows how to install them automatically on Debian/Ubuntu (apt) and macOS (Homebrew)." >&2
  echo "See README.md's Build and Reproducibility sections for manual instructions on '$os'." >&2
}

configure_extra=""

if have_prereqs; then
  echo "==> Using existing toolchain: $(command -v clang) (clang $major)"
  export CC="$(command -v clang)"
  export CXX="$(command -v clang++)"
  if command -v llvm-config >/dev/null 2>&1; then
    configure_extra="-DLLVM_DIR=$(llvm-config --cmakedir)"
  fi
  if [ "$os" = "Darwin" ]; then
    configure_extra="$configure_extra -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)"
  fi
else
  case "$os" in
    Linux)
      if ! command -v apt-get >/dev/null 2>&1; then
        print_missing
        exit 1
      fi

      echo "==> Installing LLVM/Clang 20 toolchain via apt"
      sudo apt-get update
      sudo apt-get install -y cmake ninja-build \
        clang-20 libclang-20-dev libclang-cpp20-dev llvm-20-dev lld-20 \
        zlib1g-dev libzstd-dev libedit-dev

      clang_bin_dir="/usr/lib/llvm-20/bin"
      export CC=clang-20
      export CXX=clang++-20
      export PATH="$clang_bin_dir:$PATH"
      configure_extra="-DLLVM_DIR=$(llvm-config-20 --cmakedir)"
      ;;
    Darwin)
      if ! command -v brew >/dev/null 2>&1; then
        print_missing
        exit 1
      fi

      formula=llvm
      [ "$want_clang20" = 1 ] && formula=llvm@20
      echo "==> Installing LLVM/Clang toolchain via Homebrew ($formula)"
      brew install cmake ninja "$formula" lld

      llvm_prefix="$(brew --prefix "$formula")"
      clang_bin_dir="$llvm_prefix/bin"
      export CC="$clang_bin_dir/clang"
      export CXX="$clang_bin_dir/clang++"
      export PATH="$clang_bin_dir:$PATH"
      configure_extra="-DLLVM_DIR=$llvm_prefix/lib/cmake/llvm -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)"
      ;;
    *)
      print_missing
      exit 1
      ;;
  esac
fi

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

sysroot_dir=""
if [ "$want_sysroot" = 1 ]; then
  install_prefix="${prefix:-$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' build/CMakeCache.txt)}"
  sysroot_dir="$install_prefix/share/argv-c/sysroot/ubuntu-24.04-amd64"
  if [ -d "$sysroot_dir" ]; then
    echo "==> Pinned sysroot already present at $sysroot_dir"
  else
    echo "==> Fetching pinned sysroot"
    staging="build/sysroot-staging"
    rm -rf "$staging"
    bash scripts/make-sysroot.sh "$staging"
    if [ -n "$prefix" ]; then
      mkdir -p "$(dirname "$sysroot_dir")"
      mv "$staging" "$sysroot_dir"
    else
      sudo mkdir -p "$(dirname "$sysroot_dir")"
      sudo mv "$staging" "$sysroot_dir"
    fi
  fi
fi

echo "==> Done. Verifying argv-c runs:"
if [ -n "$prefix" ]; then
  "$prefix/bin/argv-c" || true
else
  argv-c || true
fi

if [ -n "$sysroot_dir" ]; then
  echo
  echo "Pinned sysroot installed. argv-c uses it only when asked, per run:"
  echo "  export ARGV_SYSROOT=\"$sysroot_dir\""
  echo "or set sysroot=$sysroot_dir under [File Settings] in a config file."
fi
