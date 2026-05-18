<!--
SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
SPDX-License-Identifier: Apache-2.0
-->

# SV-COMP ARG-V-C Benchmarks

These benchmarks are part of the ARG-V project: [Website](https://arg-v.dev/).

Benchmarks generated from real-world C OSS code found on github. Code is modified as little as possible to preserve original logic and structure. Additions include a verifier harness with main function, assertions for the unreach-call property, as well as certain mocked constructs when necessary. Where verification is intractable, separate `_tract` source and task files apply restricted bounds or concrete input values.

Preprocessing is performed with `gcc -E -P -std=gnu11 -m64 (src).c -o (src).i`.

[Github](https://github.com/unl-pal/argv-c-transformer)
Contact: [Nathanael Steven](nathanaelsteven@u.boisestate.edu) (main developer)
[Dr. Elena Sherman](elenasherman@boisestate.edu)
[Dr. Robert Dyer](rdyer@unl.edu)

---

## superfasthash (DrKLO/Telegram)

[Source](https://github.com/DrKLO/Telegram/blob/009e97356f966bb81eceba113d210230bf383122/TMessagesProj/jni/voip/webrtc/base/third_party/superfasthash/superfasthash.c)

Hash algorithm operating on byte arrays with bitwise operations. A bound controls the number of input bytes.

## endianconv (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/endianconv.c)

Byte-order reversal for 16/32/64-bit integers using bitwise ops and byte swaps. Assertions verify the double-reversal identity.

## fastjson (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/modules/vector-sets/fastjson.c)

Lightweight JSON field extractor using **mutually recursive** token parsers (`jsonParseValueToken` ↔ `jsonParseArrayToken`). The tractable variant pins input to a single field with a symbolic string value.

## localtime (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/localtime.c)

Lock-free `localtime()` implementation. Assertions verify computed `tm_hour`, `tm_min`, `tm_sec`, `tm_wday` are within valid calendar ranges. The `_unsafe` variant widens input to expose the Y2038 signed overflow bug.

## mt19937-64 (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/mt19937-64.c)

Mersenne Twister 64-bit PRNG. There is a scalar-seeded variant (`mt19937-64`) and array-seeded variant (`mt19937-64_array`). The unreach-call property requires concrete seeds, handled via separate `_unreach_*` task files for each output type (`ll`, `ull`, `r1`–`r3`).

## strl (antirez/redis)

[Source](https://github.com/antirez/redis/blob/e8726d18e5bab24cbfcb0a0c36f21ce5a1140471/src/strl.c)

`strlcpy` and `strlcat`: bounded string copy and concatenation. Assertions verify return-value semantics and null-termination. The tractable variant uses smaller fixed-size buffers.

## fstrcmp (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/utils/fstrcmp.c)

Fuzzy string similarity via a **recursive** longest-common-substring algorithm. Assertions check symmetry and identity. The tractable variant uses concrete strings.

## getdelim (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/android/bionic_supplement/getdelim.c)

Dynamic line-reading function using `realloc`-based buffer growth. The tractable variant caps `mock_getc` calls to keep all writes within the initial buffer, eliminating the realloc path.

## rand_r (plexinc/plex-home-theater-public)

[Source](https://github.com/plexinc/plex-home-theater-public/blob/f2f1d63f2d48ad0d2935f7002ee1168ebcd5bb3d/xbmc/android/bionic_supplement/rand_r.c)

Reentrant PRNG using xorshift-style bit arithmetic on a single `unsigned int` seed. Assertions check output range and seed-replay determinism.

## sorts (natsteven)

Classic sorting algorithms split into individual benchmark files: bubble (`_bub`), insertion (`_ins`), selection (`_sel`), merge (`_mer`), and an unsort (`_unsort`). Bubble sort uses an xor swap by default. The merge sort is **recursive** (divide-and-conquer) and uses stack-allocated VLAs. Tractable variants reduce array size or pin inputs for unreach-call and termination coverage.

## (en|de)hex (visit-vis/VisIt)

[Source](https://github.com/visit-vis/VisIt/blob/141aa46a88ed33742e3ae951c1e2825f9465cce4/third_party_builtin/slivr/teem-1.9.0-src/src/hex/dehex.c)

Hex (en|de)coder. Assertions guard nibble range and conversion correctness. The base file has symbolic input with bounded length, and the tractable variant uses a concrete input.

## matrix (visit-vis/VisIt)

[Source](https://github.com/visit-vis/VisIt/blob/141aa46a88ed33742e3ae951c1e2825f9465cce4/plots/Molecule/matrix.c)

4×4 LU decomposition, matrix inversion (`_inv`), and affine point transform (`_mult`). Uses single-precision floats.
