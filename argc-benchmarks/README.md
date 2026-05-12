<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
SPDX-License-Identifier: Apache-2.0
-->

# SV-COMP ARG-C Benchmarks

Benchmarks generated from real-world C code by the ArgV C Transformer team.
Where full verification is intractable, separate `_tract` source and task files apply restricted bounds or concrete input values.
Links to original source files and descriptions are provided below.
Unless specified, properties are assumed to have `true` as the expected verdict.

---

## superfasthash (DrKLO/Telegram)

[Source](https://github.com/DrKLO/Telegram/blob/009e97356f966bb81eceba113d210230bf383122/TMessagesProj/jni/voip/webrtc/base/third_party/superfasthash/superfasthash.c)

Hash algorithm operating on byte arrays with bitwise operations and a Fisher-Yates-style shuffle. A bound controls the number of input bytes.

**Properties:** unreach-call (expected false), no-overflow (expected false), valid-memsafety, termination

---

## endianconv (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/endianconv.c)

Byte-order reversal functions for 16/32/64-bit integers using bitwise ops and byte swaps. No arrays. Assertions verify the double-reversal identity.

**Properties:** unreach-call, valid-memsafety, termination

- UAuto reports a false alarm on unreach-call; the double-reversal identity holds by construction.
- CPA errors on valid-memsafety: does not support partial reads of symbolic values.

---

## fastjson (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/modules/vector-sets/fastjson.c)

Lightweight top-level JSON field extractor using mutually recursive token parsers (`jsonParseValueToken` ↔ `jsonParseArrayToken`). Bounds restrict parsing depth and field count. The tractable variant (`_tract`) pins input to a single field with a symbolic string value.

**Properties:** unreach-call, no-overflow, valid-memsafety, termination

- UAuto requires a named union; anonymous unions are not supported.
- Neither tool resolves unreach-call or termination due to mutual recursion: CPAchecker's termination analysis does not support mutual recursion; UAuto cannot prove the recursive structure regardless of bounds. This is a structural limitation, not a bounds issue.

---

## localtime (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/localtime.c)

`localtime()` implementation using timestamp arithmetic. Assertions verify that computed `tm_hour`, `min`, `sec`, `wday`, `yday`, `mday` are within valid calendar ranges. Bounds restrict the timezone and a 5-year window to cover leap years. The `_unsafe` variant widens input to expose the Y2038 bug and underflow from timezone adjustment. Tractable variants pin or greatly reduce the time domain; the `_unsafe_tract` version is the primary source of expected-false counterexamples.

**Properties:** unreach-call (unsafe: expected false), no-overflow (unsafe: expected false), termination

- CPA does not support proof splitting, causing exceptions in some tract termination variants.

---

## mt19937-64 (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/mt19937-64.c)

Mersenne Twister 64-bit PRNG in two variants: scalar-seeded (`mt19937-64`) and array-seeded (`mt19937-64_array`). Heavy use of bitwise ops, 64-bit arithmetic, and a state array of length NN (312). Assertions on PRNG output only make sense with a concrete seed value, so the unreach-call property is handled via separate `_unreach_*` files, each providing a concretely-seeded harness for a different integer type variant (`ll`, `ull`, `r1`–`r3`). The array variant has a tractable valid-memsafety variant that reduces the seeding loop bound.

**Properties:** unreach-call (via concrete-seed variants), no-overflow, valid-memsafety, termination

---

## strl (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/strl.c)

`strlcpy` and `strlcat`: null-safe bounded string copy and concatenation. Assertions verify return-value semantics and null-termination. The tractable variant uses smaller fixed-size buffers.

**Properties:** unreach-call, valid-memsafety, termination

- CPAchecker errors due to a const type mismatch in pointer arithmetic.
- UAuto models `strlen` imprecisely, affecting unreach-call results.

---

## fstrcmp (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/utils/fstrcmp.c)

Fuzzy string similarity via a recursive longest-common-substring algorithm. Assertions check symmetry (`fstrcmp(a,b) == fstrcmp(b,a)`) and identity (`fstrcmp(a,a) == 1.0`). The tractable variant uses concrete strings.

**Properties:** unreach-call, no-overflow, valid-memsafety, termination

- Termination is a structural limitation: CPAchecker does not support recursive termination analysis; UAuto times out regardless of bounds.

---

## getdelim (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/android/bionic_supplement/getdelim.c)

Dynamic line-reading function using `realloc`-based buffer growth. Assertions verify delimiter detection and null-termination; the harness frees allocated memory on all explored paths.

**Properties:** unreach-call, valid-memsafety, termination

---

## rand_r (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/android/bionic_supplement/rand_r.c)

Simple reentrant PRNG using xorshift-style bit arithmetic on a single `unsigned int` seed. Assertions check output range and seed-replay determinism. No arrays.

**Properties:** unreach-call, no-overflow, valid-memsafety, termination

---

## sorts (scottmwinters/projects)

[Source](https://github.com/scottmwinters/projects/blob/fb38a62c98984b4bac4fcefbc875ca55b3e92f5c/sorts.c)

Classic sorting algorithms split into individual benchmark files: bubble (`_bub`), insertion (`_ins`), selection (`_sel`), merge (`_mer`), and an unsorted baseline (`_unsort`). Tractable variants for insertion and selection reduce the array size. The merge sort has a tractable termination variant. `_unsort` is the primary expected-false source for both unreach-call and valid-memsafety.

**Properties:** unreach-call (unsort: expected false), valid-memsafety (mer, unsort: expected false), termination

- CPAchecker does not support recursive termination analysis, affecting sorts_mer.
- sorts_mer valid-memsafety is expected false: `merge` dereferences a `malloc` result without a null guard.

---

## dehex (visit-vis/VisIt)

[Source](https://github.com/visit-vis/VisIt/blob/141aa46a88ed33742e3ae951c1e2825f9465cce4/third_party_builtin/slivr/teem-1.9.0-src/src/hex/dehex.c)

Hex decoder operating on nibbles with character classification checks. Assertions guard nibble range and conversion correctness. A tractable variant restricts input length.

**Properties:** unreach-call (expected false), valid-memsafety (expected false), termination

- UAuto's termination analysis produces an error on this benchmark (log investigation needed).

---

## enhex (visit-vis/VisIt)

[Source](https://github.com/visit-vis/VisIt/blob/141aa46a88ed33742e3ae951c1e2825f9465cce4/third_party_builtin/slivr/teem-1.9.0-src/src/hex/enhex.c)

Hex encoder transforming bytes to nibble pairs. A tractable variant restricts input length.

**Properties:** unreach-call, valid-memsafety, termination

- UAuto's termination analysis errors consistently (same pattern as dehex).

---

## matrix (visit-vis/VisIt)

[Source](https://github.com/visit-vis/VisIt/blob/141aa46a88ed33742e3ae951c1e2825f9465cce4/plots/Molecule/matrix.c)

4×4 LU decomposition, matrix inversion (`_inv`), and affine point transform (`_mult`). Uses single-precision floats. The unreach-call property applies only to the multiply variant via a separate `_mult_unreach` file. Tractable variants restrict the input domain for both inversion and multiplication.

**Properties:** unreach-call (mult only, via `_unreach` variant), no-overflow, valid-memsafety, termination

- UAuto does not support `fabs`, causing consistent ERROR 7 on termination and valid-memsafety.
- Both tools time out on unreach-call even in the tractable variant; float-heavy LU decomposition is expensive regardless of bounds.

---

## SV-COMP Category Mapping

Suggested category `.set` file membership when adding to the sv-benchmarks repository (`c/` level).
Each `.yml` file (including `_tract` and `_unreach_*` variants) is listed under the category matching its actual properties.

### ReachSafety (unreach-call)

| Category | `.yml` files |
|---|---|
| `ReachSafety-Arrays` | `DrKLO_Telegram_superfasthash`, `antirez_redis_strl`, `antirez_redis_strl_tract`, `scottmwinters_projects_sorts_bub`, `scottmwinters_projects_sorts_ins`, `scottmwinters_projects_sorts_ins_tract`, `scottmwinters_projects_sorts_sel`, `scottmwinters_projects_sorts_sel_tract`, `scottmwinters_projects_sorts_unsort`, `visit-vis_VisIt_dehex_tract`, `visit-vis_VisIt_enhex_tract` |
| `ReachSafety-BitVectors` | `antirez_redis_endianconv`, `antirez_redis_mt19937-64_unreach_ll`, `antirez_redis_mt19937-64_unreach_ull`, `antirez_redis_mt19937-64_unreach_r1`, `antirez_redis_mt19937-64_unreach_r2`, `antirez_redis_mt19937-64_unreach_r3`, `plexinc_plex-home-theater-public_rand_r` |
| `ReachSafety-Floats` | `visit-vis_VisIt_matrix_mult_unreach` |
| `ReachSafety-Heap` | `plexinc_plex-home-theater-public_getdelim` |
| `ReachSafety-Recursive` | `antirez_redis_fastjson_tract`, `plexinc_plex-home-theater-public_fstrcmp`, `plexinc_plex-home-theater-public_fstrcmp_tract`, `scottmwinters_projects_sorts_mer`, `scottmwinters_projects_sorts_mer_tract` |
| `ReachSafety-Integers` | `antirez_redis_localtime`, `antirez_redis_localtime_tract`, `antirez_redis_localtime_unsafe`, `antirez_redis_localtime_unsafe_tract` |

Note: `antirez_redis_fastjson` (base) has no unreach-call property; only the `_tract` variant does. `visit-vis_VisIt_dehex` and `visit-vis_VisIt_enhex` (base) likewise carry only valid-memsafety and termination; their `_tract` variants carry unreach-call.

### NoOverflows (no-overflow)

| Category | `.yml` files |
|---|---|
| `NoOverflows-BitVectors` | `DrKLO_Telegram_superfasthash`, `antirez_redis_mt19937-64`, `antirez_redis_mt19937-64_array`, `plexinc_plex-home-theater-public_rand_r` |
| `NoOverflows-Other` | `antirez_redis_fastjson`, `antirez_redis_fastjson_tract`, `antirez_redis_localtime`, `antirez_redis_localtime_tract`, `antirez_redis_localtime_unsafe`, `antirez_redis_localtime_unsafe_tract`, `plexinc_plex-home-theater-public_fstrcmp`, `plexinc_plex-home-theater-public_fstrcmp_tract`, `visit-vis_VisIt_matrix_inv`, `visit-vis_VisIt_matrix_mult` |

### MemSafety (valid-memsafety)

| Category | `.yml` files |
|---|---|
| `MemSafety-Arrays` | `DrKLO_Telegram_superfasthash`, `antirez_redis_mt19937-64`, `antirez_redis_mt19937-64_array`, `antirez_redis_mt19937-64_array_valid-memsafety_tract`, `antirez_redis_strl`, `scottmwinters_projects_sorts_bub`, `scottmwinters_projects_sorts_ins`, `scottmwinters_projects_sorts_mer`, `scottmwinters_projects_sorts_sel`, `scottmwinters_projects_sorts_unsort`, `visit-vis_VisIt_dehex`, `visit-vis_VisIt_enhex`, `visit-vis_VisIt_matrix_inv`, `visit-vis_VisIt_matrix_mult` |
| `MemSafety-Heap` | `plexinc_plex-home-theater-public_getdelim`, `plexinc_plex-home-theater-public_fstrcmp` |
| `MemSafety-Other` | `antirez_redis_endianconv`, `antirez_redis_fastjson`, `plexinc_plex-home-theater-public_rand_r` |

### Termination

| Category | `.yml` files |
|---|---|
| `Termination-MainHeap` | `plexinc_plex-home-theater-public_getdelim` |
| `Termination-MainLoop` | all other `.yml` files that carry the termination property |
