<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
SPDX-License-Identifier: Apache-2.0
-->

# Verifier Notes

Observations from CPAchecker 4.2.2 and ULTIMATE Automizer 0.3.1-35a84365. Only benchmarks with notable wrong verdicts or tool errors are listed. "False alarm" means the tool reports a violation where none exists; "false negative" means the tool misses a real violation.

---

## superfasthash (DrKLO/Telegram)

- `no-overflow` — **CPA false negative** (expected false): CPA reports `true`, missing the hash computation overflow. UAuto correctly finds `false(no-overflow)`.

---

## endianconv (antirez/redis)

- `unreach-call` — **UAuto false alarm** (expected true): spurious counterexample; the double-reversal identity holds by construction.
- `valid-memsafety` — **CPA error**: does not support partial reads of symbolic values.

---

## fastjson (antirez/redis)

- `_tract` `unreach-call` — **CPA error**, UAuto `unknown`: CPA throws an internal `Non-monotonic SSAMap update` assertion triggered by the mutual recursion. This leaves `unreach-call` with zero coverage across all fastjson tasks — the base file has no unreach-call property and the only task that carries it (`_tract`) cannot be resolved by either tool.
- `_tract` `termination` — **CPA error** (`ERROR (recursion)`), UAuto timeout: CPA does not support recursive termination analysis. The base `fastjson.yml` termination is covered by UAuto (694s); the tract has no coverage.
- `no-overflow` — both tools time out or OOM on both variants regardless of bounds; the mutual-recursion control flow dominates analysis cost.

---

## localtime (antirez/redis)

- **Note**: `_unsafe*` files declare `data_model: ILP32` but the directory Makefile declares `CC.Arch := 64`. The SV-COMP check tool flags this; resolution is to move ILP32 files into a subdirectory with `CC.Arch := 32`. The ILP32 model is intentional — the Y2038 bug is a 32-bit `long` issue.
- `_unsafe`/`_unsafe_tract` `unreach-call` — **UAuto false negative** (expected false): UAuto does not correctly model ILP32 `long` width, so the signed overflow that triggers `reach_error()` is missed. CPA handles it correctly.
- `_unsafe_term` `termination` — no coverage: CPA times out, UAuto OOM. The wide nondet input range makes termination analysis expensive; accepted as intractable.

---

## mt19937-64 (antirez/redis)

- `_unreach_*` `unreach-call` — both tools time out on all variants despite concrete execution paths. The 312-element PRNG state is inherently intractable; no tractable variant is possible without replacing the algorithm.

---

## strl (antirez/redis)

- `unreach-call`/`termination` (base and tract) — **CPA error**: "Can't subtract pointers of different types" on `dst - odst` (`const char *` minus `char *`). UAuto yields `unknown`/timeout. No coverage for either property on either variant.

---

## fstrcmp (plexinc/plex-home-theater-public)

- `_fstrcmp_tract` `unreach-call` — **CPA false alarm** (expected true): spurious counterexample. UAuto gets `unknown`.
- `termination` (base and tract) — **CPA error** (`ERROR (recursion)`): CPA does not support recursive termination analysis. UAuto times out on the base and errors (ERROR 7) on the tract. No termination coverage on either variant.

---

## sorts (natsteven)

- `_mer`/`_mer_tract` `termination` — **CPA error** (`ERROR (recursion)`): CPA does not support recursive termination analysis. UAuto also errors (ERROR 7) on the base `_mer`. Covered by UAuto on `_mer_tract_term`.
- `_mer_tract` `unreach-call` — **CPA error** (`ERROR (interpolation failed)`): the VLA-based recursive structure causes the interpolation algorithm to fail even with fully concrete harness inputs. UAuto returns `true` (434s).
- `_unsort` `valid-memsafety` — **CPA error** (expected true): UAuto correctly returns `true`.
- `_bub` `termination` — no coverage: CPA returns `unknown`, UAuto times out. No tract currently covers this property.
- `_unsort_term` `termination` — no coverage: both tools time out. The fixed-count loop terminates trivially but the symbolic swap indices make state-space exploration expensive.
- `_bub_tract` vs `_bub_swap_tract` `unreach-call` (both SIZE=4) — standard `swap` (189s CPA, 877s UAuto) is faster than `xor_swap` (448s CPA, UAuto timeout). Contrary to expectation, `xor_swap` is harder for both tools; the XOR chain produces more complex SMT formulae than the straightforward temp-variable swap.

---

## dehex (visit-vis/VisIt)

- `valid-memsafety` — **CPA false alarm** (expected true): CPA cannot precisely model `strcmp("-", inS)` with a string literal, treating the comparison as symbolic and exploring the unexercised `fopen` branch, where it reports a spurious `valid-deref` violation. UAuto errors (ERROR 7).

---

## enhex (visit-vis/VisIt)

- `valid-memsafety` — **CPA false alarm** (expected true): same `strcmp("-", inS)` precision loss as dehex. UAuto times out.
- `_tract` `unreach-call` — **CPA false alarm** (expected true): CPA ignores `vsnprintf` side effects on stack buffers, leaving the output buffer zeroed. Fixed by replacing `mock_fprintf` with a fixed-signature function `(FILE*, const char*, int, int)` that reads the two nibble arguments directly, eliminating all variadic machinery. Not yet reflected in current results.
- `_tract` `termination` — **CPA error** (`Unsupported feature: __builtin_va_arg`): the old variadic `mock_fprintf` used `va_arg`, which CPA cannot model. Resolved by the same fixed-signature replacement above; expected to be fixed in next run.

---

## matrix (visit-vis/VisIt)

- `_inv` (all properties) — **UAuto error** (ERROR 7): `fabs` is unsupported in non-bitprecise translation.
- `_inv` `no-overflow` — **CPA error**: MATHSAT5 eager fp solver does not support proof generation for floating-point.
- `_inv` `termination` — **CPA crash** (SIGSEGV): cascading `java.lang.NoSuchMethodError` on `java.lang.invoke.*` methods causes the JVM to crash. Root cause is a Java version incompatibility in the termination analysis module.
- `_inv` `valid-memsafety` — **CPA error**: symbolic or non-zero offsets in pointer-to-array function arguments are unsupported.
