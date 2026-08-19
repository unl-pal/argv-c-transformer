// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file HavocPolicy.hpp
 * @brief Bounds governing how much symbolic state generated havoc code creates.
 *
 * Every havocked object is finite. Their bounds are emitted into generated
 * benchmarks as @c __HAVOC_* macros so a benchmark can be retuned.
 *
 * Nondet values are constrained with @c if (cond) abort().
 */

/**
 * @brief Lower bound on the synthesized @c argc.
 *
 * C11 5.1.2.2.1 permits @c argc==0 (with a NULL @c argv[0]), and Linux
 * @c execve can produce it, but virtually every real @c main dereferences
 * @c argv[0] unconditionally. Starting at 1 models normal execution instead of
 * flagging every benchmark for a null deref; set to 0 to explore that path.
 */
inline constexpr unsigned kArgcMin = 1;

/**
 * @brief Upper bound on the synthesized @c argc.
 *
 * Each admitted argument costs a havocked string, so this multiplies with
 * @ref kStrMax to set the harness's symbolic footprint.
 */
inline constexpr unsigned kArgcMax = 4;

/**
 * @brief Size in bytes of each havocked C string, terminator included.
 *
 * The terminator lands at a nondet offset in [0, kStrMax-1], so reachable
 * string lengths span empty through @c kStrMax-1.
 */
inline constexpr unsigned kStrMax = 16;

/**
 * @brief Size in bytes of a havocked pointer-return block.
 *
 * Used for both the opaque-block and char-pointee (C string) cases; the
 * char-pointee case additionally null-terminates within this bound.
 */
inline constexpr unsigned kBlockMax = 128;
