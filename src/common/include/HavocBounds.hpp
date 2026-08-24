// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @brief Bounds on synthesized symbolic state, emitted as {@code __HAVOC_*}
 * macros ahead of each transformed file's {@code #include "argv_c_harness.h"}.
 */
struct HavocBounds {
  /** Lower bound on the synthesized main's argc. */
  int argcMin = 1;
  /** Upper bound on the synthesized main's argc. */
  int argcMax = 4;
  /** Buffer size (terminator included) for a havocked C string. */
  int strMax = 16;
  /** Byte size for a havocked opaque-pointer block. */
  int blockMax = 128;
};
