# ArgV C Transformer

ArgV C Transformer takes C source files or directories and converts them into
[SV-Comp](https://sv-comp.sosy-lab.org/) style verification benchmarks. It uses
Clang/LLVM's C++ APIs to parse and rewrite C ASTs according to user-defined
parameters that determine what makes a file and its functions interesting
candidates for verification.

See `argc-benchmarks/` for examples of produced benchmarks.

# Dependencies

**Build:**
- LLVM/Clang developer toolkit (`llvm-devel`, `clang-devel`)
- CMake (>= 3.30)
- Ninja

**Downloader (optional):**
- Python 3
- GitPython (`pip install GitPython`)

# Build

```sh
cmake -B build -S . -G Ninja
ninja -C build filter transform full
```

Before running any binary, set the Clang resource directory:

```sh
export CLANG_RESOURCES=$(clang -print-resource-dir)
```

This is required for the AST pipeline to correctly resolve standard library
headers in the files being processed.

# Downloader

The downloader fetches C source repositories from GitHub for use as pipeline
input. To set it up:

```sh
python3 -m venv .venv
source .venv/bin/activate
pip install GitPython
```

Then configure the `[File Locations]` section of your config file with a
`downloadDir` pointing to where repositories should be cloned, and run:

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

A convenience script is provided that builds and runs both stages sequentially:

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
