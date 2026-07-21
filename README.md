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

## Contents

- [Install](#install)
  - [Download a prebuilt package](#download-a-prebuilt-package)
  - [Build from source](#build-from-source)
- [Running](#running)
  - [Configuration](#configuration)
- [Build](#build)
  - [macOS (Homebrew)](#macos-homebrew)
  - [Linux (Debian/Ubuntu)](#linux-debianubuntu)
  - [`clang` on `PATH` must match the build](#clang-on-path-must-match-the-build)
- [Testing](#testing)
- [Downloader (optional)](#downloader-optional)

# Install

`argv-c` is the one binary most users need — it runs the whole filter →
transform → verify pipeline (see [Running](#running)). Get it either of two
ways:

## Download a prebuilt package

Grab the archive for your platform from the
[Releases page](https://github.com/unl-pal/argv-c-transformer/releases):

- Linux (x86_64): `argv-c-<version>-linux-x86_64.tar.gz`
- macOS (Apple Silicon): `argv-c-<version>-macos-arm64.tar.gz`

then:

```sh
tar xzf argv-c-<version>-<platform>.tar.gz
sudo mv argv-c /usr/local/bin/   # or anywhere else on PATH
```

Packages don't bundle LLVM/Clang — `argv-c` still shells out to a bare
`clang` at runtime for preprocessing and compile-checking (see
["`clang` on `PATH` must match the build"](#clang-on-path-must-match-the-build)),
so either way you need Clang 20+ on `PATH`. `argv-c` checks this itself at
startup and tells you what to do if it's missing.

No package for your platform (e.g. Windows, Linux ARM)? Build from source
instead.

## Build from source

Clone the repo, then see [Build](#build) below for dependencies and build
steps, then:

```sh
cmake --install build
```

installs `argv-c` to the standard CMake prefix (`/usr/local` by default).
Pass `--prefix <dir>` to install elsewhere, e.g. `cmake --install build --prefix ~/.local`.
The individual stage binaries (`filter`/`transform`/`verify`) aren't
installed; run them out of `build/` if you need them (see
[Running](#running)).

# Running

All four binaries take up to two positional arguments — an input (a directory
of C files, or a single `.c` file) and/or a config file, in either order.
A directory or `.c` file is treated as the input; any other argument is the
config. Both are optional individually, but at least one is required.
Examples below assume `argv-c` is installed (see [Install](#install));
otherwise run it as `./build/argv-c`.

`argv-c` runs the whole filter → transform → verify pipeline in one process
and produces just the final benchmarks — the intermediate `-filtered`/
`-transformed` directories are cleaned up once verify finishes, unless the
config file explicitly names `filterDir`/`transformDir`, which is taken as a
request to keep them around:

```sh
argv-c <config>              # dirs and thresholds from the config file
argv-c <repo-dir>            # no config needed: built-in defaults,
                             #   output goes to <repo>-benchmarks/ in the
                             #   working directory
argv-c <repo-dir> <config>   # thresholds from config, input from CLI
```

Run a single stage on its own — useful for development, e.g. iterating on
one stage without re-running the others. These aren't installed, so run them
straight out of `build/`:

```sh
./build/filter    <repo-dir>          # filter stage only    → <repo>-filtered/
./build/transform <repo>-filtered     # transform stage only → <repo>-transformed/
./build/verify    <repo>-transformed  # verify stage only    → <repo>-benchmarks/
```

## Configuration

Config files use INI syntax. See `settings.config` for all available options.
Key sections:

- `[File Locations]` — `databaseDir`, `filterDir`, `benchmarkDir`
- `[Function Requirements]` — per-function thresholds (`minForLoops`, `minTypeIfStmt`, etc.)
- `[File Settings]` — `FileLoC`, `fileTimeoutSecs`, `keepCompilesOnly`, `debugLevel` (0–3)

The transform stage preprocesses each surviving benchmark into a `.i` file with
`clang -E -P -std=gnu11`, requiring `clang` on `PATH` (see
["`clang` on `PATH` must match the build"](#clang-on-path-must-match-the-build)
below).

# Build

Check what you already have before installing anything — `clang --version`
tells you the version currently resolved on `PATH`. This project requires
**LLVM/Clang 20**, CMake (>= 3.20), and Ninja; if you're already on 20 or
newer, skip straight to the build commands below. `verify` and `argv-c` (the
tools that shell out to `clang` at runtime — see
["`clang` on `PATH` must match the build"](#clang-on-path-must-match-the-build))
check this for you at startup too, and will only complain if what's on
`PATH` doesn't meet the minimum.

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

Beyond the build itself, `verify` (and therefore `argv-c`) shells out to a
bare `clang` command at runtime (see `ClangToolUtils.hpp` / `Verifier.cpp`) to
preprocess and compile-check each candidate benchmark — this is separate
from, and not guaranteed to match, the LLVM 20 libraries the tools are linked
against. `verify`/`argv-c` check this at startup and refuse to run if `PATH`
doesn't resolve to Clang 20+, so a mismatch here is caught immediately rather
than silently producing different benchmark output than the one the project
was built and tested with — but you still need to fix your `PATH` to get
past it. Neither platform puts the versioned LLVM install on `PATH` by
default:

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

CI sets this explicitly for the same reason.

```sh
cmake -B build -S . -G Ninja
ninja -C build
```

This builds all four binaries — `argv-c` plus the individual `filter`/
`transform`/`verify` stages (useful for development: iterating on one stage
without re-running the others). They're runnable straight out of `build/`,
e.g. `./build/filter <repo-dir>`. See [Install](#install) to install just
`argv-c`.

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

Downloader.py has its own config, separate from `settings.config` (which
the filter/transform/verify/argv-c pipeline reads) since `argv-c` never
invokes the downloader — it's a standalone step you run first to populate a
`databaseDir` for the pipeline to later read as input. See
`src/download/downloader.config` for the default, or write your own with a
`[File Locations]` `databaseDir` pointing to where repositories should be
cloned, then run:

```sh
python3 src/download/Downloader.py src/download/downloader.config
```

This reads a CSV index of repositories (`csv` setting, default `repos.csv`),
applies the `[Downloader]` section's criteria (CSV column filters like
`language`, `stars`, `size`), and stops after `projectCount` repos.

Alternatively, pass a `.csv` file directly instead of a `.config`:

```sh
python3 src/download/Downloader.py <repos.csv>
```

This treats the file as a plain list of repos (its `repository` column) and
downloads every row unconditionally, with no filtering, into the default
`database/` directory.
