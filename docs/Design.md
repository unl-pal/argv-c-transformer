<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

SPDX-License-Identifier: Apache-2.0
-->

# ArgV C Transformer - Design

This document is intended to describe the pipeline's design choices, the
trade-offs and roadblocks that motivated them, and what's explicitly unsupported.
For what the pipeline does and how to run it, see [`README.md`](../README.md).

Download, Filter, Transform, and Verify are four separate steps, not one
uniformly-configured pipeline: Download reads its own config
(`downloader.config`) and is never invoked by `argv-c`, while Filter,
Transform, and Verify share `settings.config` via `parsePipelineConfig`
(`src/common/include/ConfigParser.hpp`). See the [Tutorial](tutorial/Tutorial.md)
for a worked example of Download → Filter → Transform on a toy repo.

```mermaid
flowchart LR
    subgraph inputs[" "]
        CSV[/"repos.csv<br/>(GitHub repo index)"/]
        CFG[/"settings.config"/]
    end

    DL["Download<br/><code>scripts/downloader.py</code>"]
    FI["Filter<br/><code>build/filter</code>"]
    TR["Transform<br/><code>build/transform</code>"]
    VF["Verify<br/><code>build/verify</code>"]

    DB[("databaseDir<br/>cloned repos / raw .c files")]
    FD[("filterDir<br/>filtered .c files")]
    TD[("transformDir<br/>transformed .c files")]
    BM[("benchmarkDir<br/>.c + .yml + .i benchmarks")]

    CSV --> DL
    DL --> DB
    DB --> FI
    FI --> FD
    FD --> TR
    TR --> TD
    TD --> VF
    VF --> BM

    CFG -.-> FI
    CFG -.-> TR
    CFG -.-> VF
```

## Design Choices and Limitations

### Supported Types

Only C builtin primitives have `__VERIFIER_nondet_*` equivalents. The full set (defined in
`VerifierNames.hpp`):

`_Bool`, `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`,
`unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`,
`double`

Types outside this set (structs, unions, enums, typedefs to non-builtins) are
**unsupported for parameter synthesis**. Functions with any unsupported parameter type
cannot be called in the generated harness and have their bodies stripped during filtering.

Pointer parameters are the exception: they are synthesized by allocating a real block
rather than inventing an address, so they are supported whenever `planPointer`
(`src/common/include/HavocPolicy.hpp`) finds a size for the pointee. It does not for
function pointers, pointer-to-pointer, or records containing pointer fields — those stay
unsupported, and a function taking one is body-stripped like any other. The filter gates
on the same classifier the transform uses, so a pointer-param function survives filtering
exactly when the harness can call it.

An integer parameter alongside a pointer parameter is clamped rather than left as a raw
nondet value: `MainGenConsumer::genCallHarness` bounds it with `if (n < 0 || n >
__HAVOC_ARRAY_ELEMS) abort();` (the signedness check dropped for unsigned types). This
sidesteps guessing which parameter is "the length" from its name — every integer is
bounded by the element count of the storage that was actually declared for the pointer
parameters, so a genuinely-undersized block can't be indexed out of bounds regardless of
which parameter the callee treats as a length.

### Pointer Shapes

`planPointer` classifies every pointer or constant-array parameter/return into one
`PointerShape` (`src/common/include/HavocPolicy.hpp`), and `renderPointerStorage` turns a
viable plan into real stack storage for both a parameter or a hoisted return, so
the two call sites can't drift:

- **`CString`** : (`char*`/pointee is any character type) → `char b[__HAVOC_STR_MAX]`,
  filled and null-terminated in bounds by `__havoc_cstring_fill`.
- **`Array`** : a parameter spelled `T[N]`. `ParmVarDecl::getOriginalType()` preserves the
  pre-decay `ConstantArrayType`, so storage uses the declared bound `N` exactly rather than
  a generic size.
- **`Block`** : any other pointer to a sized type, no declared bound available →
  `T b[__HAVOC_ARRAY_ELEMS]`.
- **`Record`** : a struct/union pointee with a definition, viable only when
  `recordHasPointerFields` finds no pointer anywhere in it (transitively). A block filled
  by `__VERIFIER_nondet_memory` would otherwise leave pointer-typed bytes the callee could
  dereference. Not viable means the function isn't harnessed.
- **`Opaque`** : `void*`, an incomplete type, or a record whose definition isn't in the
  main file → `unsigned char b[__HAVOC_BLOCK_MAX]`, cast to the declared pointer type.
- **`Function`** : never viable; no value can be synthesized for a function pointer.

Every shape (besides `Function`) fills its storage with `__VERIFIER_nondet_memory`, so
there's no heap and no `free` obligation, and its declared element count bounds whatever
indexing the callee does into it.

A struct or union tag first named inside a parameter list (`void f(struct Rect *r)`) has
*prototype scope*, invisible outside that declaration, so a harness cast to it would
name a distinct, incompatible type and fail to compile. `pointeeFwdDecl` detects this and
emits a file-scope forward declaration (`struct Rect;`), collected into a set shared by
`HavocCallsConsumer` (havocked pointer returns) and `MainGenConsumer` (harnessed pointer
parameters); `MainGenConsumer` writes the whole set into the file's prelude once every
call is known, since it runs last of the two. A repeated forward declaration is legal C
even when a full definition follows.

### Intraprocedural by Design

Each function body is made self-contained: calls to other functions *in the same file*
are replaced with nondeterministic values (havocked). This means the benchmark exercises
each function in isolation; interprocedural reasoning is not tested. Library calls
(C stdlib, system headers) are kept because they have well-known semantics that verifiers
model directly.

### Body-Stripping vs Deletion

Removed functions are body-stripped to bare prototypes (`void f(int x);`), not deleted
entirely. This is intentional: the Transform step's `HavocCallsVisitor` needs the
callee's declaration to resolve its return type when replacing calls. Without the
prototype, the return type would be unknown and the havoc replacement couldn't determine
which `__VERIFIER_nondet_*` variant to use.

### Main Handling

`main` receives special treatment throughout the pipeline:

- **Filter**: subject to the same complexity/feature thresholds as any other function; a
  `main` that doesn't meet them gets body-stripped too. It's exempt only from the
  parameter-type gate, so its `argc`/`argv` pointer params never trigger removal on their
  own.
- **Transform**: renamed to `original_main` and called from a synthesized `main(void)`.
  For `main(int, char**)`, the pointer params have a well-defined contract (unlike an
  arbitrary `void *`), so we synthesize a realistic `argc`/`argv` using havocked C strings
  rather than skipping the function.
- **Verify**: the *generated* `main` is exempt from the threshold re-check (it's harness
  scaffolding, not benchmark content); `original_main` is subject to thresholds like any
  function, but no parameter gate is re-applied, so its `argc`/`argv` never trigger
  removal.
- The `argc` bound (default `1–4`) and `argv` string size (default `16` bytes) keep the
  verifier's state space bounded. They are emitted into each benchmark as the
  `__HAVOC_ARGC_MIN` / `__HAVOC_ARGC_MAX` / `__HAVOC_STR_MAX` macros rather than inlined
  as literals, so a generated benchmark can be retuned without rerunning the pipeline;
  the values come from `[Havoc Settings]` in the config, falling back to the constants in
  `src/common/include/HavocPolicy.hpp`.

### Pointer Returns in Havocking

When havocking a call that returns a pointer, a raw nondet pointer value cannot be used
(dereferencing an arbitrary address is undefined behavior). Instead, `HavocCallsVisitor`
runs the return type through the same `planPointer`/`renderPointerStorage` pair the
harness uses for parameters (see "Pointer Shapes" above), hoisting a uniquely-named stack
buffer declaration above the call's enclosing statement so it outlives the call, and
replacing the call with a plain-C reference to that storage. No heap, so no `free`
obligation. A non-viable plan (function pointer, pointer-to-pointer, a record with
pointer fields) is left as-is, and a pointer return whose value is discarded is dropped.

### Cleaning Up After Havocking

Havocking hollows statements out: `helper();` becomes a bare nondet call, and a
loop whose body and increment were both calls now spins on nothing — the
transform would have *manufactured* nontermination the source didn't have. Such
residue is erased, but only under a deliberately narrow rule:

> A statement is removed if it is side-effect-free **and** contains a call this
> run havocked.

The second condition is the important one. Without it the pass becomes a
dead-code eliminator, and a benchmark would diff against upstream in places
havocking never touched. Pure-but-untouched code stays exactly as written.

A havocked call counts as side-effect-free because the transform is
intraprocedural: the callee's writes to globals and out-parameters are discarded
already, so the call contributes only its return value. Calls that are *not*
havocked (library calls, aggregate returns, non-viable pointer plans) remain
side-effecting and block removal.

Because a statement can be erased after its inner calls were rewritten, verifier
declarations are decided only once traversal ends — otherwise an erased call
would leave a dangling `extern` that no compiler warning would catch.

### Include Stripping

The Transform step removes all non-system `#include` directives. Functions declared in
project-local headers are havocked anyway, so the includes only leave unresolvable
references. Standard types that were reaching the file transitively through a stripped
project header are recovered by `AddStdIncludesConsumer`. Files that depend on
*project* types or macros from a local header will still fail to compile after
stripping and are caught by the Verify stage's `keepCompilesOnly` compile check.

An unresolved type isn't always AST-visible for `AddStdIncludesConsumer` to recover: a
local variable declared with an unrecognized type name (e.g. `mode_t m;` with no
`sys/types.h` in scope) causes Clang to drop the whole `DeclStmt`, leaving no node to
walk. `UnknownTypeDiagConsumer` closes this gap by hooking the parser's diagnostics
directly (`err_unknown_typename`, and - the common case for a bare local declaration,
which is syntactically ambiguous with an expression-statement - `err_undeclared_var_use`)
and feeding the recovered names into the same `StdHeaders` lookup, name-only and
backstopped by the same compile check.

### Local Header Resolution

Filter and Transform resolve each file's quoted `#include "..."` directives against a
`HeaderIndex` (`src/common/include/IncludeIndex.hpp`) built once per run over
`databaseDir`, passing matches to Clang as `-I` search paths (`runToolOnFile`'s
`extraIncludeDirs`) so project-local headers actually parse instead of just being
stripped later. Basename collisions across the tree are disambiguated only by rebasing
the include's own subdirectory components onto the candidate directory; a spec with no
subdirectory that still matches more than one candidate just takes the closest one.

### Preprocessor-Gated Code

The pipeline only operates on the **active** preprocessed translation unit. Functions inside
inactive `#ifdef` blocks (e.g. `#ifdef REDIS_TEST`) are invisible to all AST passes:
they are neither counted, filtered, havocked, nor harnessed. They pass through unchanged
as raw text.

### Assert Rewriting and Property Selection

Transform installs a second `PPCallbacks` hook alongside `IncludeFinder`: `AssertRewriter`
(`src/transform/include/TransformAction.hpp`, implemented in `TransformAction.cpp`) rewrites
every `assert(cond)` invocation in the main file into `if (!(cond)) reach_error();`. It
fires on `MacroExpands`, checks the expanded macro is function-like and named `assert`, and
re-lexes the invocation's own source text to pull `cond` out from between the first `(` and
the last `)` — no AST node for the call exists yet at that point in the pipeline, so there
is nothing to rewrite from except the raw text. `reach_error` itself is defined
unconditionally in `argv_c_harness.h` as `void reach_error(void) { assert(0); }`; that
`assert` is never touched because it lives outside the main file, so there's no
self-rewriting loop.

SV-Comp's `unreach-call.prp` is checked as `LTL(G ! call(reach_error()))` — a property over
calls to a function with that literal name, not over `assert` or any C-level "reachability"
concept the verifier understands natively. Rewriting `assert` into `reach_error` is what
makes a benchmark's assertions visible to that check at all.

`CountingConsumer`'s `CountingVisitor` (reused unmodified between Filter and Verify) detects
a call to `reach_error` and records a bare `reach_error` key in the counts map via
`try_emplace` — deliberately excluded from every other count, alongside nondet/havoc-helper
calls, so it doesn't skew a function's own metrics. `Verifier::selectProperties`
(`src/verify/Verifier.cpp`) reads the *post-transform* counts — the same map
`VerifyFunctionsConsumer` used for the threshold re-check — and picks `.prp` files by
structural signal, independent of the reach_error check:

- `reach_error` present in counts → `unreach-call.prp`
- any function with `ForLoops` or `WhileLoops` → `termination.prp`
- any function with `Operations` (a side-effecting op on a signed type) → `no-overflow.prp`
- any function with `PointerDeref`, `MemAlloc`, or `MemFree` → `valid-memsafety.prp` (one
  file bundling the deref/free/memtrack CHECKs)

The loop over functions exits early once loops, integer arithmetic, and memory-safety have
each matched once — the reach_error check runs separately up front, so it isn't part of
that short-circuit. `writeBenchmarkTask` then emits the `.yml`: each selected property as an
`expected_verdict: true` block (a generated benchmark is never a deliberately-planted-bug
one, so every selected property is asserted to hold), pointing at `../properties/<name>.prp`
relative to `benchmarkDir`. An empty property list still produces a valid `.yml` (`properties:
[]`) but logs a warning, since a benchmark nothing was selected for is unlikely to be useful
to a verifier run.

### Empty Benchmark Discard

A benchmark whose generated `main` calls no functions is unconditionally deleted. Both
Transform and Verify check for this with the same text-based `harnessIsEmpty`
(`src/common/include/ClangToolUtils.hpp`): it looks for the exact verbatim empty main
`int main(void) {\n  return 0;\n}` in the written output. This works after Verify's
`HarnessRepairConsumer` too, because it erases a whole line per rejected call (indent,
call, semicolon, trailing newline) rather than leaving a bare `;` behind, so a `main`
emptied by repair collapses back to that same verbatim text.

### What Is Not Supported

- **Struct / union / enum parameters passed by value**: no `__VERIFIER_nondet_*`
  equivalent exists for aggregate types, so functions with these parameter types are
  body-stripped and not harnessed. A pointer *to* a struct/union is a separate case — see
  "Pointer Shapes" above — and is supported when the pointee has no pointer fields
- **Variadic functions** (e.g. `printf`-style): skipped with a warning; no way to
  synthesize a meaningful argument list
- **Aggregate return types**: calls returning structs/unions are left as-is (not havocked),
  since there is no expression-position nondet equivalent
- **Function pointer returns**: calls returning function pointers are left as-is
- **`envp` (third `main` parameter)**: `int main(int, char**, char**)` is not explicitly
  handled; only the first two parameters (`argc`, `argv`) are synthesized
- **Macro-expanded calls**: calls inside macro expansions have no rewritable source range
  and are skipped by the havoc pass
- **K&R-style (old-style) declarations**: the pipeline assumes ANSI-prototyped function
  declarations throughout (parameter typing, the filter's parameter-type gate, and
  `HavocCallsVisitor`'s callee resolution all read from `FunctionDecl::parameters()`).
  Old-style `int f(a, b) int a, b; { ... }` definitions are not explicitly detected or
  special-cased, and their behavior through the pipeline is untested
- **Typedef'd types**: a typedef to a supported builtin (e.g. `typedef int myint`) resolves
  correctly via `getAs<BuiltinType>()`, but a typedef to an unsupported type (e.g.
  `typedef struct foo bar`) is treated as unsupported

## Internal Structure: the Clang AST Pipeline Pattern

Filter, Transform, and Verify share the same Clang tooling skeleton. Each tool wires a
sequence of AST consumers into a `MultiplexConsumer`; all consumers share one `Rewriter`
and communicate through shared state passed into each consumer's constructor (Filter's
`toFilter` map and `toRemove` vector; Verify's `counts` map and its own `toRemove`).
The Rewriter's edited buffer is flushed to the output file in `EndSourceFileAction`.
Verify deliberately *reuses* Filter's `CountingConsumer` and `RemoveConsumer` rather than
reimplementing them, so the counting and stripping semantics cannot drift between the two
stages.

```mermaid
flowchart TB
    DRV["Filterer / Transformer / Verifier<br/>(tool driver)"]
    FAC["FrontendActionFactory<br/>(carries config args)"]
    ACT["FilterAction / TransformAction / VerifyAction<br/>(ASTFrontendAction)"]
    MUX["MultiplexConsumer"]

    DRV --> FAC --> ACT --> MUX

    subgraph filterC["Filter consumers (in order)"]
        F1["CountingConsumer<br/>count AST nodes per function → <code>toFilter</code>"]
        F2["FilterFunctionsConsumer<br/>apply thresholds/param gate → <code>toRemove</code>"]
        F3["RemoveConsumer<br/>strip rejected bodies to prototypes"]
        F1 --> F2 --> F3
    end

    subgraph transformC["Transform consumers (in order)"]
        T1["HavocCallsConsumer<br/>havoc in-file calls"]
        T2["MainGenConsumer<br/>rename main, synthesize int main(void),<br/>insert argv_c_harness.h include"]
        T3["AddStdIncludesConsumer<br/>re-inject needed standard headers"]
        T1 --> T2 --> T3
    end

    subgraph verifyC["Verify consumers (in order)"]
        V1["CountingConsumer<br/>(reused from Filter) fresh counts"]
        V2["VerifyFunctionsConsumer<br/>re-apply thresholds, exempt generated"]
        V3["RemoveConsumer<br/>(reused from Filter) strip rejected bodies"]
        V4["HarnessRepairConsumer<br/>erase rejected calls from main"]
        V1 --> V2 --> V3 --> V4
    end

    MUX --> filterC
    MUX --> transformC
    MUX --> verifyC

    RW[("shared Rewriter<br/>+ per-stage toFilter/toRemove/counts state")]
    filterC -.-> RW
    transformC -.-> RW
    verifyC -.-> RW
    RW --> OUT[/"output .c file<br/>(flushed in EndSourceFileAction)"/]
```

Verifier nondet naming, suffix→C-type mappings, and the `isVerifierGenerated`
generated-artifact check live in `src/common/include/VerifierNames.hpp`, shared by all
stages.
