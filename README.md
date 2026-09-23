<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

SPDX-License-Identifier: Apache-2.0
-->

# ArgV C Transformer

[![Build and Test](https://github.com/unl-pal/argv-c-transformer/actions/workflows/ci.yaml/badge.svg)](https://github.com/unl-pal/argv-c-transformer/actions/workflows/ci.yaml)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue)](LICENSE)
[![Release](https://img.shields.io/github/v/release/unl-pal/argv-c-transformer)](https://github.com/unl-pal/argv-c-transformer/releases)

ArgV C Transformer takes C source files or directories and converts them into
[SV-Comp](https://sv-comp.sosy-lab.org/) style verification benchmarks. It uses
Clang/LLVM's C++ APIs to parse and rewrite C ASTs according to user-defined
parameters that determine what makes a file and its functions interesting
candidates for verification. In addition, we provide a python [downloader](scripts/downloader.py)
script to aid in downloading and filtering open-source Github repositories.
Otherwise the tool can be pointed to any local repositories as the user desires.

## Contents

- [Setup](#setup)
  - [Quick build and install (script)](#quick-build-and-install-script)
  - [Prequisites](#build-prequisites)
  - [Build](#build)
- [Running](#running)
  - [Configuration](#configuration)
- [Testing](#testing)
- [Downloader (optional)](#downloader-optional)
- [Repository Layout](#repository-layout)

## Setup

`argv-c` is the one binary most users need. It runs the whole filter ->
transform -> verify pipeline (see [Running](#running)). For simplicity an
install script is provided and more granular control, build instructions
are below.

### Quick build and install (script)

There's no prebuilt binary because `argv-c` dynamically links `libclang-cpp`/`libLLVM`
at runtime. Instead, clone the repo and run the install script, which checks
whether a compatible clang (LLVM 20+) is already on `PATH` and, if so, builds
and installs `argv-c` for you.

```sh
git clone https://github.com/unl-pal/argv-c-transformer
cd argv-c-transformer
./scripts/install.sh              # installs to the default CMake prefix
./scripts/install.sh --prefix ~/.local   # or install elsewhere
```

### Prequisites

The project requires clang/LLVM 20 or newer (`CMakeLists.txt` enforces this at
configure time) - below are some examples for different platforms.

**macOS**

```sh
brew install cmake ninja llvm lld
```

**Linux (Debian/Ubuntu)**

```sh
sudo apt install cmake ninja-build \
  clang-20 libclang-20-dev libclang-cpp20-dev llvm-20-dev lld-20 \
  zlib1g-dev libzstd-dev libedit-dev
```

**Arch Linux (pacman)**

```sh
sudo pacman -S cmake ninja clang lld
```

**Fedora (dnf)**

```sh
sudo dnf install cmake ninja-build clang clang-devel llvm-devel lld \
  zlib-devel libzstd-devel libedit-devel
```

Other package managers should have LLVM/Clang 20+ available too

### Build

This project uses CMake and Ninja. To build it run:

```sh
cmake -B build -S . -G Ninja
ninja -C build
```

Each stage's binary and the full `argv-c` version can then be found and run from
the `build/` directory. Additionally, it is recommended to put argv-c on your path:

```sh
cmake --install build
```

This installs `argv-c` to the standard CMake prefix (`/usr/local` by default).
Pass `--prefix <dir>` to install elsewhere, e.g. `cmake --install build --prefix ~/.local`.

# Running

`argv-c` takes up to two positional arguments: an input path (directory
of C files, or a single `.c` file) and/or a config file, in either order.
At least one is required. Starting off users will likely want to run
the pipeline on some repo(s) without filtering to see what kind of
benchmarks are generated.

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

Configuration allows the user to set thresholds for filtering and other choices
that affect the generated benchmarks. Config files use INI syntax. Any
positional argument that isn't a directory or a `.c` file is treated as the
config file, regardless of its name or extension. See `settings.config` for
more info.

- `[Complexity Requirements]` - per-function `min,max` thresholds: `ForLoops`, `WhileLoops`, `IfStmt`, `CallFunc`, `Param`, `Operations`
- `[Feature Requirements]` - per-function gates: `require` | `forbid` | `ignore` (default): `Concurrency`, `FloatingPoint`, `PointerOrArray`, `PointerDeref`, `MemAlloc`, `MemFree`
- `[File Settings]` - `FileLoC`, `fileTimeoutSecs`, `nproc` (files processed concurrently per stage; 0 = auto, three quarters of detected cores; higher values are capped at the core count), `keepCompilesOnly`, `debugLevel` (0–3)
- `[Havoc Settings]` - bounds emitted as `__HAVOC_*` macros into each benchmark: `havocArgcMin`, `havocArgcMax`, `havocStrMax`, `havocBlockMax`, `havocArrayElems`
- `[Stage Directories]` - `databaseDir`, `filterDir`, `transformDir`, `benchmarkDir`

# Testing

After building you can run the test suite (GoogleTest is fetched automatically by CMake):

```sh
ctest --test-dir build
```

Four suites run:

- **`filter_tests`** - unit tests for the filter stage's AST counting
  (`tests/filter/`), plus the shared-code unit tests in `tests/common/`
  (`ConfigParser`, `HavocPolicy`, `IncludeIndex`, `WorkerPool`,
  `ClangToolUtils`).
- **`transform_tests`** - golden-file tests for the transform stage
  (`tests/transform/`). Each case is a pair of files in
  `tests/transform/cases/`: `<name>.input.c` is fed through the full transform
  pipeline (include stripping → call havocking → main generation → runtime
  header include) and the output must match `<name>.expected.c` exactly.
- **`transform_stage_tests`** - end-to-end tests driving the transform stage
  entry point directly (`tests/transform/`).
- **`verify_stage_tests`** - end-to-end tests for the verify stage, including
  header closure behavior (`tests/verify/`).

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
input. First setup the environment and install the `GitPython` dependency:

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
