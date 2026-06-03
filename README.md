# ArgV C Transformer

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


# Build

```sh
cmake -B build -S . -G Ninja
ninja -C build filter transform full
```

Before running any binary, set the Clang resource directory — required for the
AST pipeline to resolve standard library headers in processed files:

```sh
export CLANG_RESOURCES=$(clang -print-resource-dir)
```

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

All three binaries take a config file as their sole argument:

```sh
./build/filter    <config>   # filter stage only
./build/transform <config>   # transform stage only
./build/full      <config>   # filter then transform
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
- `[File Requirements and Settings]` — `type`, `minFileLoC`, `useNonStdHeaders`, `keepCompilesOnly`
- `[Debugging Flags]` — `debug`, `debugLevel` (0–3)
