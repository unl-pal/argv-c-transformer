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
# Usage: ./scripts/install.sh [--prefix <dir>]

set -eu

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

prefix=""
if [ "${1:-}" = "--prefix" ]; then
  prefix="${2:?--prefix requires a directory}"
fi

os="$(uname -s)"

have_prereqs() {
  command -v cmake >/dev/null 2>&1 || return 1
  command -v ninja >/dev/null 2>&1 || return 1
  command -v clang >/dev/null 2>&1 || return 1
  command -v clang++ >/dev/null 2>&1 || return 1
  major="$(clang -dumpversion 2>/dev/null | cut -d. -f1)"
  [ -n "$major" ] && [ "$major" -ge 20 ] 2>/dev/null
}

print_missing() {
  echo "error: missing prerequisites (need cmake, ninja, and clang/LLVM 20 or newer)." >&2
  echo "This script only knows how to install them automatically on Debian/Ubuntu (apt) and macOS (Homebrew)." >&2
  echo "See README.md's Build section for manual instructions on '$os'." >&2
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

      export CC=clang-20
      export CXX=clang++-20
      export PATH="/usr/lib/llvm-20/bin:$PATH"
      configure_extra="-DLLVM_DIR=$(llvm-config-20 --cmakedir)"
      ;;
    Darwin)
      if ! command -v brew >/dev/null 2>&1; then
        print_missing
        exit 1
      fi

      echo "==> Installing LLVM/Clang toolchain via Homebrew"
      brew install cmake ninja llvm lld

      llvm_prefix="$(brew --prefix llvm)"
      export CC="$llvm_prefix/bin/clang"
      export CXX="$llvm_prefix/bin/clang++"
      export PATH="$llvm_prefix/bin:$PATH"
      configure_extra="-DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)"
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

echo "==> Done. Verifying argv-c runs:"
if [ -n "$prefix" ]; then
  "$prefix/bin/argv-c" || true
else
  argv-c || true
fi

cat <<'EOF'

Note: argv-c needs a `clang` resolvable on PATH at runtime (to ask
it for its resource directory), and it must be Clang 20+
EOF
