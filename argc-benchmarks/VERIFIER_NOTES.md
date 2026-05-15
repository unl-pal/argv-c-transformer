<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
SPDX-License-Identifier: Apache-2.0
-->

# Verifier Notes

Observations from CPAchecker 4.2.2 and ULTIMATE Automizer 0.3.1-35a84365. Only benchmarks with notable wrong verdicts or tool errors are listed. "False alarm" means the tool reports a violation where none exists; "false negative" means the tool misses a real violation.

---

## superfasthash (DrKLO/Telegram)

- **CPA false negative** on `no-overflow` (expected false): CPA reports `true`, missing the hash computation overflow. UAuto correctly finds `false(no-overflow)`.

---

## endianconv (antirez/redis)

- **UAuto false alarm** on `unreach-call` (expected true): spurious counterexample; the double-reversal identity holds by construction.
- **CPA error** on `valid-memsafety`: does not support partial reads of symbolic values.

---

## fastjson (antirez/redis)

- **CPA error** on `_tract` `unreach-call`: internal `Non-monotonic SSAMap update` assertion triggered by the mutual recursion.
- Both tools time out or OOM on `no-overflow` regardless of bounds; the mutual-recursion control flow dominates analysis cost.

---

## localtime (antirez/redis)

- **Note**: `_unsafe*` files declare `data_model: ILP32` but the directory Makefile declares `CC.Arch := 64`. The SV-COMP check tool flags this; resolution is to move ILP32 files into a subdirectory with `CC.Arch := 32`. The ILP32 model is intentional — the Y2038 bug is a 32-bit `long` issue.
- **UAuto false negative** on `unreach-call` for `_unsafe` and `_unsafe_tract` (expected false): UAuto does not correctly model ILP32 `long` width, so the signed overflow that triggers `reach_error()` is missed. CPA handles it correctly.

---

## mt19937-64 (antirez/redis)

- All `_unreach_ll` variant times out on both tools despite a single concrete execution path. The 312-element PRNG state is inherently intractable; no tractable variant is possible without replacing the algorithm.

---

## strl (antirez/redis)

- **CPA error** on `termination` and `unreach-call`: "Can't subtract pointers of different types" on `dst - odst` (`const char *` minus `char *`). UAuto yields `unknown` on both.

---

## fstrcmp (plexinc/plex-home-theater-public)

- **CPA false alarm** on `_fstrcmp_tract` `unreach-call` (expected true): spurious counterexample. UAuto gets `unknown`.

---

## sorts (natsteven)

- **CPA error** on `termination` for `_mer` and `_mer_tract`: `ERROR (recursion)` — CPA does not support recursive termination analysis. UAuto also errors (ERROR 7) on the base `_mer` file. Covered by UAuto on `_mer_tract_term`.
- **CPA error** on `_mer_tract` `unreach-call` with symbolic inputs: `ERROR (interpolation failed)` caused by complex abstract invariants across recursive call sites with symbolic input. Fixed by pinning both harness inputs to concrete values in `_mer_tract`.
- **CPA error** on `_unsort` `valid-memsafety` (expected true). UAuto correctly returns `true`.
- `_bub_tract` (xor swap) and `_bub_swap_tract` (standard swap) are both SIZE=4 variants intended to compare CPA tractability across swap implementations.

---

## dehex (visit-vis/VisIt)

- **CPA false alarm** on `valid-memsafety` (expected true): CPA cannot precisely model `strcmp("-", inS)` with a string literal, treating the comparison result as symbolic and exploring the unexercised `fopen` branch, where it reports a spurious `valid-deref` violation. UAuto errors (ERROR 7).

---

## enhex (visit-vis/VisIt)

- **CPA false alarm** on `valid-memsafety` (expected true): same `strcmp("-", inS)` precision loss as dehex. UAuto times out.
- **CPA false alarm** on `_tract` `unreach-call` (expected true): CPA ignores `vsnprintf` side effects on stack buffers, leaving the output buffer zeroed. Fixed in `_tract` by replacing `mock_fprintf` with a fixed-signature function that reads arguments directly and writes to the output buffer without calling any library formatting functions. UAuto gets `unknown`.

---

## matrix (visit-vis/VisIt)

- **UAuto error** (ERROR 7) on all `_inv` properties: `fabs` is unsupported in non-bitprecise translation.
- **CPA error** on `_inv` `no-overflow`: MATHSAT5 eager fp solver does not support proof generation for floating-point.
- **CPA crash** on `_inv` `termination`: JVM `NoSuchMethodError` — a Java version incompatibility in the termination analysis module.
- **CPA error** on `_inv` `valid-memsafety`: symbolic or non-zero offsets in pointer-to-array function arguments are unsupported.
