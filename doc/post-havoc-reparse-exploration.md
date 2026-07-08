# Post-havoc reparse filter: exploration (not adopted)

## The problem this explored

After `HavocCallsConsumer` havocs every in-file call, a function can end up
much less "interesting" than it looked before transforming — e.g. a while
loop whose only content was a call that got dropped collapses to an empty
loop. `HavocCallsConsumer` already strips and excludes functions that
collapse *entirely* to a no-op body (this predates this exploration and
needed no changes). What it doesn't catch: a function that still has some
body left after havocking, but has fallen below the complexity thresholds
you'd otherwise filter on pre-transform (e.g. lost its only while-loop but
kept an if-statement). Those still get harnessed into the benchmark today.

## The approach prototyped here

`clang::Rewriter` edits are text-only and never mutate the AST already
parsed from the original source, so a second pass over the *same* AST would
still see the pre-edit node counts — a naive recheck wouldn't see the
loop that got erased. Two ways around that were considered:

1. Thread the no-op `Stmt` set through a second counting visitor within the
   same AST pass, so it skips already-erased nodes. Implemented first,
   but judged too much added machinery/state-tracking for what it bought.
2. **Reparse**: write the havocked buffer to a real file/stream and have a
   completely separate `ClangTool` pass reparse it from scratch, so the new
   AST always matches the current text. This is what's on this branch.

Concretely, the transform step was split into three sequential `ClangTool`
invocations per file (see `src/transform/Transformer.cpp`):

1. **`HavocAction`** — `IncludeFinder` + `HavocCallsConsumer` only, writing
   the havocked buffer to a `.stage1.c` intermediate file.
2. **Filter recheck** — the existing, unmodified `FilterAction` (count →
   threshold-check → strip bodies) reused verbatim against stage 1's output,
   using the same `complexityConfig`/`featureConfig` maps `Filterer` uses
   (via the shared `ConfigParser.hpp`, renamed from `ComplexityConfigParser`
   for this branch). Writes to `.stage2.c`.
3. **`FinishAction`** — `IncludeFinder` + `MainGenConsumer` +
   `AddVerifiersConsumer` + `AddStdIncludesConsumer`, deliberately excluding
   `HavocCallsConsumer` (rerunning it here would re-havoc the already-
   inserted, not-yet-declared `__VERIFIER_nondet_*` calls, which — being
   undeclared at that point — would pick up an implicit `int` return type
   and risk silently corrupting e.g. a `bool` havoc into an `int` havoc).

A key simplification found along the way: since both `HavocCallsConsumer`
and the filter's `RemoveVisitor` physically replace a rejected/no-op body
with `;`, a fresh reparse in stage 3 naturally makes `MainGenConsumer`'s
existing `isThisDeclarationADefinition()` check skip those functions with
zero new code — no explicit no-op name-set needs to survive the reparse.

Verified manually: a function whose while-loop collapses to a no-op only
after havocking is correctly stripped by the stage-2 threshold recheck and
excluded from the stage-3 harness, while a genuine-logic function in the
same file is retained. Full test suite (43/43) passed with this in place.

## Why it's probably not worth adopting (as of this exploration)

- **The benefit is narrow and the value is unclear.** The clearest, highest-
  value case (fully no-op functions) was already handled with zero reparse
  machinery. What's left is catching functions that are *diminished but not
  empty* post-havoc — how often that actually happens, and how much it
  matters, depends heavily on how much havocking currently degrades real
  code, which is expected to change (havoc's behavior isn't settled yet, and
  may come to support more constructs / make smaller changes to code shape).
  Building this out now is optimizing for a problem whose size we don't
  actually know yet.
- **The cost is immediate and real.** Three full `ClangTool` parses per file
  instead of one, two new `ASTFrontendAction` classes plus factories,
  intermediate temp files, and cross-stage state (verifier suffixes) that
  has to be threaded from stage 1 into stage 3 — meaningfully more moving
  parts for a marginal gain.
- **Cheaper backstops already exist.** `harnessIsEmpty` discards a benchmark
  outright if nothing at all could be harnessed, and trivial-but-not-empty
  benchmarks can simply be discarded downstream by whoever's curating the
  benchmark set. That may be good enough without enforcing the metrics
  post-havoc at all.

Revisit this if, in practice, non-trivial-but-still-junk benchmarks are
observed slipping through despite the existing no-op stripping and
`harnessIsEmpty` check — that's a much stronger basis for this than
anticipating it ahead of time.

## Where the code lives

This branch (`explore/natsteven/post-havoc-reparse-filter`) has the full
working implementation, in case the reparse approach is revisited later:

- `src/transform/HavocAction.{hpp,cpp}`, `HavocActionFactory.{hpp,cpp}`
- `src/transform/FinishAction.{hpp,cpp}`, `FinishActionFactory.{hpp,cpp}`
- `src/transform/Transformer.{hpp,cpp}` — 3-stage orchestration in
  `transformFile`, plus `complexityConfig`/`featureConfig` members
- `src/filter/include/ConfigParser.hpp` — renamed from
  `ComplexityConfigParser.hpp`, shared by `Filterer` and `Transformer`

Known follow-ups if this is picked back up (raised in review but not yet
done): retire `TransformAction`/`ArgsFrontendActionFactory` (unused in
production once this lands, and the golden tests that exercise it no longer
reflect the real pipeline shape); collapse the near-identical
`HavocActionFactory`/`FinishActionFactory`/`ArgsFrontendActionFactory` into
one generic factory; and stop threading `neededSuffixes` across stages by
having `AddVerifiersConsumer` rediscover stage-1's verifier calls itself via
an AST scan in stage 3, since by then they're real reparsed AST nodes, not
just pending Rewriter edits.
