# Tutorial: Filtering and Transforming a Toy Repo

This walks through the Filter and Transform stages end to end on a tiny,
purpose-built C project, so you can see exactly what each stage does to real
source before diving into `Design.md`.

The toy project lives in its own repo,
[`argv-tutorial-example`](https://github.com/unl-pal/argv-tutorial-example)
(clone URL TBD — see the note at the end of this doc). It's small on purpose:
one function with no loops, one function with a struct parameter, one
function that has a loop and calls another in-file function, and a `main`
that ties them together. Each of those choices exercises a different part of
the pipeline.

## 1. Get the source

Clone the toy repo into a directory the Filter step can scan
(`databaseDir` in your config — see step 2):

```sh
git clone git@github.com:unl-pal/argv-tutorial-example.git source-files/argv-tutorial-example
```

(This replaces the `Download` stage for tutorial purposes — `Download` exists
to pull many repos from a CSV index via the GitHub API; here we just want one
known repo.)

`source-files/argv-tutorial-example/src/example.c`:

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

/* Takes a struct param — no __VERIFIER_nondet_* equivalent exists for
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

## 2. Configure

A minimal config for this walkthrough (see `properties.config` at the repo
root for the full set of options):

```ini
[File Locations]
databaseDir=source-files
filterDir=filtered
benchmarkDir=benchmarks

[Complexity Requirements]
ForLoops = 1,9999

[File Requirements and Settings]
minFileLoC=1
useNonStdHeaders=true
keepCompilesOnly=true

[Debugging Flags]
debugLevel=1
```

`ForLoops = 1,9999` is the only threshold we set: a function needs at least
one `for` loop to survive. That's enough to separate the three functions in
`example.c` into "removed" and "kept" buckets.

## 3. Filter

```sh
./build/filter tutorial.config
```

Output, `filtered/argv-tutorial-example/src/example.c`:

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

/* Takes a struct param — no __VERIFIER_nondet_* equivalent exists for
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

- **`add`** — 0 for-loops, fails the `ForLoops = 1,9999` threshold. Its body
  (`{ return a + b; }`) was replaced with `;`, leaving a bare prototype.
- **`print_point`** — has a `for` loop *nowhere*, but more importantly takes
  a `Point` (struct) parameter. Structs have no `__VERIFIER_nondet_*`
  equivalent, so the parameter-type gate strips it regardless of loop count.
- **`sum_range`** and **`main`** both have a `for` loop, so both keep their
  bodies untouched at this stage — filtering only body-strips, it never
  rewrites the surviving code.

Note that `add` and `print_point` keep their prototypes (`int add(int a, int
b);` not a deleted declaration). This matters for the next stage: Transform's
`HavocCallsVisitor` needs the prototype to know `add`'s return type when it
replaces the call inside `sum_range`.

## 4. Transform

```sh
./build/transform tutorial.config
```

Output, `benchmarks/argv-tutorial-example_src_example.c`:

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

/* Takes a struct param — no __VERIFIER_nondet_* equivalent exists for
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

- **Havoc calls** — `add(total, i)` inside `sum_range` became
  `__VERIFIER_nondet_int()`. `add` is declared in this file (even though its
  body is gone), so its call sites are havocked rather than kept as-is; the
  int return type came from the surviving prototype. `print_point(p)` inside
  the old `main` became an empty statement (`;`) since `print_point` returns
  `void` — a dropped call, not a havocked one.
- **Main generation** — the original `main` was renamed to `original_main`.
  A fresh `int main(void)` was appended that calls every function that still
  has a real body: `sum_range` (with a havocked `int` argument) and
  `original_main` (called with no arguments, since it took no parameters).
  `add` and `print_point` are prototypes only (`isThisDeclarationADefinition`
  is false for both), so they're skipped — they exist only so return types
  could be resolved during havocking.
- **Verifier declaration** — `extern int __VERIFIER_nondet_int(void);` was
  inserted at the top, the one nondet suffix this file ended up needing.

The `.yml` task file emitted alongside it:

```yaml
# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
# SPDX-License-Identifier: Apache-2.0

format_version: '2.0'

input_files: 'argv-tutorial-example_src_example.i'

properties:
  - property_file: ../properties/no-overflow.prp
    expected_verdict: true
  - property_file: ../properties/termination.prp
    expected_verdict: true

options:
  language: C
  data_model: LP64
```

`input_files` points at a preprocessed `.i` file (produced by
`Transformer::transformFile`'s final preprocessing step), not the `.c`
directly — that's the form SV-Comp benchmark runners consume.

## Where to go next

- `Design.md` (same directory) — full pipeline design, every config
  option, and the design choices/limitations behind them (unsupported
  types, intraprocedural-by-design, pointer-return havocking, etc.)
- `properties.config` at the repo root — every filter/transform setting,
  with defaults documented inline.
- `run.sh` — the same filter → transform sequence as a build+run script,
  for a real (non-toy) `databaseDir`.

## Note on the toy repo's remote

This doc references `git@github.com:unl-pal/argv-tutorial-example.git` as a
placeholder. Once the real remote exists, swap the clone URL above for the
real one. A local copy is prepared at `~/Repos/argv-tutorial-example` ready
to push.
