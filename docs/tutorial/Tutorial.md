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
looks for known `key = value` pairs wherever they appear. Feeding
`Downloader.py`'s `[Downloader]` section to the C++ tools would just print a
harmless but confusing `Unknown config key: repo`.

Both point at the same paths this tutorial uses
(`docs/tutorial/source-files`, `docs/tutorial/filtered-files`,
`docs/tutorial/transformed-files`, `docs/tutorial/benchmark-files` - all
gitignored, so running this tutorial won't leave anything to clean up in
`git status`).

`docs/tutorial/download.config`:

```ini
[File Locations]
# Must match tutorial.config's databaseDir, since filter reads from there.
databaseDir=docs/tutorial/source-files

[Downloader]
# A single "owner/name" repo, bypassing the CSV-index flow entirely (see
# Downloader.py's `repo` setting). Good for a known, deliberately-chosen
# repo like this tutorial's toy example.
repo=natsteven/argc-example
```

`docs/tutorial/tutorial.config`:

```ini
[Stage Directories]
databaseDir=docs/tutorial/source-files
filterDir=docs/tutorial/filtered-files
transformDir=docs/tutorial/transformed-files
benchmarkDir=docs/tutorial/benchmark-files

[Complexity Requirements]
# A function needs at least one for-loop to survive filtering. This is the
# only threshold set here - enough to split argc-example's three functions
# into "removed" and "kept" buckets.
ForLoops = 1,9999

[File Settings]
FileLoC=1,9999
keepCompilesOnly=true
debugLevel=1
```

## 1. Download

```sh
./scripts/downloader.py docs/tutorial/download.config
```

Normally `Downloader.py` reads a CSV index of repositories and clones every
one that meets `language`/`minRepoLoC`/`minNumStars` criteria (see
`../../README.md`'s Downloader section). That's the wrong tool for "grab this
one specific repo I already picked", so `Downloader.py` also accepts a `repo`
setting under `[Downloader]` (an `"owner/name"` string) that skips the CSV
entirely and downloads just that repo, no criteria applied. That's what
`docs/tutorial/download.config` uses above.

You'll see some `KeyError: ... On Setting ...` lines in the output. Those
are `Downloader.py` warning that `projectCount`/`csv` aren't set in the
config. That's expected and harmless here: those settings only matter for
the CSV-driven flow, which the `repo` setting bypasses.

Result:

```
docs/tutorial/source-files/natsteven/argc-example/src/example.c
```

`databaseDir` serves double duty: the downloader writes into
`databaseDir/<owner>/<name>/...`, and the Filter step scans that same
`databaseDir` for what Download fetched.

`.../example.c`:

```c
#include <stdio.h>

typedef struct Point {
    int x;
    int y;
} Point;

/* Trivial helper: no loops, so the filter's ForLoops threshold strips it. */
int add(int a, int b) {
    return a + b;
}

/* Has a for-loop, so it survives filtering. Calls add(), which becomes a
 * havoc target in the transform step. */
int sum_range(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total = add(total, i);
    }
    return total;
}

/* Takes a struct param - no __VERIFIER_nondet_* equivalent exists for
 * aggregate types, so the filter's parameter-type gate strips this body
 * even though it isn't touched by the loop threshold. */
void print_point(Point p) {
    printf("(%d, %d)\n", p.x, p.y);
}

int main(void) {
    int result = sum_range(10);
    for (int i = 0; i < 3; i++) {
        Point p = {i, i};
        print_point(p);
    }
    printf("Result: %d\n", result);
    return 0;
}
```

## 2. Filter

```sh
./build/filter docs/tutorial/tutorial.config
```

Output, `docs/tutorial/filtered-files/natsteven/argc-example/src/example.c`:

```c
#include <stdio.h>

typedef struct Point {
    int x;
    int y;
} Point;

/* Trivial helper: no loops, so the filter's ForLoops threshold strips it. */
int add(int a, int b) ;

/* Has a for-loop, so it survives filtering. Calls add(), which becomes a
 * havoc target in the transform step. */
int sum_range(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total = add(total, i);
    }
    return total;
}

/* Takes a struct param - no __VERIFIER_nondet_* equivalent exists for
 * aggregate types, so the filter's parameter-type gate strips this body
 * even though it isn't touched by the loop threshold. */
void print_point(Point p) ;

int main(void) {
    int result = sum_range(10);
    for (int i = 0; i < 3; i++) {
        Point p = {i, i};
        print_point(p);
    }
    printf("Result: %d\n", result);
    return 0;
}
```

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

Output, `docs/tutorial/transformed-files/natsteven_argc-example_src_example.c`:

```c
#include <stdio.h>

extern int __VERIFIER_nondet_int(void);

typedef struct Point {
    int x;
    int y;
} Point;

/* Trivial helper: no loops, so the filter's ForLoops threshold strips it. */
int add(int a, int b) ;

/* Has a for-loop, so it survives filtering. Calls add(), which becomes a
 * havoc target in the transform step. */
int sum_range(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total = __VERIFIER_nondet_int();
    }
    return total;
}

/* Takes a struct param - no __VERIFIER_nondet_* equivalent exists for
 * aggregate types, so the filter's parameter-type gate strips this body
 * even though it isn't touched by the loop threshold. */
void print_point(Point p) ;

int original_main(void) {
    int result = __VERIFIER_nondet_int();
    for (int i = 0; i < 3; i++) {
        Point p = {i, i};
        ;
    }
    printf("Result: %d\n", result);
    return 0;
}

int main(void) {
  sum_range(__VERIFIER_nondet_int());
  original_main();
  return 0;
}
```

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

The `.c` here is byte-identical to Transform's output above: Verify reparses
it fresh (Transform's edits are Rewriter text edits, not AST edits, so only a
fresh parse sees the post-transform shape), re-applies the `ForLoops`
threshold per function, and finds nothing to repair - `sum_range` still has
its loop, and the generated `main`/`original_main` are exempt from the
re-check anyway. A file where havocking dropped a function below threshold
would come out repaired or discarded instead; see *Empty Benchmark Discard*
in `../Design.md`.

The `.yml` task file:

```yaml
format_version: '2.0'

input_files: 'natsteven_argc-example_src_example.i'

properties:
  - property_file: ../properties/termination.prp
    expected_verdict: true
  - property_file: ../properties/no-overflow.prp
    expected_verdict: true

options:
  language: C
  data_model: LP64
```

`input_files` points at the preprocessed `.i` (produced by
`Verifier::preprocess`), not the `.c` directly; that's the form SV-Comp
benchmark runners consume. The property set here (`termination` +
`no-overflow`) is currently fixed for every benchmark - see `selectProperties`
in `../Design.md`/`../../CLAUDE.md` for where per-function counts could drive
this in the future.

## Where to go next

- `../Design.md` - the design choices, limitations, and downstream-verifier
  quirks behind the pipeline (unsupported types, intraprocedural-by-design,
  pointer-return havocking, etc.)
- `settings.config` at the repo root - every filter/transform/verify setting,
  with defaults documented inline.
- `./build/argv-c <config>` - the same filter → transform → verify sequence
  in one binary, for a real (non-toy) `databaseDir`.
