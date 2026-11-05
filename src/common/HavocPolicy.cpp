// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "HavocPolicy.hpp"
#include "VerifierNames.hpp"

#include <algorithm>
#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>
#include <optional>

/**
 * @file HavocPolicy.cpp
 * @brief Implements @ref planPointer and @ref renderPointerStorage; every helper is file-local.
 *
 * Planning: @c classifyPointee picks the storage shape for a pointee, @c collectFields
 * walks a record for the pointers inside it, and @c addField plans each of those one
 * level deeper. Any rejection (past the depth bound, unspellable, unassignable)
 * propagates up and makes the whole plan non-viable.
 *
 * Rendering: @c renderStorage declares a plan's storage, fills it (a typed nondet
 * per scalar when the plan is fieldwise, else one @c __VERIFIER_nondet_memory), then
 * recursively declares each field's storage and assigns the field to it, one row per
 * parent element.
 */

namespace {

/**
 * @brief The file-scope declaration a cast to @p pointee needs, or "" if none.
 */
std::string pointeeFwdDecl(clang::QualType pointee, const clang::SourceManager &mgr) {
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
bool isSizedPointee(clang::QualType pointee, const clang::SourceManager &mgr) {
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
bool isSpellable(clang::QualType type) {
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

bool classifyPointee(clang::QualType pointee, const clang::SourceManager &mgr,
                     unsigned depth, PointerPlan &plan);

/**
 * @brief Plans the pointer of @p type at @p path in each element of @p parent
 * and appends it to @c parent.fields.
 *
 * @param depth Levels remaining for @p parent.
 * @return False if it can't be given storage: a function pointer, an
 * unspellable pointee, or past the depth bound.
 */
bool addField(PointerPlan &parent, std::vector<const clang::FieldDecl *> path,
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
bool holdsPointer(clang::QualType type, unsigned depth = 0) {
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
 * @brief The nondet suffix a storage element or record field of @p type is
 * assigned from, or nullopt if it has none and the storage must be bulk-filled instead.
 */
std::optional<std::string> scalarFieldSuffix(clang::QualType type, bool readOnly) {
  if (readOnly) return std::nullopt; // only nondet_memory can write a const member
  if (const auto *enumType = type->getAs<clang::EnumType>())
    return verifierSuffixForType(enumType->getDecl()->getIntegerType());
  return verifierSuffixForType(type);
}

/**
 * @brief Appends a field for every pointer and a scalar for every
 * nondet-assignable scalar reachable by value inside @p record, clearing
 * @c plan.fieldwise if some field is neither.
 *
 * @param readOnly The record is reached through a const member.
 * @param enclosing Records being walked above this one.
 * @return False if the record has a flexible array member, a pointer can't be
 * assigned (a const member or a union member), or @ref addField rejects it.
 */
bool collectFields(const clang::RecordDecl *record,
                   const std::vector<const clang::FieldDecl *> &path, bool readOnly,
                   const clang::SourceManager &mgr, unsigned depth, PointerPlan &plan,
                   std::vector<const clang::RecordDecl *> enclosing = {}) {
  const clang::RecordDecl *def = record->getDefinition();
  if (!def || def->isUnion()) { // assigning one member would clobber its siblings
    plan.fieldwise = false;
    if (def)
      for (const clang::FieldDecl *field : def->fields())
        if (holdsPointer(field->getType())) return false; // a member pointer is never initialized
    return true;
  }
  if (def->hasFlexibleArrayMember()) return false; // the tail is outside sizeof, so outside every element
  // Error recovery can leave a record containing itself by value.
  if (std::find(enclosing.begin(), enclosing.end(), def) != enclosing.end()) {
    plan.fieldwise = false;
    return true;
  }
  enclosing.push_back(def);
  for (const clang::FieldDecl *field : def->fields()) {
    if (field->isInvalidDecl() || field->isUnnamedBitField()) continue; // padding, not addressable
    std::vector<const clang::FieldDecl *> fieldPath = path;
    fieldPath.push_back(field);
    clang::QualType type = field->getType();
    while (const auto *array =
               llvm::dyn_cast_or_null<clang::ConstantArrayType>(type->getAsArrayTypeUnsafe()))
      type = array->getElementType();
    bool fieldReadOnly = readOnly || type.isConstQualified();
    if (type->isAnyPointerType()) {
      if (fieldReadOnly || !addField(plan, fieldPath, type, mgr, depth)) return false;
    } else if (const clang::RecordDecl *nested = type->getAsRecordDecl()) {
      if (!collectFields(nested, fieldPath, fieldReadOnly, mgr, depth, plan, enclosing))
        return false;
    } else if (std::optional<std::string> suffix = scalarFieldSuffix(type, fieldReadOnly)) {
      plan.scalars.push_back({fieldPath, *suffix});
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
 * @return False if no storage can be built, including when an element holds a
 * pointer past the depth bound.
 */
bool classifyPointee(clang::QualType pointee, const clang::SourceManager &mgr,
                     unsigned depth, PointerPlan &plan) {
  if (pointee->isAnyCharacterType()) {
    plan.shape = PointerShape::CString;
    return true;
  }
  if (pointee->isAnyPointerType()) { // storage is unqualified, so a const element still assigns
    plan.fieldwise = true;
    return addField(plan, {}, pointee, mgr, depth);
  }
  if (!isSizedPointee(pointee, mgr)) {
    plan.shape = PointerShape::Opaque;
    plan.fwdDecl = pointeeFwdDecl(pointee, mgr);
    return true;
  }
  if (const clang::RecordDecl *record = pointee->getAsRecordDecl()) {
    if (plan.shape != PointerShape::Array) plan.shape = PointerShape::Record;
    plan.fieldwise = true;
    if (!collectFields(record, {}, false, mgr, depth, plan)) return false;
    if (!plan.fieldwise) plan.scalars.clear(); // the bulk fill covers them
    return true;
  }
  if (std::optional<std::string> suffix = scalarFieldSuffix(pointee, false)) {
    plan.scalars.push_back({{}, *suffix});
    plan.fieldwise = true;
  }
  return true;
}

/**
 * @brief Drops the fieldwise fill from @p plan and every plan beneath it.
 */
void useBulkFill(PointerPlan &plan) {
  plan.fieldwise = false;
  plan.scalars.clear();
  for (PointerPlan &field : plan.fields)
    useBulkFill(field);
}

/**
 * @brief The type whose sizeof floors an opaque block, or "" for none.
 */
std::string opaqueSizeofType(const PointerPlan &plan) {
  if (plan.shape != PointerShape::Opaque || !plan.opaqueSized) return "";
  clang::QualType type = plan.type->isArrayType() ? plan.type : plan.type->getPointeeType();
  return type.getUnqualifiedType().getAsString(); // clang nests array declarators, e.g. int (*[2])(int)
}

std::string elementCount(const PointerPlan &plan) {
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

std::string loopVar(size_t i) { return "__i" + std::to_string(i); }

std::string indexed(const std::string &name, size_t count) {
  std::string out = name;
  for (size_t i = 0; i < count; ++i)
    out += "[" + loopVar(i) + "]";
  return out;
}

/**
 * @brief The type of one element of @p plan's storage; null if @c plan.type is
 * neither a pointer nor a constant array.
 */
clang::QualType elementType(const PointerPlan &plan) {
  if (plan.type.isNull()) return {};
  if (const auto *array =
          llvm::dyn_cast_or_null<clang::ConstantArrayType>(plan.type->getAsArrayTypeUnsafe()))
    return array->getElementType();
  return plan.type->getPointeeType();
}

/**
 * @brief Appends the access to @p path within one element to @p lhs, and a loop
 * bound to @p dims for every array along it.
 */
void appendMemberAccess(const std::vector<const clang::FieldDecl *> &path, std::string &lhs,
                        std::vector<std::string> &dims) {
  for (const clang::FieldDecl *member : path) {
    if (!member->isAnonymousStructOrUnion()) lhs += "." + member->getName().str();
    clang::QualType type = member->getType();
    while (const auto *array =
               llvm::dyn_cast_or_null<clang::ConstantArrayType>(type->getAsArrayTypeUnsafe())) {
      lhs += "[" + loopVar(dims.size()) + "]";
      dims.push_back(std::to_string(array->getSize().getZExtValue()));
      type = array->getElementType();
    }
  }
}

/**
 * @brief Emits loops over @p dims then @c lhs = value; innermost.
 *
 * @param looped Leading entries of @p dims the caller has already opened loops for.
 */
void emitAssign(const std::string &lhs, const std::vector<std::string> &dims,
                const std::string &value, const std::string &indent, std::string &out,
                size_t looped = 0) {
  for (size_t i = looped; i < dims.size(); ++i)
    out += indent + std::string(2 * i, ' ') + "for (int " + loopVar(i) + " = 0; " + loopVar(i) +
           " < " + dims[i] + "; ++" + loopVar(i) + ")\n";
  out += indent + std::string(2 * dims.size(), ' ') + lhs + " = " + value + ";\n";
}

/**
 * @brief Assigns every scalar of every element of @p name a nondet value, one
 * loop nest over @p dims building each element whole.
 */
void fillFieldwise(const PointerPlan &plan, const std::string &name,
                   const std::vector<std::string> &dims, const std::string &indent,
                   std::string &out) {
  if (plan.scalars.empty()) return;
  const char *open = plan.scalars.size() > 1 ? " {\n" : "\n";
  for (size_t i = 0; i < dims.size(); ++i)
    out += indent + std::string(2 * i, ' ') + "for (int " + loopVar(i) + " = 0; " + loopVar(i) +
           " < " + dims[i] + "; ++" + loopVar(i) + ")" + open;
  for (const ScalarField &scalar : plan.scalars) {
    std::string lhs = indexed(name, dims.size());
    std::vector<std::string> loopDims = dims;
    appendMemberAccess(scalar.path, lhs, loopDims);
    emitAssign(lhs, loopDims, "__VERIFIER_nondet_" + scalar.suffix + "()", indent, out,
               dims.size());
  }
  if (plan.scalars.size() == 1) return;
  for (size_t i = dims.size(); i-- > 0;)
    out += indent + std::string(2 * i, ' ') + "}\n";
}

/**
 * @brief Emits @c name[outer...][count] storage for @p plan, fills it (field by
 * field when @c plan.fieldwise, else in bulk), and assigns every pointer field
 * in every element, recursing into fields first.
 */
void renderStorage(const PointerPlan &plan, const std::string &name,
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
  if (plan.fieldwise)
    fillFieldwise(plan, name, dims, indent, out);
  else
    out += indent + "__VERIFIER_nondet_memory(" + name + ", sizeof(" + name + "));\n";
  for (size_t f = 0; f < plan.fields.size(); ++f) {
    const PointerPlan &field = plan.fields[f];
    std::vector<std::string> loopDims = dims;
    std::string lhs = indexed(name, dims.size());
    appendMemberAccess(field.path, lhs, loopDims);

    std::string storage = name + "_" + std::to_string(f);
    renderStorage(field, storage, loopDims, indent, out);
    std::string value = indexed(storage, loopDims.size());
    if (field.shape == PointerShape::CString)
      value = "__havoc_cstring_fill(" + value + ", __HAVOC_STR_MAX)";
    else if (field.shape == PointerShape::Opaque)
      value = "(" + field.type.getAsString() + ")" + value;
    emitAssign(lhs, loopDims, value, indent, out);
  }
}

} // namespace

PointerPlan planPointer(clang::QualType QT, const clang::SourceManager &mgr,
                        const HavocBounds &havoc) {
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

  if (havoc.pointerDepth == 0) {
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

  plan.viable = classifyPointee(pointee, mgr, havoc.pointerDepth, plan);
  if (!havoc.fieldwise) useBulkFill(plan); // viability is unaffected; only the fill changes
  return plan;
}

PointerStorage renderPointerStorage(const PointerPlan &plan, const std::string &name,
                                    const std::string &castType, const std::string &indent) {
  PointerStorage out;
  if (!plan.viable) return out;
  if (plan.shape != PointerShape::Opaque && elementType(plan).isNull()) return out;

  renderStorage(plan, name, {}, indent, out.decls);
  if (plan.shape == PointerShape::CString) {
    out.arg = "__havoc_cstring_fill(" + name + ", " + elementCount(plan) + ")";
  } else if (plan.shape == PointerShape::Opaque) {
    out.arg = castType.empty() ? name : "(" + castType + ")" + name;
  } else {
    out.arg = name;
  }
  return out;
}
