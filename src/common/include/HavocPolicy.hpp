// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <string>

/**
 * @file HavocPolicy.hpp
 * @brief Bounds governing how much symbolic state generated havoc code creates,
 * and the classifier deciding how a given pointer type gets havocked.
 *
 * Every havocked object is finite. There bounds are emitted into generated
 * benchmarks as @c __HAVOC_* macros so a benchmark can be retuned.
 *
 * Nondet values are constrained with @c if (cond) abort().
 *
 * @ref planPointer is the single source of truth for pointer handling, shared
 * by the two sites that need it: pointer-returning calls (expression position,
 * heap-backed) and harnessed pointer parameters (statement position, so
 * stack-backed setup is possible). It is pure AST-to-decision, with no
 * Rewriter and no emitted text.
 */

/**
 * @brief Lower bound on the synthesized @c argc.
 *
 * note that technically argc can be 0 but we assume not here
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
 * @brief Element count assumed for a pointer with no declared bound.
 *
 * A bare @c T* is ambiguous. This doubles as the clamp applied to int parameters
 * of a function that also takes a pointer: bounding the indices to fit the block.
 */
inline constexpr unsigned kArrayElems = 8;

/**
 * @brief Byte size used when the pointee has no computable size.
 *
 * Applies to @c void* and to any type whose definition will not exist in the
 * transformed output (see @ref planPointer). Nothing better is available: the
 * real extent is unknown by construction.
 */
inline constexpr unsigned kOpaqueBytes = 128;

/**
 * @brief The bounds as actually applied to one run, after config overrides.
 *
 * Only the consumer that emits the @c __HAVOC_* macro definitions needs these
 * numbers. Everything else — @ref planPointer, the harness synthesis — emits
 * the macro *names*, so the generated source stays retunable by editing the
 * benchmark, and no other code has to be threaded with configuration.
 */
struct HavocBounds {
  unsigned argcMin = kArgcMin;
  unsigned argcMax = kArgcMax;
  unsigned strMax = kStrMax;
  unsigned arrayElems = kArrayElems;
  unsigned opaqueBytes = kOpaqueBytes;
};

/**
 * @brief How a pointer type should be havocked.
 */
enum class PointerShape {
  CString,  ///< @c char* — havocked bytes with a nondet-positioned terminator.
  Block,    ///< Pointer to a sized type, no declared bound: @ref kArrayElems of them.
  Array,    ///< Parameter spelled @c T[N] — the declared bound N is used exactly.
  Record,   ///< Pointer to a struct/union with a definition.
  Opaque,   ///< @c void*, incomplete.
  Function, ///< Function pointer. Never viable: no value can be synthesized.
};

/**
 * @brief A decision about one pointer type: what shape, how big, spelled how.
 */
struct PointerPlan {
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;      ///< False means: do not havoc this pointer at all.
  std::string pointeeType;  ///< Unqualified pointee spelling; empty when Opaque.
  std::string sizeExpr;     ///< C expression for the byte size, in __HAVOC_* terms.
  std::string helper;       ///< "__havoc_cstring" or "__havoc_block".
  /// Tag needing a file-scope forward declaration, e.g. "struct Rect"; else empty.
  /// A struct named only inside a parameter list has *prototype scope*, so a
  /// cast to it elsewhere would name a distinct, incompatible type. Emitting
  /// "struct Rect;" in the preamble hoists the tag to file scope. Legal even
  /// when a definition follows, so call sites need not check whether it is
  /// already declared.
  std::string fwdDecl;
  unsigned elems = 0;       ///< Element count; 0 when the size is a raw byte count.
};

/**
 * @brief True if a record has a pointer somewhere in its fields, transitively.
 *
 * Such a record cannot be havocked by bulk @c __VERIFIER_nondet_memory alone:
 * the spec forbids dereferencing a pointer value that nondet_memory produced,
 * so the pointer fields would have to be assigned real blocks afterwards.
 * Until that recursive initialization exists, these records are not viable.
 */
inline bool recordHasPointerFields(const clang::RecordDecl *record,
                                   unsigned depth = 0) {
  const clang::RecordDecl *def = record ? record->getDefinition() : nullptr;
  if (!def || depth > 8) // depth cap guards against cycles via embedded records
    return true;
  for (const clang::FieldDecl *field : def->fields()) {
    clang::QualType type = field->getType();
    if (const auto *arrayType =
            llvm::dyn_cast_or_null<clang::ArrayType>(type->getAsArrayTypeUnsafe()))
      type = arrayType->getElementType();
    if (type->isAnyPointerType())
      return true;
    if (const clang::RecordDecl *nested = type->getAsRecordDecl())
      if (recordHasPointerFields(nested, depth + 1))
        return true;
  }
  return false;
}

/**
 * @brief Classifies a pointer (or array) type into a havoc plan.
 *
 * @param QT  The type to classify. Pass @c ParmVarDecl::getOriginalType() rather
 *            than @c getType(): a parameter spelled @c T[N] has already decayed
 *            to @c T* in the latter, discarding the bound this uses.
 * @param mgr SourceManager for the translation unit being transformed.
 * @return The plan; check @c viable before using it.
 *
 * @note Completeness is deliberately *not* the test for record types. The
 * transform strips project-local includes as a textual edit after preprocessing
 * has already run, so the AST still holds full definitions for types that will
 * not exist in the output file. Emitting @c sizeof(struct S) for such a type
 * yields source that does not compile. The test that matters is whether the
 * definition lives in the main file and therefore survives into the output.
 */
inline PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr) {
  PointerPlan plan;
  if (QT.isNull() || QT.getTypePtrOrNull() == nullptr)
    return plan;

  // A parameter spelled T[N] keeps its bound in the original type; anything
  // else is a pointer whose element count we have to assume.
  clang::QualType pointee;
  if (const auto *arrayType =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(QT->getAsArrayTypeUnsafe())) {
    pointee = arrayType->getElementType();
    plan.shape = PointerShape::Array;
    plan.elems = static_cast<unsigned>(arrayType->getSize().getZExtValue());
  } else if (QT->isAnyPointerType()) {
    pointee = QT->getPointeeType();
    plan.shape = PointerShape::Block;
    plan.elems = kArrayElems;
  } else {
    return plan; // not a pointer at all
  }

  if (QT->isFunctionPointerType() || pointee->isFunctionType()) {
    plan.shape = PointerShape::Function;
    return plan;
  }

  plan.pointeeType = pointee.getUnqualifiedType().getAsString();

  // Pointer-to-pointer has the same problem as a record with pointer fields:
  // bulk nondet_memory would fill the block with pointer values the callee may
  // not legally dereference. Filling each slot with its own block is the same
  // machinery as recursive field initialization; until that exists, decline.
  // (main's argv is exempt: MainGenConsumer synthesizes it explicitly.)
  if (pointee->isAnyPointerType())
    return plan;

  // char* is a string, not a byte block: the terminator has to land somewhere
  // in bounds or every string operation in the callee runs off the end.
  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    plan.helper = "__havoc_cstring";
    plan.sizeExpr = "__HAVOC_STR_MAX";
    plan.elems = 0;
    plan.viable = true;
    return plan;
  }

  plan.helper = "__havoc_block";

  // Recorded before the Opaque branch below clears pointeeType: an opaque
  // record is exactly the case that needs the forward declaration most, since
  // nothing else in the output file will ever name the tag.
  if (const clang::RecordDecl *record = pointee->getAsRecordDecl())
    if (!record->getName().empty())
      plan.fwdDecl = std::string(record->getKindName()) + " " + record->getName().str();

  // void*, an incomplete type, or a record whose definition came from a header
  // the transform strips: no sizeof is available or will survive.
  bool sized = !pointee->isVoidType() && !pointee->isIncompleteType();
  if (sized) {
    if (const clang::RecordDecl *record = pointee->getAsRecordDecl()) {
      const clang::RecordDecl *def = record->getDefinition();
      sized = def && mgr.isInMainFile(def->getLocation());
    }
  }
  if (!sized) {
    plan.shape = PointerShape::Opaque;
    plan.pointeeType.clear();
    plan.sizeExpr = "__HAVOC_OPAQUE_BYTES";
    plan.elems = 0;
    plan.viable = true;
    return plan;
  }

  if (pointee->isRecordType()) {
    plan.shape = PointerShape::Record;
    // Bulk-havocking a record with pointer fields would hand the callee
    // pointers it may not legally dereference. Recursive field initialization
    // is what makes these viable; until then, decline.
    if (recordHasPointerFields(pointee->getAsRecordDecl()))
      return plan;
  }

  plan.sizeExpr = "sizeof(" + plan.pointeeType + ")";
  if (plan.shape == PointerShape::Array)
    plan.sizeExpr += " * " + std::to_string(plan.elems);
  else
    plan.sizeExpr += " * __HAVOC_ARRAY_ELEMS";
  plan.viable = true;
  return plan;
}

/**
 * @brief Renders a plan as a single C expression yielding the pointer.
 *
 * This is the form both call sites can use, and the only form available to
 * pointer-returning calls, which sit in expression position with nowhere to put
 * setup statements.
 *
 * @param plan     A viable plan from @ref planPointer.
 * @param castType Full pointer type to cast to (the helpers return
 *                 @c void* or @c char*, so e.g. an @c unsigned char* result would
 *                 otherwise be an incompatible assignment). Empty to omit.
 * @return The expression, or "" if the plan is not viable.
 */
inline std::string renderPointerExpr(const PointerPlan &plan, const std::string &castType) {
  if (!plan.viable)
    return "";
  std::string cast = castType.empty() ? "" : "(" + castType + ")";
  return cast + plan.helper + "(" + plan.sizeExpr + ")";
}
