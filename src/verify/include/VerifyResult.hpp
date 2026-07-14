// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @brief Per-file outcome of the verify pass, filled in by the consumers and
 * read back by the Verifier driver after the tool run.
 *
 * The driver uses {@code harnessCalls} to detect a benchmark whose harness
 * emptied out after repair (nothing left worth verifying) and the counters
 * for the end-of-run summary.
 */
struct VerifyResult {
  /// Calls left in the generated main after repair (excluding generated
  /// helpers). 0 means the benchmark no longer exercises anything.
  int harnessCalls = 0;
  /// Harness calls removed because their target failed the metric re-check.
  int removedCalls = 0;
};
