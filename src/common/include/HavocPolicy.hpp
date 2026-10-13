// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VerifierNames.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <cstdint>
#include <llvm/Support/Casting.h>
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
  CString,  ///< @c char* havocked bytes with a nondet-positioned terminator.
  Block,    ///< Pointer to a sized type, no known bound.
  Array,    ///< Constant Array parameter @c T[N], using the declared bound N.
  Record,   ///< Struct/union with a definition.
  Opaque,   ///< @c void* or incomplete.
  Function, ///< Never viable: no value can be synthesized.
};

/**
 * @brief A scalar inside each element of a plan's storage, assigned a
 * @c __VERIFIER_nondet_<suffix>() value.
 */
struct ScalarField {
  std::vector<std::string> path; ///< Member accesses from the element, "[]" for each array level.
  std::vector<uint64_t> dims;    ///< Bound of each "[]" in @c path.
  std::string suffix;
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
  std::vector<ScalarField> scalars;
  bool fieldwise = false; ///< @c scalars cover every field, so no bulk fill is needed.
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
    if (type->isAnyPointerType()) return true;
    if (const clang::RecordDecl *nested = type->getAsRecordDecl())
      if (recordHasPointerFields(nested, depth + 1)) return true;
  }
  return false;
}

/**
 * @brief The nondet suffix a storage element or record field of @p type is
 * assigned from, or nullopt if it has none and the storage must be bulk-filled instead.
 */
inline std::optional<std::string> scalarFieldSuffix(clang::QualType type, bool readOnly) {
  if (readOnly) return std::nullopt; // only nondet_memory can write a const member
  if (const auto *enumType = type->getAs<clang::EnumType>())
    return verifierSuffixForType(enumType->getDecl()->getIntegerType());
  return verifierSuffixForType(type);
}

/**
 * @brief Appends a scalar field for every nondet-assignable scalar reachable by
 * value inside @p record, clearing @c plan.fieldwise if some field is not one.
 *
 * @param readOnly The record is reached through a const member.
 * @param enclosing Records being walked above this one.
 */
inline void collectScalars(const clang::RecordDecl *record, const std::vector<std::string> &path,
                           const std::vector<uint64_t> &dims, bool readOnly, PointerPlan &plan,
                           std::vector<const clang::RecordDecl *> enclosing = {}) {
  const clang::RecordDecl *def = record->getDefinition();
  if (!def || def->isUnion()) { // assigning one member would clobber its siblings
    plan.fieldwise = false;
    return;
  }
  // Error recovery can leave a record containing itself by value.
  if (std::find(enclosing.begin(), enclosing.end(), def) != enclosing.end()) {
    plan.fieldwise = false;
    return;
  }
  enclosing.push_back(def);
  for (const clang::FieldDecl *field : def->fields()) {
    if (field->isInvalidDecl() || field->isUnnamedBitField()) continue; // padding, not addressable
    std::vector<std::string> fieldPath = path;
    std::vector<uint64_t> fieldDims = dims;
    if (!field->isAnonymousStructOrUnion()) fieldPath.push_back("." + field->getName().str());
    clang::QualType type = field->getType();
    bool flexible = false;
    while (const clang::ArrayType *array = type->getAsArrayTypeUnsafe()) {
      const auto *constant = llvm::dyn_cast<clang::ConstantArrayType>(array);
      if (!constant) {
        flexible = true;
        break;
      }
      fieldDims.push_back(constant->getSize().getZExtValue());
      fieldPath.push_back("[]");
      type = constant->getElementType();
    }
    if (flexible) continue; // outside sizeof, so outside the storage
    bool fieldReadOnly = readOnly || type.isConstQualified();
    if (const clang::RecordDecl *nested = type->getAsRecordDecl())
      collectScalars(nested, fieldPath, fieldDims, fieldReadOnly, plan, enclosing);
    else if (std::optional<std::string> suffix = scalarFieldSuffix(type, fieldReadOnly))
      plan.scalars.push_back({fieldPath, fieldDims, *suffix});
    else
      plan.fieldwise = false;
  }
}

/**
 * @brief The file-scope declaration a cast to @p pointee needs, or "" if none.
 */
inline std::string pointeeFwdDecl(clang::QualType pointee, const clang::SourceManager &mgr) {
  const clang::TagDecl *tag = pointee->getAsTagDecl();
  if (!tag) return "";
  std::string kind(tag->getKindName());
  if (const auto *typedefType = pointee->getAs<clang::TypedefType>()) {
    const clang::TypedefNameDecl *decl = typedefType->getDecl();
    if (decl && !mgr.isInMainFile(decl->getLocation()) &&
        !mgr.isInSystemHeader(mgr.getFileLoc(decl->getLocation()))) {
      std::string name = decl->getName().str();
      std::string synth = tag->getName().empty() ? "__havoc_" + name : tag->getName().str();
      return "typedef " + kind + " " + synth + " " + name;
    }
    return "";
  }
  if (tag->getName().empty()) return "";
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
  if (QT.isNull() || QT.getTypePtrOrNull() == nullptr) return plan;

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
    if (recordHasPointerFields(
            pointee->getAsRecordDecl())) // not viable: no recursive field init yet
      return plan;
  }

  if (const clang::RecordDecl *record = pointee->getAsRecordDecl()) {
    plan.fieldwise = true;
    collectScalars(record, {}, {}, false, plan);
    if (!plan.fieldwise) plan.scalars.clear(); // the bulk fill covers them
  } else if (std::optional<std::string> suffix = scalarFieldSuffix(pointee, false)) {
    plan.scalars.push_back({{}, {}, *suffix});
    plan.fieldwise = true;
  }

  plan.viable = true;
  return plan;
}

/**
 * @brief A pointer havocked in statement position: setup plus the argument.
 */
struct PointerStorage {
  std::string decls; ///< Prologue statements, indented and newline-terminated.
  std::string arg;
  bool cstring = false;
};

namespace havoc_detail {

inline std::string loopVar(size_t i) { return "__i" + std::to_string(i); }

inline std::string indexed(const std::string &name, size_t count) {
  std::string out = name;
  for (size_t i = 0; i < count; ++i)
    out += "[" + loopVar(i) + "]";
  return out;
}

/**
 * @brief Emits loops over @p dims then @p fieldDims assigning @p value to
 * @p path within every element of @p name.
 *
 * @param looped Leading entries of @p dims the caller has already opened loops for.
 */
inline void assignEach(const std::string &name, const std::vector<std::string> &dims,
                       const std::vector<std::string> &path, const std::vector<uint64_t> &fieldDims,
                       const std::string &value, const std::string &indent, std::string &out,
                       size_t looped = 0) {
  std::vector<std::string> loopDims = dims;
  for (uint64_t dim : fieldDims)
    loopDims.push_back(std::to_string(dim));
  std::string lhs = indexed(name, dims.size());
  size_t var = dims.size();
  for (const std::string &segment : path)
    lhs += segment == "[]" ? "[" + loopVar(var++) + "]" : segment;
  for (size_t i = looped; i < loopDims.size(); ++i)
    out += indent + std::string(2 * i, ' ') + "for (int " + loopVar(i) + " = 0; " + loopVar(i) +
           " < " + loopDims[i] + "; ++" + loopVar(i) + ")\n";
  out += indent + std::string(2 * loopDims.size(), ' ') + lhs + " = " + value + ";\n";
}

/**
 * @brief Assigns every scalar field of every element of @p name a nondet
 * value, one loop nest over @p dims building each element whole.
 */
inline void fillFieldwise(const PointerPlan &plan, const std::string &name,
                          const std::vector<std::string> &dims, const std::string &indent,
                          std::string &out) {
  if (plan.scalars.empty()) return;
  const char *open = plan.scalars.size() > 1 ? " {\n" : "\n";
  for (size_t i = 0; i < dims.size(); ++i)
    out += indent + std::string(2 * i, ' ') + "for (int " + loopVar(i) + " = 0; " + loopVar(i) +
           " < " + dims[i] + "; ++" + loopVar(i) + ")" + open;
  for (const ScalarField &field : plan.scalars)
    assignEach(name, dims, field.path, field.dims, "__VERIFIER_nondet_" + field.suffix + "()",
               indent, out, dims.size());
  if (plan.scalars.size() == 1) return;
  for (size_t i = dims.size(); i-- > 0;)
    out += indent + std::string(2 * i, ' ') + "}\n";
}

} // namespace havoc_detail

/**
 * @brief Declares stack storage for a viable pointer plan and fills it with
 * nondet values (in bulk, or field by field when @c plan.fieldwise), for use
 * in statement position.
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
  if (!plan.viable) return out;

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
  if (plan.shape == PointerShape::Array) count = std::to_string(plan.elems);
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

  if (plan.fieldwise)
    havoc_detail::fillFieldwise(plan, name, {count}, indent, out.decls);
  else
    out.decls += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  out.arg = name;
  return out;
}
