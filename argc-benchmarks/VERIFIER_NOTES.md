<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
SPDX-License-Identifier: Apache-2.0
-->

# Verifier Notes

Observations from CPAchecker 4.2.2 and ULTIMATE Automizer 0.3.1-35a84365 logs. Covers wrong verdicts and named tool errors only; timeouts and unknowns are not listed unless a specific support issue was identified in the logs.

---

## superfasthash (DrKLO/Telegram)

- `no-overflow` — **CPA false negative** (expected false): CPA reports `true`.

---

## endianconv (antirez/redis)

- `unreach-call` — **UAuto false alarm** (expected true): spurious counterexample.
- `valid-memsafety` — **CPA error**: partial reads of symbolic values appear unsupported.

---

## fastjson (antirez/redis)

- `_tract` `unreach-call` — **CPA exception**: internal `Non-monotonic SSAMap update` assertion.
- `_tract` `termination` — **CPA error** (`ERROR (recursion)`): recursive termination analysis appears unsupported.

---

## localtime (antirez/redis)

- **Note**: `_unsafe*` files declare `data_model: ILP32`; the directory Makefile declares `CC.Arch := 64`. The SV-COMP check tool flags this as a mismatch.
- `_unsafe`/`_unsafe_tract` `unreach-call` — **UAuto false negative** (expected false): UAuto appears not to model ILP32 `long` correctly. CPA returns the correct verdict.

---

## strl (antirez/redis)

- `unreach-call`/`termination` (base and tract) — **CPA error**: "Can't subtract pointers of different types" on `dst - odst`.

---

## fstrcmp (plexinc/plex-home-theater-public)

- `_fstrcmp_tract` `unreach-call` — **CPA false alarm** (expected true): spurious counterexample.
- `termination` (base and tract) — **CPA error** (`ERROR (recursion)`): recursive termination analysis appears unsupported.

---

## sorts (natsteven)

- `_mer`/`_mer_tract` `termination` — **CPA error** (`ERROR (recursion)`): recursive termination analysis appears unsupported. UAuto also errors (ERROR 7) on the base `_mer`.
- `_mer_tract` `unreach-call` — **CPA error** (`ERROR (interpolation failed)`).
- `_unsort` `valid-memsafety` — **CPA error** (expected true).

---

## dehex (visit-vis/VisIt)

- `valid-memsafety` — **CPA false alarm** (expected true): CPA approximates `strcmp("-", inS)` as symbolic, then bails on a symbolic-offset read derived from it (`Stop analysis because of an error in symbolic offset in read operation`). Memsafe by construction: `argv[1]` is pinned to `"-"`, so only the `fin = stdin` path runs; all buffers are fixed-size static arrays and mock I/O is bound-checked.

---

## enhex (visit-vis/VisIt)

- `valid-memsafety` — **CPA false alarm** (expected true): same pattern as dehex.

---

## matrix (visit-vis/VisIt)

- `_inv` (all properties) — **UAuto error** (ERROR 7): `fabs` appears unsupported.
- `_inv` `no-overflow` — **CPA error**: fp solver appears not to support proof generation.
- `_inv` `termination` — **CPA crash** (SIGSEGV): cascading `java.lang.NoSuchMethodError` on `java.lang.invoke.*` methods.
- `_inv` `valid-memsafety` — **CPA error**: pointer-to-array argument handling appears unsupported.
