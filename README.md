<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

SPDX-License-Identifier: Apache-2.0
-->

# ArgV C Transformer

[![Build and Test](https://github.com/unl-pal/argv-c-transformer/actions/workflows/ci.yaml/badge.svg)](https://github.com/unl-pal/argv-c-transformer/actions/workflows/ci.yaml)
[![License](https://img.shields.io/github/license/unl-pal/argv-c-transformer)](LICENSE)
[![Release](https://img.shields.io/github/v/release/unl-pal/argv-c-transformer)](https://github.com/unl-pal/argv-c-transformer/releases)

ArgV C Transformer takes C source files or directories and converts them into
[SV-Comp](https://sv-comp.sosy-lab.org/) style verification benchmarks. It uses
Clang/LLVM's C++ APIs to parse and rewrite C ASTs according to user-defined
parameters that determine what makes a file and its functions interesting
candidates for verification.

See `argc-benchmarks/` for examples of produced benchmarks.

# Dependencies

This project requires **LLVM/Clang 20**, CMake (>= 3.20), and Ninja.

## macOS (Homebrew)

macOS ships a stripped-down Apple Clang that does not include the linkable
`clang-cpp`/`clang` libraries or `llvm-config` required to build this project.
Install the full LLVM toolchain via Homebrew:

```sh
brew install cmake ninja llvm@20 lld@20
```

`llvm@20` is keg-only (not symlinked into `/opt/homebrew`) so CMake cannot find
it automatically — `CMakeLists.txt` handles this on Apple platforms. No extra
flags are needed when invoking CMake.

## Linux (Debian/Ubuntu)

LLVM 20 is available directly from Ubuntu 24.04's default apt repos:

```sh
sudo apt install cmake ninja-build \
  clang-20 libclang-20-dev libclang-cpp20-dev llvm-20-dev lld-20 \
  zlib1g-dev libzstd-dev libedit-dev
```

When invoking CMake, point it at the versioned compiler:

```sh
CXX=clang++-20 CC=clang-20 cmake -B build -S . -G Ninja
```

## `clang` on `PATH` must match the build

Beyond the build itself, `filter`/`transform` shell out to a bare `clang`
command at runtime (see `ClangToolUtils.hpp` / `Transformer.cpp`) to
preprocess and compile-check each candidate benchmark — this is separate
from, and not guaranteed to match, the LLVM 20 libraries the tools are
linked against. Neither platform puts the versioned LLVM install on `PATH`
by default:

- **Linux**: installing `clang-20` via apt does not repoint the unversioned
  `clang` — that may already point at a different preinstalled version.
  Put the versioned install first on `PATH`:
  ```sh
  export PATH="/usr/lib/llvm-20/bin:$PATH"
  ```
- **macOS**: Homebrew's `llvm@20` is keg-only, so it's never on `PATH`
  automatically:
  ```sh
  export PATH="$(brew --prefix llvm@20)/bin:$PATH"
  ```

Run `clang --version` after exporting to confirm it resolves to LLVM 20
before running `filter`/`transform` — a mismatched `clang` here can silently
produce different benchmark output than the one the project was built and
tested with. CI sets this explicitly for the same reason.

# Build

```sh
cmake -B build -S . -G Ninja
ninja -C build filter transform full
```

# Testing

Build and run the test suite (GoogleTest is fetched automatically by CMake):

```sh
cmake -B build -S . -G Ninja
ninja -C build
ctest --test-dir build
```

Two suites run:

- **`filter_tests`** — unit tests for the filter stage's AST counting
  (`tests/filter/`).
- **`transform_tests`** — golden-file tests for the transform stage
  (`tests/transform/`). Each case is a pair of files in
  `tests/transform/cases/`: `<name>.input.c` is fed through the full transform
  pipeline (include stripping → call havocking → main generation → verifier
  extern injection) and the output must match `<name>.expected.c` exactly.

To add a transform test, drop a new `<name>.input.c` into the cases directory
(support headers can sit alongside; quoted includes resolve there) and generate
its golden:

```sh
UPDATE_GOLDENS=1 ./build/tests/transform_tests
```

Review the generated/changed `.expected.c` files like any other code change —
this is also how goldens are refreshed after an intentional behavior change.

Note: test cases that include system headers are skipped (with an explanatory
message) when the clang resource directory cannot be resolved — this usually
means `clang` is not on `PATH`.

# Downloader (optional)

The downloader fetches C source repositories from GitHub for use as pipeline
input:

```sh
python3 -m venv .venv
source .venv/bin/activate
pip install GitPython
```

Configure the `[File Locations]` section of your config file with a `downloadDir`
pointing to where repositories should be cloned, then run:

```sh
python3 src/download/Downloader.py <config>
```

# Running

All three binaries take up to two positional arguments — an input (a directory
of C files, or a single `.c` file) and/or a config file, in either order.
A directory or `.c` file is treated as the input; any other argument is the
config. Both are optional individually, but at least one is required.

```sh
./build/full <config>              # dirs and thresholds from the config file
./build/full <repo-dir>            # no config needed: built-in defaults,
                                   #   outputs to <repo>-filtered/ and
                                   #   <repo>-benchmarks/ in the working dir
./build/full <repo-dir> <config>   # thresholds from config, input from CLI
./build/filter    <repo-dir>       # filter stage only → <repo>-filtered/
./build/transform <repo>-filtered  # transform stage only → <repo>-benchmarks/
```

A convenience script builds and runs both stages sequentially:

```sh
./run.sh <config>
```

To include the download step, uncomment the relevant lines in `run.sh`.

## Configuration

Config files use INI syntax. See `properties.config` for all available options.
Key sections:

- `[File Locations]` — `databaseDir`, `filterDir`, `benchmarkDir`
- `[Function Requirements]` — per-function thresholds (`minForLoops`, `minTypeIfStmt`, etc.)
- `[File Requirements and Settings]` — `minFileLoC`, `maxFileLoC`, `fileTimeoutSecs`
- `[Debugging Flags]` — `debug`, `debugLevel` (0–3)

The transform stage preprocesses each surviving benchmark into a `.i` file with
`clang -E -P -std=gnu11`, requiring `clang` on `PATH` (see
["`clang` on `PATH` must match the build"](#clang-on-path-must-match-the-build)
above).
