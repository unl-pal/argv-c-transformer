// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "AddStdIncludesConsumer.hpp"
#include "StdHeaders.hpp"

#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <set>

namespace {

/**
 * @brief Records the standard headers required by implicitly-declared functions
 * called in the main file.
 *
 * Visits every CallExpr in the main file whose callee is a compiler-synthesized
 * implicit FunctionDecl - Sema's fallback when a function is called without any
 * visible declaration (a warning, not an error, so it never reaches
 * UnknownTypeDiagConsumer). Matched on name alone, since an implicit decl has no
 * real declaring location to resolve against; a best-effort guess backstopped
 * by the verify stage's compile check.
 */
class FunctionCollector : public clang::RecursiveASTVisitor<FunctionCollector> {
public:
  FunctionCollector(clang::SourceManager &SM) : _SM(SM) {}

  bool VisitCallExpr(clang::CallExpr *CE) {
    if (!_SM.isInMainFile(CE->getBeginLoc())) return true;
    if (const clang::FunctionDecl *FD = CE->getDirectCallee())
      if (FD->isImplicit()) recordFunction(FD);
    return true;
  }

  const std::set<std::string> &neededHeaders() const { return _NeededHeaders; }

private:
  void recordFunction(const clang::FunctionDecl *FD) {
    std::string name = FD->getNameAsString();
    auto it = StdHeaders.find(name);
    if (it != StdHeaders.end()) _NeededHeaders.insert(it->second);
  }

  clang::SourceManager &_SM;
  std::set<std::string> _NeededHeaders;
};
} // namespace

AddStdIncludesConsumer::AddStdIncludesConsumer(
    std::shared_ptr<std::set<std::string>> existingIncludes,
    std::shared_ptr<std::set<std::string>> unresolvedTypeNames, clang::Rewriter &rewriter)
    : _ExistingIncludes(existingIncludes), _UnresolvedTypeNames(unresolvedTypeNames),
      _Rewriter(rewriter) {}

void AddStdIncludesConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &SM = Context.getSourceManager();

  FunctionCollector funCollector(SM);
  funCollector.TraverseDecl(Context.getTranslationUnitDecl());

  std::string includes;
  for (const std::string &header : funCollector.neededHeaders()) {
    if (_ExistingIncludes->count(header)) continue;
    includes += "#include <" + header + ">\n";
    _ExistingIncludes->insert(header);
  }
  for (const std::string &name : *_UnresolvedTypeNames) {
    auto it = StdHeaders.find(name);
    if (it == StdHeaders.end() || _ExistingIncludes->count(it->second)) continue;
    includes += "#include <" + it->second + ">\n";
    _ExistingIncludes->insert(it->second);
  }

  if (includes.empty()) return;

  clang::SourceLocation loc = SM.translateLineCol(SM.getMainFileID(), 1, 1);
  _Rewriter.InsertTextBefore(loc, includes);
}
