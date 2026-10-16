// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "HavocBounds.hpp"
#include "VerifierNames.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <cstdint>
#include <llvm/Support/Casting.h>
#include <memory>
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
 *
 * The pointer depth selects how much structure is modelled. Depth 0 havocs
 * every pointer as an opaque byte block (char pointers stay strings). Depth
 * k >= 1 builds typed storage and initializes the pointers inside it k levels
 * deep; every pointer past the bound is set to 0. A generic model havocs every
 * pointer, char pointers included, as an opaque block of exactly
 * @c __HAVOC_BLOCK_MAX bytes.
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

struct PointerPlan;

/**
 * @brief A pointer inside each element of a plan's storage, assigned after the
 * nondet fill so no raw nondet pointer value survives.
 */
struct PointerSlot {
  std::vector<std::string> path; ///< From an element: ".member" segments, "[]" per entry of @c dims.
  std::vector<uint64_t> dims;
  clang::QualType type;
  std::shared_ptr<PointerPlan> child; ///< Null: the slot is set to 0.
};

/**
 * @brief A scalar inside each element of a record plan's storage, assigned a
 * @c __VERIFIER_nondet_<suffix>() value.
 */
struct ScalarField {
  std::vector<std::string> path; ///< As in @ref PointerSlot.
  std::vector<uint64_t> dims;
  std::string suffix;
};

/**
 * @brief A decision about one pointer type: what shape, and any declaration its
 * spelling needs.
 */
struct PointerPlan {
  PointerShape shape = PointerShape::Opaque;
  bool viable = false;
  std::string fwdDecl;      ///< For an opaque pointee.
  unsigned elems = 0;       ///< Constant array size.
  std::string opaqueSizeof; ///< Type whose sizeof floors an opaque block; "" for none.
  std::vector<PointerSlot> slots;
  std::vector<ScalarField> scalars;
  bool fieldwise = false; ///< @c scalars and @c slots cover every field, so no bulk fill is needed.
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
 * @brief Plans the storage a pointer slot points to, or null to set it to 0:
 * past the depth bound, or an unspellable pointee.
 */
inline std::shared_ptr<PointerPlan> planSlotTarget(clang::QualType type,
                                                   const clang::SourceManager &mgr,
                                                   unsigned depth) {
  if (depth == 0 || !isSpellable(type)) return nullptr;
  auto plan = std::make_shared<PointerPlan>();
  plan->shape = PointerShape::Block;
  if (!classifyPointee(type->getPointeeType(), mgr, depth, *plan)) return nullptr;
  plan->viable = true;
  return plan;
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
 * @brief Appends a slot for every pointer and a scalar field for every
 * nondet-assignable scalar reachable by value inside @p record, clearing
 * @c plan.fieldwise if some field is neither.
 *
 * @param readOnly The record is reached through a const member.
 * @param enclosing Records being walked above this one.
 * @return False if a pointer can't be assigned (a const member) or is a function pointer.
 */
inline bool collectSlots(const clang::RecordDecl *record, const std::vector<std::string> &path,
                         const std::vector<uint64_t> &dims, bool readOnly,
                         const clang::SourceManager &mgr, unsigned depth, PointerPlan &plan,
                         std::vector<const clang::RecordDecl *> enclosing = {}) {
  const clang::RecordDecl *def = record->getDefinition();
  if (!def || def->isUnion()) { // assigning one member would clobber its siblings
    plan.fieldwise = false;
    return true;
  }
  // Error recovery can leave a record containing itself by value.
  if (std::find(enclosing.begin(), enclosing.end(), def) != enclosing.end()) {
    plan.fieldwise = false;
    return true;
  }
  enclosing.push_back(def);
  for (const clang::FieldDecl *field : def->fields()) {
    if (field->isInvalidDecl()) continue;
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
    if (type->isAnyPointerType()) {
      if (fieldReadOnly || type->isFunctionPointerType()) return false;
      plan.slots.push_back({fieldPath, fieldDims, type, planSlotTarget(type, mgr, depth - 1)});
    } else if (const clang::RecordDecl *nested = type->getAsRecordDecl()) {
      if (!collectSlots(nested, fieldPath, fieldDims, fieldReadOnly, mgr, depth, plan, enclosing))
        return false;
    } else if (std::optional<std::string> suffix = scalarFieldSuffix(type, fieldReadOnly)) {
      plan.scalars.push_back({fieldPath, fieldDims, *suffix});
    } else {
      plan.fieldwise = false;
    }
  }
  return true;
}

/**
 * @brief Classifies storage for elements of @p pointee into @p plan, planning
 * the pointers inside each element @p depth - 1 levels further.
 *
 * @param depth Levels remaining, including this storage; at least 1.
 * @return False if no storage can be built.
 */
inline bool classifyPointee(clang::QualType pointee, const clang::SourceManager &mgr,
                            unsigned depth, PointerPlan &plan) {
  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    return true;
  }
  if (pointee->isAnyPointerType()) { // storage is unqualified, so a const element still assigns
    if (pointee->isFunctionPointerType()) return false;
    plan.slots.push_back({{}, {}, pointee, planSlotTarget(pointee, mgr, depth - 1)});
    plan.fieldwise = true;
    return true;
  }
  if (!isSizedPointee(pointee, mgr)) {
    plan.shape = PointerShape::Opaque;
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
    return true;
  }
  if (const clang::RecordDecl *record = pointee->getAsRecordDecl()) {
    if (plan.shape != PointerShape::Array) plan.shape = PointerShape::Record;
    plan.fieldwise = true;
    if (!collectSlots(record, {}, {}, false, mgr, depth, plan)) return false;
    if (!plan.fieldwise) plan.scalars.clear(); // the bulk fill covers them
    return true;
  }
  if (std::optional<std::string> suffix = scalarFieldSuffix(pointee, false)) {
    plan.scalars.push_back({{}, {}, *suffix});
    plan.fieldwise = true;
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
                               PointerModel model = {}) {
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
  if (!isSpellable(QT)) return plan; // no name for the storage or the cast

  if (model.generic) {
    plan.shape = PointerShape::Opaque;
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
    plan.viable = true;
    return plan;
  }

  if (model.depth == 0) {
    if (pointee->isAnyCharacterType()) {
      plan.shape = PointerShape::CString;
    } else {
      plan.shape = PointerShape::Opaque;
      plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
      if (isSizedPointee(pointee, mgr)) // clang nests array declarators, e.g. int (*[2])(int)
        plan.opaqueSizeof = (QT->isArrayType() ? QT : pointee).getUnqualifiedType().getAsString();
    }
    plan.viable = true;
    return plan;
  }

  plan.viable = classifyPointee(pointee, mgr, model.depth, plan);
  if (!model.fieldwise) { // viability is unaffected; only the fill changes
    plan.fieldwise = false;
    plan.scalars.clear();
  }
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

inline std::string elementCount(const PointerPlan &plan) {
  switch (plan.shape) {
  case PointerShape::Array:
    return std::to_string(plan.elems);
  case PointerShape::CString:
    return "__HAVOC_STR_MAX";
  case PointerShape::Opaque:
    if (plan.opaqueSizeof.empty()) return "__HAVOC_BLOCK_MAX";
    return "sizeof(" + plan.opaqueSizeof + ") > __HAVOC_BLOCK_MAX ? sizeof(" + plan.opaqueSizeof +
           ") : __HAVOC_BLOCK_MAX";
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

/**
 * @brief Emits @c name[outer...][count] storage for @p plan, fills it (in bulk,
 * or field by field when @c plan.fieldwise), and assigns every slot in every
 * element, recursing into slot targets first.
 */
inline void renderStorage(const PointerPlan &plan, clang::QualType element, const std::string &name,
                          const std::vector<std::string> &outer, const std::string &indent,
                          std::string &out) {
  std::string count = elementCount(plan);
  std::string decl = name;
  for (const std::string &dim : outer)
    decl += "[" + dim + "]";
  decl += "[" + count + "]";
  if (plan.shape == PointerShape::Opaque) decl = "unsigned char " + decl; // alignment potentially an issue
  else // appends an n-dimensional element's own bounds after ours
    element.getUnqualifiedType().getAsStringInternal(decl, clang::LangOptions());
  out += indent + decl + ";\n";
  if (plan.shape == PointerShape::CString) return; // filled where it is referenced

  std::vector<std::string> dims = outer;
  dims.push_back(count);
  if (plan.fieldwise)
    fillFieldwise(plan, name, dims, indent, out);
  else
    out += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  for (size_t s = 0; s < plan.slots.size(); ++s) {
    const PointerSlot &slot = plan.slots[s];
    std::vector<std::string> loopDims = dims;
    for (uint64_t dim : slot.dims)
      loopDims.push_back(std::to_string(dim));

    std::string value = "0";
    if (slot.child) {
      std::string target = name + "_" + std::to_string(s);
      renderStorage(*slot.child, slot.type->getPointeeType(), target, loopDims, indent, out);
      std::string row = indexed(target, loopDims.size());
      if (slot.child->shape == PointerShape::CString)
        value = "__havoc_cstring_fill(" + row + ", __HAVOC_STR_MAX)";
      else if (slot.child->shape == PointerShape::Opaque)
        value = "(" + slot.type.getAsString() + ")" + row;
      else
        value = row;
    }
    assignEach(name, dims, slot.path, slot.dims, value, indent, out);
  }
}

} // namespace havoc_detail

/**
 * @brief Declares stack storage for a viable pointer plan and fills it with
 * nondet values, for use in statement position.
 *
 * @param declared The parameter's pre-decay type, or a call's return type.
 * @param name     Unique local name for the storage; nested storage appends suffixes to it.
 * @param castType Cast applied only to the opaque byte buffer. Empty to omit.
 * @param indent   Leading whitespace for each emitted statement line.
 * @return The setup and argument; empty when the plan is not viable.
 */
inline PointerStorage renderPointerStorage(const PointerPlan &plan, clang::QualType declared,
                                           const std::string &name, const std::string &castType,
                                           const std::string &indent = "  ") {
  PointerStorage out;
  if (!plan.viable) return out;

  clang::QualType element; // unused by an opaque block
  if (plan.shape != PointerShape::Opaque) {
    if (const auto *arrayType =
            llvm::dyn_cast_or_null<clang::ConstantArrayType>(declared->getAsArrayTypeUnsafe()))
      element = arrayType->getElementType();
    else if (declared->isAnyPointerType())
      element = declared->getPointeeType();
    else
      return out;
  }

  havoc_detail::renderStorage(plan, element, name, {}, indent, out.decls);
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
