# SV-COMP benchmark submission summary (`argc-benchmarks`)

This MR adds/updates C verification benchmarks intended for SV-COMP submission.

> **Draft status:** expected verdicts and a few harness/assertion details are still being finalized, so this document should be treated as a working snapshot.

## Property coverage overview (from `*.yml`)

| Property | Benchmarks with property | Expected `true` | Expected `false` |
|---|---:|---:|---:|
| `unreach-call.prp` | 20 | 18 | 2 |
| `valid-memsafety.prp` | 18 | 15 | 3 |
| `no-overflow.prp` | 9 | 7 | 2 |
| `termination.prp` | 20 | 20 | 0 |

## `unreach-call` breakdown (feature-oriented, SV-COMP-style)

| Category | Benchmarks |
|---|---|
| **Arrays / index-heavy loops** | `DrKLO_Telegram_superfasthash`, `scottmwinters_projects_sorts*`, `visit-vis_VisIt_matrix`, `visit-vis_VisIt_dehex`, `visit-vis_VisIt_enhex` |
| **Strings / parsing / text processing** | `antirez_redis_fastjson`, `antirez_redis_strl`, `plexinc_plex-home-theater-public_fstrcmp`, `plexinc_plex-home-theater-public_getdelim`, `visit-vis_VisIt_dehex`, `visit-vis_VisIt_enhex` |
| **Bitwise / arithmetic / numeric kernels** | `antirez_redis_endianconv`, `antirez_redis_mt19937-64`, `antirez_redis_mt19937-64_array`, `DrKLO_Telegram_superfasthash`, `plexinc_plex-home-theater-public_rand_r` |
| **Date/time arithmetic** | `antirez_redis_localtime`, `antirez_redis_localtime_unsafe` |

## Benchmark-by-benchmark summary

| Benchmark(s) (`.yml`) | Input program | Attributes | Submitted properties (expected verdict) | Brief verdict rationale |
|---|---|---|---|---|
| `DrKLO_Telegram_superfasthash.yml` | `DrKLO_Telegram_superfasthash.i` | non-cryptographic hash, byte-level indexing, bit-mixing | `unreach-call: false`, `no-overflow: false`, `valid-memsafety: true`, `termination: true` | Harness asserts `result != 0` for `len > 0`; there are feasible inputs where hash may evaluate to `0`, so error is reachable; arithmetic domain is intentionally aggressive for overflow checking. |
| `antirez_redis_endianconv.yml` | `antirez_redis_endianconv.c` | bitvector/endianness transforms (`16/32/64`), round-trip checks | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Assertions encode involution property (`intrevX(intrevX(v)) == v`), and all memory accesses are local/scalar. |
| `antirez_redis_fastjson.yml` | `antirez_redis_fastjson.i` | JSON token scanning, pointer walking, bounded state-machine parsing | `unreach-call: true`, `no-overflow: true`, `valid-memsafety: true`, `termination: true` | Assertions validate parser invariants; loops are guard-driven and parsing logic is bounded by string end conditions. |
| `antirez_redis_localtime.yml` | `antirez_redis_localtime.c` | date/time conversion arithmetic, calendar invariants | `unreach-call: true`, `no-overflow: true`, `termination: true` | Restricted `t/tz/dst` assumptions keep calendar computations in safe ranges where internal invariants hold. |
| `antirez_redis_localtime_unsafe.yml` | `antirez_redis_localtime_unsafe.c` | same kernel as above, intentionally wider input domain | `unreach-call: false`, `no-overflow: false`, `termination: true` | With widened assumptions, calendar/arithmetic invariants are intentionally stress-tested and expected to admit counterexamples. |
| `antirez_redis_mt19937-64.yml` | `antirez_redis_mt19937-64.i` | PRNG core, bitwise shifts/masks, fixed-size state array | `unreach-call: true`, `valid-memsafety: true`, `no-overflow: true`, `termination: true` | Deterministic bounded loops over fixed state size; no unbounded heap operations in harness path. |
| `antirez_redis_mt19937-64_array.yml` | `antirez_redis_mt19937-64_array.i` | PRNG array-seeding variant, indexed state updates | `unreach-call: true`, `valid-memsafety: true`, `no-overflow: true`, `termination: true` | Similar to above, with array-based seeding path and bounded/state-indexed computation. |
| `antirez_redis_strl.yml` | `antirez_redis_strl.i` | string utility semantics (`strlcpy`/`strlcat`), null-termination checks | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Assertions directly check return-value and buffer/content invariants for string operations. |
| `plexinc_plex-home-theater-public_fstrcmp.yml` | `plexinc_plex-home-theater-public_fstrcmp.i` | fuzzy string comparison, floating-point scoring logic | `unreach-call: true`, `no-overflow: true`, `valid-memsafety: true`, `termination: true` | Compares expected equivalence properties (e.g., identity and symmetric behavior) with bounded string traversal. |
| `plexinc_plex-home-theater-public_getdelim.yml` | `plexinc_plex-home-theater-public_getdelim.i` | dynamic buffer growth, delimiter search, heap ownership | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Harness checks delimiter/null-termination outcomes and frees allocated memory on explored paths. |
| `plexinc_plex-home-theater-public_rand_r.yml` | `plexinc_plex-home-theater-public_rand_r.c` | re-entrant PRNG arithmetic, deterministic replay check | `unreach-call: true`, `no-overflow: true`, `valid-memsafety: true`, `termination: true` | Assertions check output range and seed replay determinism. |
| `scottmwinters_projects_sorts.yml` | `scottmwinters_projects_sorts.c` | sorting algorithms + unsort mutation, array indexing | `unreach-call: true`, `valid-memsafety[valid-deref]: false`, `termination: true` | Main harness eventually executes `unsort`, which uses `p[i]` with `i == SIZE` at loop start (out-of-bounds dereference), driving `valid-deref` to `false`. |
| `scottmwinters_projects_sorts_bub.yml` | `scottmwinters_projects_sorts_bub.c` | sorting benchmark family (bubble-focused name) | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Bubble-sort-only harness over stack arrays; no heap dereference path in `main`. |
| `scottmwinters_projects_sorts_ins.yml` | `scottmwinters_projects_sorts_ins.c` | sorting benchmark family (insertion-focused name) | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Insertion-sort-only harness over stack arrays; no heap dereference path in `main`. |
| `scottmwinters_projects_sorts_mer.yml` | `scottmwinters_projects_sorts_mer.c` | sorting benchmark family (merge-focused name) | `unreach-call: true`, `valid-memsafety[valid-deref]: false`, `termination: true` | `merge` allocates temporaries and dereferences them without a null guard, so `valid-deref` remains `false`. |
| `scottmwinters_projects_sorts_sel.yml` | `scottmwinters_projects_sorts_sel.c` | sorting benchmark family (selection-focused name) | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Selection-sort-only harness over stack arrays; no heap dereference path in `main`. |
| `scottmwinters_projects_sorts_unsort.yml` | `scottmwinters_projects_sorts_unsort.c` | sorting benchmark family (unsort-focused name) | `unreach-call: true`, `valid-memsafety[valid-deref]: false`, `termination: true` | `unsort` indexes `p[SIZE]` on loop entry, creating an out-of-bounds dereference counterexample. |
| `visit-vis_VisIt_dehex.yml` | `visit-vis_VisIt_dehex.c` | hex decode, nibble checks, char classification | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Assertions guard nibble range/conversion correctness with bounded per-character processing. |
| `visit-vis_VisIt_enhex.yml` | `visit-vis_VisIt_enhex.c` | hex encode, byte-to-nibble transformation | `unreach-call: true`, `valid-memsafety: true`, `termination: true` | Assertions and loop structure enforce valid nibble/character mapping and bounded traversal. |
| `visit-vis_VisIt_matrix.yml` | `visit-vis_VisIt_matrix.c` | matrix/vector math, dot products, floating-point invariants | `unreach-call: true`, `valid-memsafety: true`, `no-overflow: true`, `termination: true` | Assertions check algebraic consistency (`dot`/normalization expectations) and index-bounded iteration. |

