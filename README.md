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
candidates for verification. In addition, we provide a python [downloader](scripts/downloader.py)
script to aid in downloading and filtering open-source Github repositories.
Otherwise the tool can be pointed to any local repositories as the user desires.

## Contents

- [Install](#install)
  - [Quick install (script)](#quick-install-script)
  - [Build from source](#build-from-source)
- [Running](#running)
  - [Configuration](#configuration)
- [Build](#build)
  - [macOS (Homebrew)](#macos-homebrew)
  - [Linux (Debian/Ubuntu)](#linux-debianubuntu)
  - [`clang` on `PATH` must match the build](#clang-on-path-must-match-the-build)
- [Testing](#testing)
- [Downloader (optional)](#downloader-optional)
- [Repository Layout](#repository-layout)

# Install

`argv-c` is the one binary most users need. It runs the whole filter ->
transform -> verify pipeline (see [Running](#running)). Get it either of two
ways:

## Quick install (script)

There's no prebuilt binary: `argv-c` dynamically links `libclang-cpp`/`libLLVM`
at runtime and separately shells out to a bare `clang` for preprocessing and
compile-checking (see
["`clang` on `PATH` must match the build"](#clang-on-path-must-match-the-build)),
so a downloaded binary would still require you to separately install a
matching LLVM 20 - it wouldn't actually save you the setup step. Instead,
clone the repo and run the install script, which detects your platform,
installs the pinned LLVM/Clang 20 toolchain (via `apt` on Linux or `brew` on
macOS), and builds and installs `argv-c` for you:

```sh
git clone https://github.com/unl-pal/argv-c-transformer
cd argv-c-transformer
./scripts/install.sh              # installs to the default CMake prefix
./scripts/install.sh --prefix ~/.local   # or install elsewhere
```

Only Debian/Ubuntu (`apt`) and macOS (`brew`) are supported by the script.
On other platforms, or if you'd rather manage the toolchain yourself, follow
[Build from source](#build-from-source) below.


## Build from source

Clone the repo, then see [Build](#build) below for dependencies and build
steps, then:

```sh
cmake --install build
```

This installs `argv-c` to the standard CMake prefix (`/usr/local` by default).
Pass `--prefix <dir>` to install elsewhere, e.g. `cmake --install build --prefix ~/.local`.

# Running

`argv-c` takes up to two positional arguments: an input path (directory
of C files, or a single `.c` file) and/or a config file, in either order.
At least one is required. Examples below assume `argv-c` is installed.

`argv-c` runs the pipeline and outputs final benchmarks in `<input>-benchmarks`.
Intermediate `-filtered`/ `-transformed` directories are cleaned unless the
config file explicitly names them under \[Stage Directories\].

```sh
argv-c <config>              # dirs and thresholds from the config file
argv-c <repo-dir>            # no config needed: built-in defaults,
                             #   output goes to <repo>-benchmarks/ in the
                             #   working directory
argv-c <repo-dir> <config>   # thresholds from config, input from CLI
```

Users can run single stages if building from source.

```sh
./build/filter    <repo-dir>          # filter stage only    → <repo>-filtered/
./build/transform <repo>-filtered     # transform stage only → <repo>-transformed/
./build/verify    <repo>-transformed  # verify stage only    → <repo>-benchmarks/
```

## Configuration

Config files use INI syntax. Any `*.config` file is accepted with the following
keys. See `settings.config` for more info.

- `[Complexity Requirements]` - per-function `min,max` thresholds: `ForLoops`, `WhileLoops`, `IfStmt`, `CallFunc`, `Param`
- `[Feature Requirements]` - per-function gates: `require` | `forbid` | `ignore` (default): `Concurrency` and `FloatingPoint`
- `[File Settings]` - `FileLoC`, `fileTimeoutSecs`, `keepCompilesOnly`, `debugLevel` (0–3)
- `[Stage Directories]` - `databaseDir`, `filterDir`, `transformDir`, `benchmarkDir`

# Build

This project uses CMake and Ninja. To build it run:

```sh
cmake -B build -S . -G Ninja
ninja -C build
```

Each stage's binary and the full `argv-c` version can then be found and run from
the `build/` directory.

Because this project requires **LLVM/Clang 20**, CMake (>= 3.20) you may need
to perform additional setup. If you're already on 20 or newer the above build commands
may have worked. `clang --version` tells you the version currently resolved on `PATH`.
See [clang-on-path-must-match-the-build](#clang-on-path-must-match-the-build) for more information.
Platform-specific instructions are below.

## macOS (Homebrew)

macOS ships a stripped-down Apple Clang that does not include the linkable
`clang-cpp`/`clang` libraries or `llvm-config` required to build this project.
Install the full LLVM toolchain via Homebrew:

```sh
brew install cmake ninja llvm@20 lld@20
```

`llvm@20` is keg-only (not symlinked into `/opt/homebrew`) so CMake cannot find
it automatically.  `CMakeLists.txt` handles this on Apple platforms. No extra
flags are needed when invoking CMake. Now the above build commands should work.

## Linux (Debian/Ubuntu)

LLVM 20 is available directly from Ubuntu 24.04's default apt repos:

```sh
sudo apt install cmake ninja-build \
  clang-20 libclang-20-dev libclang-cpp20-dev llvm-20-dev lld-20 \
  zlib1g-dev libzstd-dev libedit-dev
```

When invoking CMake, you''d want to point it at the versioned compiler:

```sh
CXX=clang++-20 CC=clang-20 cmake -B build -S . -G Ninja
```

`CXX`/`CC` only need to be set for this one invocation since CMake caches the
compiler choice in `build/`. Now you should be able to run `ninja -C build` successfully.

## `clang` on `PATH` must match the build

Beyond the build itself, `argv-c` shells out to a
bare `clang` command at runtime (see `ClangToolUtils.hpp` / `Verifier.cpp`) to
preprocess and compile-check each candidate benchmark. This is separate
from, and not guaranteed to match, the LLVM 20 libraries the tools are linked
against. `verify`/`argv-c` check this at startup and refuse to run if `PATH`
doesn't resolve to Clang 20+.

- **Linux**: installing `clang-20` via apt does not repoint the unversioned
  `clang`, which may already point at a different preinstalled version.
  Put the versioned install first on `PATH`:

  ```sh
  export PATH="/usr/lib/llvm-20/bin:$PATH"
  ```

- **macOS**: Homebrew's `llvm@20` is keg-only, so it's never on `PATH`
  automatically:

  ```sh
  export PATH="$(brew --prefix llvm@20)/bin:$PATH"
  ```

# Testing

After building, run the test suite (GoogleTest is fetched automatically by CMake):

```sh
ctest --test-dir build
```

Two suites run:

- **`filter_tests`** - unit tests for the filter stage's AST counting
  (`tests/filter/`).
- **`transform_tests`** - golden-file tests for the transform stage
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

Review the generated/changed `.expected.c` files like any other code change -
this is also how goldens are refreshed after an intentional behavior change.

Note: test cases that include system headers are skipped (with an explanatory
message) when the clang resource directory cannot be resolved. This usually
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
invokes the downloader. It's a standalone step you run first to populate a
`databaseDir` for the pipeline to later read as input. See
`scripts/downloader.config` for the default, or write your own with a
`[File Locations]` `databaseDir` pointing to where repositories should be
cloned, then run:

```sh
./scripts/downloader.py scripts/downloader.config
```

This uses a CSV index of repositories (`csv` setting, default `repos.csv`),
and applies the `[Downloader]` section's criteria (CSV column filters like
`language`, `stars`, `size`), and stops after `projectCount` repos.
Alternatively, you can use the repo key to pass a single repo to download. This
downloads the tarball and only extracts the `*.c/*.h` files.

Alternatively, pass a `.csv` file directly instead of a `.config`:

```sh
./scripts/downloader.py <repos.csv>
```

This treats the file as a plain list of repos (its `repository` column) and
downloads every row unconditionally, with no filtering, into the default
`repos/` directory.

# Repository Layout

- `src/` - the C++ pipeline (filter/transform/verify/full stages, plus shared
  headers under `src/common/`).
- `scripts/` - standalone tooling that isn't part of the C++ build:
  `install.sh` ([Quick install](#quick-install-script)) and
  `downloader.py`/`downloader.config` ([Downloader](#downloader-optional)).
- `properties/` - SV-Comp property files (`.prp`, with a `.md` description
  each) that generated benchmarks' `.yml` task files point to, e.g.
  `unreach-call.prp`, `termination.prp`.
- `tests/` - the CMake-driven test suite (`ctest --test-dir build`); see
  [Testing](#testing).
- `docs/` - `Design.md` (design rationale and known limitations) and a
  worked-example tutorial (`docs/tutorial/`).
- `settings.config` - the default pipeline config (filter/transform/verify
  thresholds); see [Configuration](#configuration).
- `repos.csv` - the default CSV index of repositories the downloader reads;
  see [Downloader](#downloader-optional).
