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
  CString,  ///< @c char* havocked bytes with a nondet-positioned terminator.
  Block,    ///< Pointer to a sized type, no known bound.
  Array,    ///< Constant Array parameter @c T[N], using the declared bound N.
  Record,   ///< Struct/union with a definition.
  Opaque,   ///< @c void* or incomplete.
  Function, ///< Never viable: no value can be synthesized.
};

/**
 * @brief A decision about one pointer type: what shape, and any declaration its
 * spelling needs.
 */
struct PointerPlan {
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;
  std::string fwdDecl; ///< For an opaque pointee.
  unsigned elems = 0;  ///< Constant array size.
};

/**
 * @brief True if a record has a pointer somewhere in its fields, transitively.
 */
inline bool recordHasPointerFields(const clang::RecordDecl *record, unsigned depth = 0) {
  const clang::RecordDecl *def = record ? record->getDefinition() : nullptr;
  if (!def || depth > 8) // reasonable recursion limit
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
 */
inline std::string pointeeFwdDecl(clang::QualType pointee, const clang::SourceManager &mgr) {
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
    return "";
  }
  if (tag->getName().empty())
    return "";
  return kind + " " + tag->getName().str();
}

/**
 * @brief Classifies a pointer (or array) type into a havoc plan.
 *
 * @param QT Pass @c ParmVarDecl::getOriginalType(), not @c getType(), to avoid array decay.
 * @return The plan; check @c viable before using it.
 */
inline PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr) {
  PointerPlan plan;
  if (QT.isNull() || QT.getTypePtrOrNull() == nullptr)
    return plan;

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

  if (pointee->isAnyPointerType()) // no recursive field initialization yet
    return plan;

  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    plan.viable = true;
    return plan;
  }

  // unsized if the definition came from a header the transform strips
  bool sized = !pointee->isVoidType() && !pointee->isIncompleteType();
  if (sized) {
    if (const clang::TagDecl *tag = pointee->getAsTagDecl()) {
      const clang::TagDecl *def = tag->getDefinition();
      sized = def && mgr.isInMainFile(def->getLocation());
    }
  }
  if (!sized) {
    plan.shape = PointerShape::Opaque;
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
    plan.viable = true;
    return plan;
  }

  if (pointee->isRecordType()) {
    plan.shape = PointerShape::Record;
    if (recordHasPointerFields(pointee->getAsRecordDecl())) // not viable: no recursive field init yet
      return plan;
  }

  plan.viable = true;
  return plan;
}

/**
 * @brief A pointer havocked in statement position: setup plus the argument.
 */
struct PointerStorage {
  std::string decls;    ///< Prologue statements, indented and newline-terminated.
  std::string arg;
  bool cstring = false;
};

/**
 * @brief Declares stack storage for a viable pointer plan and fills it with
 * @c __VERIFIER_nondet_memory, for use in statement position.
 *
 * @param declared The parameter's pre-decay type, or a call's return type.
 * @param name     Unique local name for the storage variable.
 * @param castType Cast applied only to the opaque byte buffer. Empty to omit.
 * @param indent   Leading whitespace for each emitted statement line.
 * @return The setup and argument; empty when the plan is not viable.
 */
inline PointerStorage renderPointerStorage(const PointerPlan &plan, clang::QualType declared,
                                           const std::string &name, const std::string &castType,
                                           const std::string &indent = "  ") {
  PointerStorage out;
  if (!plan.viable)
    return out;

  // alignment potentially an issue
  if (plan.shape == PointerShape::Opaque) {
    out.decls = indent + "unsigned char " + name + "[__HAVOC_BLOCK_MAX];\n" + indent +
               "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
    out.arg = castType.empty() ? name : "(" + castType + ")" + name;
    return out;
  }

  // Unqualified, so a const pointee still yields writable storage.
  clang::QualType pointee;
  if (const auto *arrayType =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(declared->getAsArrayTypeUnsafe()))
    pointee = arrayType->getElementType();
  else if (declared->isAnyPointerType())
    pointee = declared->getPointeeType();
  else
    return out;

  std::string count;
  if (plan.shape == PointerShape::Array)
    count = std::to_string(plan.elems);
  else if (plan.shape == PointerShape::CString)
    count = "__HAVOC_STR_MAX";
  else
    count = "__HAVOC_ARRAY_ELEMS";

  std::string decl = name + "[" + count + "]";
  // getAsStringInternal handles correct syntax for n-dimensions by appending
  // additional dimensions' size, e.g. int[count] -> int[count][size]
  pointee.getUnqualifiedType().getAsStringInternal(decl, clang::LangOptions());
  out.decls = indent + decl + ";\n";

  if (plan.shape == PointerShape::CString) {
    out.arg = "__havoc_cstring_fill(" + name + ", " + count + ")";
    out.cstring = true;
    return out;
  }

  out.decls += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  out.arg = name;
  return out;
}
