// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <string>

/**
 * @file HavocPolicy.hpp
 * @brief The classifier deciding how a given pointer type gets havocked.
 *
 * Nondet values are constrained with @c if (cond) abort(). The @c __HAVOC_*
 * bound values themselves live in @c HavocBounds.hpp; this file only emits
 * their macro *names*, so the generated source stays retunable by editing the
 * benchmark.
 *
 * @ref planPointer is the single source of truth for pointer handling, shared
 * by the two sites that need it: pointer-returning calls and harnessed pointer
 * parameters.
 */

/**
 * @brief How a pointer type should be havocked.
 */
enum class PointerShape {
  CString,  ///< @c char* havocked bytes with a nondet-positioned terminator.
  Block,    ///< Pointer to a sized type, no declared bound: HavocBounds::arrayElems of them.
  Array,    ///< Constant Array parameter @c T[N], using the declared bound N.
  Record,   ///< Pointer to a struct/union with a definition.
  Opaque,   ///< @c void*, incomplete.
  Function, ///< Function pointer. Never viable: no value can be synthesized.
};

/**
 * @brief A decision about one pointer type: what shape, and any declaration its
 * spelling needs. The storage itself is rendered by @ref renderPointerStorage,
 * which re-derives the pointee from the source type — the plan carries only the
 * classification, so the same decision serves the filter's viability gate (which
 * has no call site to render against) and the transform's two havoc sites.
 */
struct PointerPlan {
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;      ///< False means: do not havoc this pointer at all.
  std::string fwdDecl;      ///< Necessary forward declaration for opaque pointee, or "" if none.
  unsigned elems = 0;       ///< Element count for ConstantArrayType, 0 when the size is a raw byte count.
};

/**
 * @brief True if a record has a pointer somewhere in its fields, transitively.
 *
 * Such a record cannot be havocked by bulk @c __VERIFIER_nondet_memory alone.
 * Until recursive initialization exists, these records are not viable.
 */
inline bool recordHasPointerFields(const clang::RecordDecl *record, unsigned depth = 0) {
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
 * @brief The file-scope declaration a cast to @p pointee needs, or "" if none.
 *
 * Only the *name* is needed, never the size: this path is reached only for
 * types the output cannot size anyway, which planPointer classifies as Opaque
 * and measures in raw bytes.
 */
inline std::string pointeeFwdDecl(clang::QualType pointee, const clang::SourceManager &mgr) {
  // Any tag type - struct, union, or enum - needs its tag hoisted; a builtin
  // (void, an integer) or a bare pointer names nothing the output must declare.
  const clang::TagDecl *tag = pointee->getAsTagDecl();
  if (!tag)
    return "";
  std::string kind(tag->getKindName());
  if (const auto *typedefType = pointee->getAs<clang::TypedefType>()) {
    const clang::TypedefNameDecl *decl = typedefType->getDecl();
    if (decl && !mgr.isInMainFile(decl->getLocation())) {
      std::string name = decl->getName().str();
      std::string synth = tag->getName().empty() ? "__havoc_" + name : tag->getName().str();
      return "typedef " + kind + " " + synth + " " + name;
    }
    // Declared in the main file: it comes with its own definition.
    return "";
  }
  if (tag->getName().empty())
    return ""; // anonymous and unnamed: nothing to declare, nothing names it
  return kind + " " + tag->getName().str();
}

/**
 * @brief Classifies a pointer (or array) type into a havoc plan.
 *
 * @param QT  The type to classify. Pass @c ParmVarDecl::getOriginalType() rather
 *            than @c getType() to circumvent type decay (i.e. arrays).
 * @param mgr SourceManager for the translation unit being transformed.
 * @return The plan; check @c viable before using it.
 */
inline PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr) {
  PointerPlan plan;
  if (QT.isNull() || QT.getTypePtrOrNull() == nullptr)
    return plan;

  // A constant array T[N] keeps its bound in the original type; anything
  // else is a pointer whose size we have to assume.
  clang::QualType pointee;
  if (const auto *arrayType =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(QT->getAsArrayTypeUnsafe())) {
    pointee = arrayType->getElementType();
    plan.shape = PointerShape::Array;
    plan.elems = static_cast<unsigned>(arrayType->getSize().getZExtValue());
  } else if (QT->isAnyPointerType()) {
    pointee = QT->getPointeeType();
    plan.shape = PointerShape::Block;
  } else {
    return plan; // not a pointer at all
  }

  if (QT->isFunctionPointerType() || pointee->isFunctionType()) {
    plan.shape = PointerShape::Function;
    return plan;
  }

  // No recursive field initialization yet
  if (pointee->isAnyPointerType())
    return plan;

  // char* is a string, not a byte block: the terminator has to land somewhere
  // in bounds or every string operation in the callee runs off the end.
  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    plan.viable = true;
    return plan;
  }

  // void*, an incomplete type, or a tag (struct/union/enum) whose definition
  // came from a header the transform strips: no sizeof is available or will
  // survive.
  bool sized = !pointee->isVoidType() && !pointee->isIncompleteType();
  if (sized) {
    if (const clang::TagDecl *tag = pointee->getAsTagDecl()) {
      const clang::TagDecl *def = tag->getDefinition();
      sized = def && mgr.isInMainFile(def->getLocation());
    }
  }
  if (!sized) {
    plan.shape = PointerShape::Opaque;
    // Opaque is the only shape whose storage is cast to the pointee type, so it
    // is the only one whose spelling needs a declaration the output would not
    // otherwise carry. A sized pointee is declared as real storage of its own
    // type, which brings the tag with it - no forward declaration required.
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
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

  plan.viable = true;
  return plan;
}

/**
 * @brief A pointer havocked in statement position: setup plus the argument.
 */
struct PointerStorage {
  std::string decls;    ///< Prologue statements (indented, newline-terminated).
  std::string arg;      ///< Expression to pass as the argument.
  bool cstring = false; ///< True if a nondet size_t + abort terminator was emitted.
};

/**
 * @brief Havocs a pointer by declaring stack storage and filling it, in
 * statement position.
 *
 * The one renderer for every viable pointer, param or return: it declares a real
 * array of the pointee type and fills it with @c __VERIFIER_nondet_memory.
 *
 * @param plan     A viable plan from @ref planPointer.
 * @param declared The source type the plan was built from (a parameter's
 *                 pre-decay type, or a call's return type); the pointee is
 *                 re-peeled from it here so the storage element type matches.
 * @param name     Unique local name for the storage variable.
 * @param castType Pointer type to cast to, used only for the opaque byte buffer,
 *                 whose element type is not the pointee. Empty to omit.
 * @param policy   Printing policy for spelling the storage declaration.
 * @param indent   Leading whitespace for each emitted statement line.
 * @return The setup and argument; empty when the plan is not viable.
 */
inline PointerStorage renderPointerStorage(const PointerPlan &plan, clang::QualType declared,
                                           const std::string &name, const std::string &castType,
                                           const clang::PrintingPolicy &policy,
                                           const std::string &indent = "  ") {
  PointerStorage out;
  if (!plan.viable)
    return out;

  // Opaque has no pointee type to declare: a raw byte buffer stands in, cast to
  // the parameter's pointer type at the call. _Alignas(16) covers the real
  // pointee's unknown alignment need, since its actual type is exactly what's
  // unavailable here.
  if (plan.shape == PointerShape::Opaque) {
    out.decls = indent + "_Alignas(16) unsigned char " + name + "[__HAVOC_BLOCK_MAX];\n" + indent +
               "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
    out.arg = castType.empty() ? name : "(" + castType + ")" + name;
    return out;
  }

  // Re-peel the pointee exactly as planPointer did, so the storage element type
  // matches. Unqualified, so a const pointee still yields writable storage.
  clang::QualType pointee;
  if (const auto *arrayType =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(declared->getAsArrayTypeUnsafe()))
    pointee = arrayType->getElementType();
  else if (declared->isAnyPointerType())
    pointee = declared->getPointeeType();
  else
    return out;

  // Element count: the declared bound for an array parameter, the string-max
  // for a char pointer, the assumed count for a bare pointer.
  std::string count;
  if (plan.shape == PointerShape::Array)
    count = std::to_string(plan.elems);
  else if (plan.shape == PointerShape::CString)
    count = "__HAVOC_STR_MAX";
  else
    count = "__HAVOC_ARRAY_ELEMS";

  // getAsStringInternal places the identifier *inside* the declarator, so an
  // array-of-array pointee (int[4]) spells correctly as "int name[K][4]" rather
  // than the invalid "int[4] name[K]" bare concatenation would produce.
  std::string decl = name + "[" + count + "]";
  pointee.getUnqualifiedType().getAsStringInternal(decl, policy);
  out.decls = indent + decl + ";\n";

  // A char block is a string, not raw bytes: the fill and the nondet-positioned,
  // in-bounds terminator are both handled by argv_c_harness.h's helper, which
  // hands back the same buffer so it can stand in for the call directly.
  if (plan.shape == PointerShape::CString) {
    out.arg = "__havoc_cstring_fill(" + name + ", " + count + ")";
    out.cstring = true;
    return out;
  }

  out.decls += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  out.arg = name;
  return out;
}
