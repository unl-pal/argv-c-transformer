#!/bin/sh

# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

# End-to-end smoke test for the filter/transform/verify pipeline, run against
# a small fixed corpus (tests/smoke/samples) exercising known-quirky patterns.
#
# Unlike the ctest golden tests (which check AST-consumer behavior in
# isolation), this drives the actual built binaries end-to-end, including
# the `clang -E -P` / `clang -fsyntax-only` shell-outs in Verifier.cpp
# that resolve `clang` via PATH rather than the linked LLVM libraries. That
# path is the one most likely to drift between platforms (e.g. macOS
# resolving Apple's system clang instead of Homebrew's llvm@20), so this is
# meant to run on every CI platform, not just Linux.
#
# Usage: run_smoke_test.sh <filter_bin> <transform_bin> <verify_bin>

set -eu

FILTER_BIN="${1:?usage: run_smoke_test.sh <filter_bin> <transform_bin> <verify_bin>}"
TRANSFORM_BIN="${2:?usage: run_smoke_test.sh <filter_bin> <transform_bin> <verify_bin>}"
VERIFY_BIN="${3:?usage: run_smoke_test.sh <filter_bin> <transform_bin> <verify_bin>}"
SAMPLES_DIR="$(cd "$(dirname "$0")/samples" && pwd)"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

fail=0

echo "=================================== Toolchain ==================================="
echo "filter:    $FILTER_BIN"
echo "transform: $TRANSFORM_BIN"
echo "verify:    $VERIFY_BIN"
echo "clang (PATH-resolved, used by Verifier.cpp's shell-outs):"
command -v clang && clang --version || {
  echo "FAIL: no 'clang' resolvable on PATH"
  fail=1
}

echo
echo "=================================== Run: default config (Concurrency=ignore) ==================================="
cat > "$WORK/default.config" <<EOF
[File Locations]
databaseDir=$SAMPLES_DIR
filterDir=$WORK/filtered-default
transformDir=$WORK/transformed-default
benchmarkDir=$WORK/bench-default
EOF

# Config-file-only invocation ("classic" style, see CLAUDE.md): passing
# SAMPLES_DIR as a positional input path would take precedence over
# filterDir/transformDir/benchmarkDir above (see Filterer.cpp's/
# Transformer.cpp's/Verifier.cpp's documented input-path-wins precedence),
# silently redirecting output to a derived <name>-filtered/ etc. in the
# current directory instead of $WORK.
"$FILTER_BIN" "$WORK/default.config"
"$TRANSFORM_BIN" "$WORK/default.config"
"$VERIFY_BIN" "$WORK/default.config"

echo
echo "--- Assertion: baseline file produces a benchmark ---"
if [ ! -f "$WORK/bench-default/clean_ok.c" ]; then
  echo "FAIL: clean_ok.c did not produce a benchmark; pipeline itself is broken on this platform"
  fail=1
else
  echo "OK"
fi

echo
echo "--- Assertion: benchmark has task file and preprocessed input ---"
if [ ! -f "$WORK/bench-default/clean_ok.yml" ] || [ ! -f "$WORK/bench-default/clean_ok.i" ]; then
  echo "FAIL: clean_ok benchmark is missing its .yml or .i (verify finalization broken)"
  fail=1
else
  echo "OK"
fi

echo
echo "--- Assertion: no quoted #include survives in any produced benchmark ---"
if ls "$WORK/bench-default"/*.c >/dev/null 2>&1 && grep -lE '^[[:space:]]*#include[[:space:]]*"' "$WORK/bench-default"/*.c; then
  echo "FAIL: quoted include(s) leaked into benchmark output (files listed above)"
  fail=1
else
  echo "OK"
fi

echo
echo "=================================== Run: Concurrency=forbid ==================================="
cat > "$WORK/forbid.config" <<EOF
[File Locations]
databaseDir=$SAMPLES_DIR
filterDir=$WORK/filtered-forbid
transformDir=$WORK/transformed-forbid
benchmarkDir=$WORK/bench-forbid

[Feature Requirements]
Concurrency = forbid
EOF

"$FILTER_BIN" "$WORK/forbid.config"
"$TRANSFORM_BIN" "$WORK/forbid.config"
"$VERIFY_BIN" "$WORK/forbid.config"

echo
echo "--- Assertion: no live pthread/semaphore call survives Concurrency=forbid ---"
if ls "$WORK/bench-forbid"/*.i >/dev/null 2>&1 && \
   grep -lE 'pthread_create\(|pthread_mutex_lock\(|pthread_mutex_unlock\(|pthread_join\(|sem_wait\(|sem_post\(' "$WORK/bench-forbid"/*.i; then
  echo "FAIL: concurrency call(s) survived under Concurrency=forbid (files listed above)"
  fail=1
else
  echo "OK"
fi

echo
if [ "$fail" -ne 0 ]; then
  echo "=================================== SMOKE TEST FAILED ==================================="
else
  echo "=================================== SMOKE TEST PASSED ==================================="
fi
exit $fail
