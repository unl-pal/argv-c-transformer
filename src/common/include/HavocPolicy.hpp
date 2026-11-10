// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HavocBounds.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <optional>
#include <string>
#include <vector>

/**
 * @file HavocPolicy.hpp
 * @brief The classifier deciding how a given pointer type gets havocked.
 *
 * Nondet values are constrained with @c if (cond) abort(). Bound values live
 * in @c HavocBounds.hpp; this file emits only their macro names.
 *
 * @ref planPointer is the single source of truth for pointer handling, shared
 * by pointer-returning calls and harnessed pointer parameters.
 */

/**
 * @brief How a pointer type should be havocked.
 */
enum class PointerShape {
  CString,  ///< @c char* havocked bytes, null-terminated at the last byte.
  Block,    ///< Pointer to a sized type, no known bound.
  Array,    ///< Constant Array parameter @c T[N], using the declared bound N.
  Record,   ///< Struct/union defined in the main file.
  Opaque,   ///< @c void*, incomplete, or a record defined outside the main file.
  Function, ///< Never viable: no value can be synthesized.
};

/**
 * @brief A non-pointer part of each element of a plan's storage, filled on its
 * own under a fieldwise fill.
 */
struct FieldFill {
  std::vector<const clang::FieldDecl *>
      path; ///< Members from the element; empty for the element itself.
  /// Assigned @c __VERIFIER_nondet_<suffix>(); nullopt to fill with @c __VERIFIER_nondet_memory.
  std::optional<std::string> suffix;
};

/**
 * @brief A decision about one pointer type: its storage shape, how that
 * storage is filled, and the pointers inside it.
 */
struct PointerPlan {
  clang::QualType type; ///< The pointer or constant-array type planned.
  std::vector<const clang::FieldDecl *>
      path; ///< Members from the parent's element to this pointer.
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;
  std::string fwdDecl; ///< For an opaque pointee.
  std::vector<PointerPlan>
      fields; ///< Pointers inside each element, assigned after the nondet fill.
  /// Fills that, with @c fields, cover every element; nullopt to fill in bulk with @c
  /// __VERIFIER_nondet_memory.
  std::optional<std::vector<FieldFill>> fieldwiseFill;
};

/**
 * @brief Classifies a pointer (or array) type into a havoc plan.
 *
 * @param QT Pass @c ParmVarDecl::getOriginalType(), not @c getType(), to avoid array decay.
 * @return The plan; check @c viable before using it.
 */
PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr,
                        const HavocBounds &havoc);

/**
 * @brief A pointer havocked in statement position: setup plus the argument.
 */
struct PointerStorage {
  std::string decls; ///< Prologue statements, indented and newline-terminated.
  std::string arg;
};

/**
 * @brief Declares storage for a viable pointer plan and fills it with nondet
 * values, for use in statement position.
 *
 * @param name     Unique local name for the storage; nested storage appends suffixes to it.
 * @param castType Cast applied only to the opaque byte buffer. Empty to omit.
 * @param indent   Leading whitespace for each emitted statement line.
 * @param heap     Allocate the top-level storage with @c malloc rather than on the stack; nested
 *                 storage stays on the stack.
 * @return The setup and argument; empty when the plan is not viable.
 */
PointerStorage renderPointerStorage(const PointerPlan &plan, const std::string &name,
                                    const std::string &castType, const std::string &indent = "  ",
                                    bool heap = false);
