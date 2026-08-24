# Tutorial: Downloading, Filtering, Transforming, and Verifying a Toy Repo

This walks through all four pipeline stages - Download, Filter, Transform,
Verify - end to end on a tiny, purpose-built C project, so you can see
exactly what each stage does to real source before diving into `../Design.md`.

The toy project lives in its own repo,
[`natsteven/argc-example`](https://github.com/natsteven/argc-example). It's
small on purpose: one function with no loops, one function with a struct
parameter, one function that has a loop and calls another in-file function,
and a `main` that ties them together. Each of those choices exercises a
different part of the pipeline.

All commands below are run from the `argv-c-transformer` repo root. Two
config files are checked in for this tutorial:

- `docs/tutorial/download.config` - `Downloader.py`'s own config, used only
  for step 1 below.
- `docs/tutorial/tutorial.config` - read by `filter`/`transform`/`verify`
  (steps 2-4); `Downloader.py` never reads this one.

They're kept separate because `Downloader.py` is a standalone step never
invoked by the C++ pipeline (see `../../README.md`'s Downloader section) and
reads its config with Python's `configparser`, keyed by section, while the
C++ side (`ConfigParser.hpp`) ignores section headers entirely and just
looks for known `key = value` pairs wherever they appear.

Both point at the same paths this tutorial uses
(`docs/tutorial/source-files`, `docs/tutorial/filtered-files`,
`docs/tutorial/transformed-files`, `docs/tutorial/benchmark-files`).

For this tutorial we will use `docs/tutorial/download.config` and
`docs/tutorial/tutorial.config`. The former defines the repo to download
and a destination path, while the latter has intermediate directory
locations as well as complexity threshold and file settings for each stage.
Have a quick look at them to familiarize yourself with some of the available
settings.

## 0. Prerequisites

If you haven't already, follow the steps in the `README` to build the project.
This tutorial is intended to walk through each separate stage to demonstrate
the pipeline, and so we will invoke them separately from the build directory.

## 1. Download

To download this tutorial toy example run the downloader script with the
config path as shown:

```sh
./scripts/downloader.py docs/tutorial/download.config
```

Normally, `downloader.py` reads a CSV index of repositories and clones every
one that meets `language`/`minRepoLoC`/`minNumStars` criteria (see
`README.md`'s Downloader section and `scripts/downloader.config`).
`Downloader.py` alternatively accepts a `repo` setting under `[Downloader]`
that just downloads `repo`, no criteria applied, as seen above.

You'll see some `KeyError: ... On Setting ...` lines in the output. Those
are `Downloader.py` warning that `projectCount`/`csv` aren't set in the
config. That's expected and harmless here: those settings only matter for
the CSV-driven flow, which the `repo` setting bypasses.

The result is the file:

```
docs/tutorial/source-files/natsteven/argc-example/src/example.c
```

Have a look at the source code to get an idea of what it looks like before the
filter and transform stages.

## 2. Filter

```sh
./build/filter docs/tutorial/tutorial.config
```

Have a look at the output:
`docs/tutorial/filtered-files/natsteven/argc-example/src/example.c`:

Three things happened:

- **`add`** - 0 for-loops, fails the `ForLoops = 1,9999` threshold. Its body
  (`{ return a + b; }`) was replaced with `;`, leaving a bare prototype.
- **`print_point`** - has no `for` loop either, but more importantly takes a
  `Point` (struct) parameter. Structs have no `__VERIFIER_nondet_*`
  equivalent, so the parameter-type gate strips it regardless of loop count.
- **`sum_range`** and **`main`** both have a `for` loop, so both keep their
  bodies untouched at this stage; filtering only body-strips, it never
  rewrites the surviving code. Note that `main` is checked against the same
  `ForLoops` threshold as any other function here; it's exempt only from the
  parameter-type gate, not from complexity thresholds (see `../Design.md`'s
  *Main Handling* section).

`add` and `print_point` keep their prototypes (`int add(int a, int b);`, not
a deleted declaration). This matters for the next stage: Transform's
`HavocCallsVisitor` needs the prototype to know `add`'s return type when it
replaces the call inside `sum_range`.

## 3. Transform

```sh
./build/transform docs/tutorial/tutorial.config
```

Now we have `docs/tutorial/transformed-files/natsteven_argc-example_src_example.c`:

Walking through what changed:

- **Havoc calls** - `add(total, i)` inside `sum_range` became
  `__VERIFIER_nondet_int()`. `add` is declared in this file (even though its
  body is gone), so its call sites are havocked rather than kept as-is; the
  `int` return type came from the surviving prototype. `print_point(p)`
  inside the old `main` became an empty statement (`;`) since `print_point`
  returns `void` - a dropped call, not a havocked one.
- **Main generation** - the original `main` was renamed to `original_main`.
  A fresh `int main(void)` was appended that calls every function that still
  has a real body: `sum_range` (with a havocked `int` argument) and
  `original_main` (called with no arguments, since it took no parameters).
  `add` and `print_point` are prototypes only (`isThisDeclarationADefinition`
  is false for both), so they're skipped. They exist only so return types
  could be resolved during havocking.
- **Verifier declaration** - `extern int __VERIFIER_nondet_int(void);` was
  inserted at the top, the one nondet suffix this file ended up needing.
- **Filename flattening** - the nested path
  `natsteven/argc-example/src/example.c` became
  `natsteven_argc-example_src_example.c`: `Transformer::flattenedOutputPath`
  strips the `filterDir` prefix and joins whatever's left with underscores.

Transform is purely source→source - just this one `.c` file is written.
Everything else a benchmark needs (a re-check that havocking didn't shrink a
function below threshold, the compile check, the `.yml` task file, and
preprocessing to `.i`) is the next stage's job.

## 4. Verify

```sh
./build/verify docs/tutorial/tutorial.config
```

Output, in `docs/tutorial/benchmark-files/`:

```
natsteven_argc-example_src_example.c
natsteven_argc-example_src_example.i
natsteven_argc-example_src_example.yml
```

The `.c` here is identical to Transform's output above: Verify reparses
it fresh (Transform's edits are Rewriter text edits, not AST edits, so only a
fresh parse sees the post-transform shape), re-applies the `ForLoops`
threshold per function, and finds nothing to repair - `sum_range` still has
its loop, and the generated `main`/`original_main` are exempt from the
re-check anyway. A file where havocking dropped a function below threshold
would come out repaired or discarded instead.

The `.yml` task file is what the SV-COMP benchmark runner consumes.
`input_files` points at the preprocessed `.i` (produced by
`Verifier::preprocess`), not the `.c` directly. The properties are detected
(`termination` for loops + `no-overflow` for integer arithmetic).

## Where to go next

- `../Design.md` - the design choices, limitations, and downstream-verifier
  quirks behind the pipeline (unsupported types, intraprocedural-by-design,
  pointer-return havocking, etc.)
- `settings.config` at the repo root - every filter/transform/verify setting,
  with defaults documented inline.
- `./build/argv-c <config>` - the same filter → transform → verify sequence
  in one binary, for a real (non-toy) `databaseDir`.
