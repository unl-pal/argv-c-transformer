// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HavocBounds.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <cstdint>
#include <llvm/Support/Casting.h>
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
 * @brief A decision about one pointer type: what shape, and any declaration its
 * spelling needs.
 */
struct PointerPlan {
  clang::QualType type; ///< The pointer or constant-array type planned.
  std::vector<const clang::FieldDecl *> path; ///< Members to this pointer from a parent element; empty at the root.
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;
  std::string fwdDecl; ///< For an opaque pointee.
  bool opaqueSized = false; ///< An opaque block is floored at the planned type's sizeof.
  std::vector<PointerPlan> fields; ///< Pointers inside each element, assigned after the nondet fill.
};

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
 * @brief True if @c sizeof(pointee) is usable in the output: complete, and a
 * tag's definition lives in the main file rather than a stripped header.
 */
inline bool isSizedPointee(clang::QualType pointee, const clang::SourceManager &mgr) {
  if (pointee->isVoidType() || pointee->isIncompleteType()) return false;
  if (const clang::TagDecl *tag = pointee->getAsTagDecl()) {
    const clang::TagDecl *def = tag->getDefinition();
    return def && mgr.isInMainFile(def->getLocation());
  }
  return true;
}

/**
 * @brief False if @p type bottoms out in an anonymous tag with no typedef,
 * which has no spelling for a storage declaration or cast.
 */
inline bool isSpellable(clang::QualType type) {
  while (!type->getAs<clang::TypedefType>()) {
    if (type->isAnyPointerType()) type = type->getPointeeType();
    else if (const clang::ArrayType *array = type->getAsArrayTypeUnsafe())
      type = array->getElementType();
    else {
      const clang::TagDecl *tag = type->getAsTagDecl();
      return !tag || !tag->getName().empty();
    }
  }
  return true;
}

inline bool classifyPointee(clang::QualType pointee, const clang::SourceManager &mgr,
                            unsigned depth, PointerPlan &plan);

/**
 * @brief Plans the pointer of @p type at @p path in each element of @p parent
 * and appends it to @c parent.fields.
 *
 * @param depth Levels remaining for @p parent.
 * @return False if it can't be given storage: a function pointer, an
 * unspellable pointee, or past the depth bound.
 */
inline bool addField(PointerPlan &parent, std::vector<const clang::FieldDecl *> path,
                     clang::QualType type, const clang::SourceManager &mgr, unsigned depth) {
  if (type->isFunctionPointerType() || depth == 1 || !isSpellable(type)) return false;
  PointerPlan field;
  field.type = type;
  field.path = std::move(path);
  field.shape = PointerShape::Block;
  if (!classifyPointee(type->getPointeeType(), mgr, depth - 1, field)) return false;
  field.viable = true;
  parent.fields.push_back(std::move(field));
  return true;
}

/**
 * @brief True if @p type, or any field reachable by value inside it, is a pointer.
 */
inline bool holdsPointer(clang::QualType type, unsigned depth = 0) {
  while (const clang::ArrayType *array = type->getAsArrayTypeUnsafe())
    type = array->getElementType();
  if (type->isAnyPointerType()) return true;
  const clang::RecordDecl *record = type->getAsRecordDecl();
  const clang::RecordDecl *def = record ? record->getDefinition() : nullptr;
  if (!def || depth > 8) return false;
  for (const clang::FieldDecl *field : def->fields())
    if (holdsPointer(field->getType(), depth + 1)) return true;
  return false;
}

/**
 * @brief Appends a field for every pointer reachable by value inside @p record.
 *
 * @param readOnly The record is reached through a const member.
 * @param enclosing Records being walked above this one.
 * @return False if a pointer can't be assigned (a const member, a union member,
 * or inside a flexible array member) or @ref addField rejects it.
 */
inline bool collectFields(const clang::RecordDecl *record,
                          const std::vector<const clang::FieldDecl *> &path, bool readOnly,
                          const clang::SourceManager &mgr, unsigned depth, PointerPlan &plan,
                          std::vector<const clang::RecordDecl *> enclosing = {}) {
  const clang::RecordDecl *def = record->getDefinition();
  if (!def || def->isUnion()) { // assigning one member would clobber its siblings
    if (def)
      for (const clang::FieldDecl *field : def->fields())
        if (holdsPointer(field->getType())) return false; // a member pointer is never initialized
    return true;
  }
  // Error recovery can leave a record containing itself by value.
  if (std::find(enclosing.begin(), enclosing.end(), def) != enclosing.end()) return true;
  enclosing.push_back(def);
  for (const clang::FieldDecl *field : def->fields()) {
    if (field->isInvalidDecl()) continue;
    std::vector<const clang::FieldDecl *> fieldPath = path;
    fieldPath.push_back(field);
    clang::QualType type = field->getType();
    bool flexible = false;
    while (const clang::ArrayType *array = type->getAsArrayTypeUnsafe()) {
      const auto *constant = llvm::dyn_cast<clang::ConstantArrayType>(array);
      if (!constant) {
        flexible = true;
        break;
      }
      type = constant->getElementType();
    }
    if (flexible) { // outside sizeof, so outside the storage
      if (holdsPointer(type)) return false;
      continue;
    }
    bool fieldReadOnly = readOnly || type.isConstQualified();
    if (type->isAnyPointerType()) {
      if (fieldReadOnly || !addField(plan, fieldPath, type, mgr, depth)) return false;
    } else if (const clang::RecordDecl *nested = type->getAsRecordDecl()) {
      if (!collectFields(nested, fieldPath, fieldReadOnly, mgr, depth, plan, enclosing))
        return false;
    }
  }
  return true;
}

/**
 * @brief Classifies storage for elements of @p pointee into @p plan, planning
 * the pointers inside each element @p depth - 1 levels further.
 *
 * @param depth Levels remaining, including this storage; at least 1.
 * @return False if no storage can be built, including when an element holds a
 * pointer past the depth bound.
 */
inline bool classifyPointee(clang::QualType pointee, const clang::SourceManager &mgr,
                            unsigned depth, PointerPlan &plan) {
  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    return true;
  }
  if (pointee->isAnyPointerType()) // storage is unqualified, so a const element still assigns
    return addField(plan, {}, pointee, mgr, depth);
  if (!isSizedPointee(pointee, mgr)) {
    plan.shape = PointerShape::Opaque;
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
    return true;
  }
  if (const clang::RecordDecl *record = pointee->getAsRecordDecl()) {
    if (plan.shape != PointerShape::Array) plan.shape = PointerShape::Record;
    return collectFields(record, {}, false, mgr, depth, plan);
  }
  return true;
}

/**
 * @brief Classifies a pointer (or array) type into a havoc plan.
 *
 * @param QT Pass @c ParmVarDecl::getOriginalType(), not @c getType(), to avoid array decay.
 * @return The plan; check @c viable before using it.
 */
inline PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr,
                               unsigned depth = 1) {
  PointerPlan plan;
  if (QT.isNull() || QT.getTypePtrOrNull() == nullptr) return plan;
  plan.type = QT;

  clang::QualType pointee;
  if (const auto *arrayType =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(QT->getAsArrayTypeUnsafe())) {
    pointee = arrayType->getElementType();
    plan.shape = PointerShape::Array;
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
  if (!isSpellable(QT)) return plan; // no name for the storage or the cast

  if (depth == 0) {
    if (pointee->isAnyCharacterType()) {
      plan.shape = PointerShape::CString;
    } else {
      plan.shape = PointerShape::Opaque;
      plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
      plan.opaqueSized = isSizedPointee(pointee, mgr);
    }
    plan.viable = true;
    return plan;
  }

  plan.viable = classifyPointee(pointee, mgr, depth, plan);
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

/**
 * @brief The type whose sizeof floors an opaque block, or "" for none.
 */
inline std::string opaqueSizeofType(const PointerPlan &plan) {
  if (plan.shape != PointerShape::Opaque || !plan.opaqueSized) return "";
  clang::QualType type = plan.type->isArrayType() ? plan.type : plan.type->getPointeeType();
  return type.getUnqualifiedType().getAsString(); // clang nests array declarators, e.g. int (*[2])(int)
}

inline std::string elementCount(const PointerPlan &plan) {
  switch (plan.shape) {
  case PointerShape::Array:
    return std::to_string(
        llvm::cast<clang::ConstantArrayType>(plan.type->getAsArrayTypeUnsafe())->getSize().getZExtValue());
  case PointerShape::CString:
    return "__HAVOC_STR_MAX";
  case PointerShape::Opaque: {
    std::string sized = opaqueSizeofType(plan);
    if (sized.empty()) return "__HAVOC_BLOCK_MAX";
    return "sizeof(" + sized + ") > __HAVOC_BLOCK_MAX ? sizeof(" + sized + ") : __HAVOC_BLOCK_MAX";
  }
  default:
    return "__HAVOC_ARRAY_ELEMS";
  }
}

inline std::string loopVar(size_t i) { return "__i" + std::to_string(i); }

inline std::string indexed(const std::string &name, size_t count) {
  std::string out = name;
  for (size_t i = 0; i < count; ++i)
    out += "[" + loopVar(i) + "]";
  return out;
}

/**
 * @brief The type of one element of @p plan's storage; null if @c plan.type is
 * neither a pointer nor a constant array.
 */
inline clang::QualType elementType(const PointerPlan &plan) {
  if (plan.type.isNull()) return {};
  if (const auto *array =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(plan.type->getAsArrayTypeUnsafe()))
    return array->getElementType();
  return plan.type->getPointeeType();
}

/**
 * @brief Emits @c name[outer...][count] storage for @p plan, fills it with
 * nondet bytes, and assigns every pointer field in every element, recursing
 * into fields first.
 */
inline void renderStorage(const PointerPlan &plan, const std::string &name,
                          const std::vector<std::string> &outer, const std::string &indent,
                          std::string &out) {
  std::string count = elementCount(plan);
  std::string decl = name;
  for (const std::string &dim : outer)
    decl += "[" + dim + "]";
  decl += "[" + count + "]";
  if (plan.shape == PointerShape::Opaque) decl = "unsigned char " + decl; // alignment potentially an issue
  else // appends an n-dimensional element's own bounds after ours
    elementType(plan).getUnqualifiedType().getAsStringInternal(decl, clang::LangOptions());
  out += indent + decl + ";\n";
  if (plan.shape == PointerShape::CString) return; // filled where it is referenced

  std::vector<std::string> dims = outer;
  dims.push_back(count);
  out += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  for (size_t f = 0; f < plan.fields.size(); ++f) {
    const PointerPlan &field = plan.fields[f];
    std::vector<std::string> loopDims = dims;
    std::string lhs = indexed(name, dims.size());
    for (const clang::FieldDecl *member : field.path) {
      if (!member->isAnonymousStructOrUnion()) lhs += "." + member->getName().str();
      clang::QualType type = member->getType();
      while (const auto *array =
                 llvm::dyn_cast_or_null<clang::ConstantArrayType>(type->getAsArrayTypeUnsafe())) {
        lhs += "[" + loopVar(loopDims.size()) + "]";
        loopDims.push_back(std::to_string(array->getSize().getZExtValue()));
        type = array->getElementType();
      }
    }

    std::string storage = name + "_" + std::to_string(f);
    renderStorage(field, storage, loopDims, indent, out);
    std::string value = indexed(storage, loopDims.size());
    if (field.shape == PointerShape::CString)
      value = "__havoc_cstring_fill(" + value + ", __HAVOC_STR_MAX)";
    else if (field.shape == PointerShape::Opaque)
      value = "(" + field.type.getAsString() + ")" + value;
    for (size_t i = 0; i < loopDims.size(); ++i)
      out += indent + std::string(2 * i, ' ') + "for (int " + loopVar(i) + " = 0; " + loopVar(i) +
             " < " + loopDims[i] + "; ++" + loopVar(i) + ")\n";
    out += indent + std::string(2 * loopDims.size(), ' ') + lhs + " = " + value + ";\n";
  }
}

} // namespace havoc_detail

/**
 * @brief Declares stack storage for a viable pointer plan and fills it with
 * nondet values, for use in statement position.
 *
 * @param name     Unique local name for the storage; nested storage appends suffixes to it.
 * @param castType Cast applied only to the opaque byte buffer. Empty to omit.
 * @param indent   Leading whitespace for each emitted statement line.
 * @return The setup and argument; empty when the plan is not viable.
 */
inline PointerStorage renderPointerStorage(const PointerPlan &plan, const std::string &name,
                                           const std::string &castType,
                                           const std::string &indent = "  ") {
  PointerStorage out;
  if (!plan.viable) return out;
  if (plan.shape != PointerShape::Opaque && havoc_detail::elementType(plan).isNull()) return out;

  havoc_detail::renderStorage(plan, name, {}, indent, out.decls);
  if (plan.shape == PointerShape::CString) {
    out.arg = "__havoc_cstring_fill(" + name + ", " + havoc_detail::elementCount(plan) + ")";
    out.cstring = true;
  } else if (plan.shape == PointerShape::Opaque) {
    out.arg = castType.empty() ? name : "(" + castType + ")" + name;
  } else {
    out.arg = name;
  }
  return out;
}
