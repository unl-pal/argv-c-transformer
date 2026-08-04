// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "AddStdIncludesConsumer.hpp"
#include "StdHeaders.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <set>

namespace {

/**
 * @brief Records the standard headers required by types referenced in the main file.
 *
 * Visits every TypeLoc in the main file and resolves it to a standard header
 * via recordType, requiring the resolved declaration to live in a system
 * header (guards against a local symbol sharing a standard name).
 */
class TypeCollector : public clang::RecursiveASTVisitor<TypeCollector> {
public:
  TypeCollector(clang::SourceManager &SM) : _SM(SM) {}

  bool VisitTypeLoc(clang::TypeLoc TL) {
    if (!_SM.isInMainFile(TL.getBeginLoc()))
      return true;
    recordType(TL.getType());
    return true;
  }

  const std::set<std::string> &neededHeaders() const { return _NeededHeaders; }

private:
  /**
   * @brief Looks up a type's standard header, if it has one.
   *
   * TypedefType/RecordType: look up the underlying Decl's name in StdHeaders,
   * requiring the Decl's location to be in a system header. BuiltinType: only
   * `bool` is special-cased (no Decl exists to check; `bool`/`_Bool` are
   * indistinguishable once macro-expanded, so this is a best-effort match).
   */
  void recordType(clang::QualType QT) {
    if (QT.isNull())
      return;

    const clang::Type *T = QT.getTypePtrOrNull();
    if (!T)
      return;

    if (const auto *TDT = T->getAs<clang::TypedefType>()) {
      std::string name = TDT->getDecl()->getNameAsString();
      auto it = StdHeaders.find(name);
      if (it != StdHeaders.end())
        if (_SM.isInSystemHeader(TDT->getDecl()->getLocation()))
          _NeededHeaders.insert(it->second);
    } else if (const auto *RT = T->getAs<clang::RecordType>()) {
      std::string name = RT->getDecl()->getNameAsString();
      auto it = StdHeaders.find(name);
      if (it != StdHeaders.end())
        if (_SM.isInSystemHeader(RT->getDecl()->getLocation()))
          _NeededHeaders.insert(it->second);
    } else if (const auto *BT = T->getAs<clang::BuiltinType>()) {
      if (BT->getKind() == clang::BuiltinType::Bool) {
        _NeededHeaders.insert("stdbool.h"); // special case
      }
    }
  }
  clang::SourceManager &_SM;
  std::set<std::string> _NeededHeaders;
};

/**
 * @brief Records the standard headers required by functions called in the main file.
 *
 * Visits every CallExpr in the main file and resolves its callee to a
 * standard header via recordFunction. A resolved callee must have its
 * declaration in a system header (same collision guard as TypeCollector). An
 * implicit callee (unresolved because its declaring header is missing) has
 * no such location to check, so it's matched on name alone - a best-effort
 * guess backstopped by the verify stage's compile check.
 */
class FunctionCollector : public clang::RecursiveASTVisitor<FunctionCollector> {
public:
  FunctionCollector(clang::SourceManager &SM) : _SM(SM) {}

  bool VisitCallExpr(clang::CallExpr *CE) {
    if (!_SM.isInMainFile(CE->getBeginLoc()))
      return true;
    if (const clang::FunctionDecl *FD = CE->getDirectCallee()) {
      if (FD->isImplicit() || _SM.isInSystemHeader(FD->getBeginLoc()))
        recordFunction(FD);
      return true;
    }
    return true;
  }

  const std::set<std::string> &neededHeaders() const { return _NeededHeaders; }

private:
  void recordFunction(const clang::FunctionDecl *FD) {
    std::string name = FD->getNameAsString();
    auto it = StdHeaders.find(name);
    if (it != StdHeaders.end())
      _NeededHeaders.insert(it->second);
  }

  clang::SourceManager &_SM;
  std::set<std::string> _NeededHeaders;
};
}// namespace

AddStdIncludesConsumer::AddStdIncludesConsumer(
    std::shared_ptr<std::set<std::string>> existingIncludes, clang::Rewriter &rewriter)
    : _ExistingIncludes(existingIncludes), _Rewriter(rewriter) {
}

void AddStdIncludesConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &SM = Context.getSourceManager();

  TypeCollector typeCollector(SM);
  typeCollector.TraverseDecl(Context.getTranslationUnitDecl());

  FunctionCollector funCollector(SM);
  funCollector.TraverseDecl(Context.getTranslationUnitDecl());

  std::string includes;
  for (const std::string &header : typeCollector.neededHeaders()) {
    if (_ExistingIncludes->count(header))
      continue;
    includes += "#include <" + header + ">\n";
    _ExistingIncludes->insert(header);
  }
  for (const std::string &header : funCollector.neededHeaders()) {
    if (_ExistingIncludes->count(header))
      continue;
    includes += "#include <" + header + ">\n";
    _ExistingIncludes->insert(header);
  }

  if (includes.empty())
    return;

  clang::SourceLocation loc = SM.translateLineCol(SM.getMainFileID(), 1, 1);
  _Rewriter.InsertTextBefore(loc, includes);
}
