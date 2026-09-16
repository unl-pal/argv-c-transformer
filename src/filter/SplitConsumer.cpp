// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "SplitConsumer.hpp"
#include "RemoveVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>
#include <vector>

SplitConsumer::SplitConsumer(std::shared_ptr<std::vector<std::string>> toRemove,
                             std::shared_ptr<HeaderClosureState> closureState,
                             SplitOutputWriter writeOutput)
    : _ToRemove(toRemove), _ClosureState(closureState), _WriteOutput(std::move(writeOutput)) {}

void SplitConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  clang::SourceManager &mgr = context.getSourceManager();
  const clang::LangOptions &langOpts = context.getLangOpts();
  std::set<std::string> rejected(_ToRemove->begin(), _ToRemove->end());

  std::vector<std::string> survivors;
  for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
    const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!func || !func->doesThisDeclarationHaveABody() || !func->getBody())
      continue;
    if (!mgr.isInMainFile(mgr.getFileLoc(func->getLocation())))
      continue;
    std::string name = func->getNameAsString();
    if (!rejected.count(name))
      survivors.push_back(name);
  }

  // Every other survivor is stripped exactly like a rejected function, so
  // each target's output is self-contained and its harness calls only it.
  for (const std::string &target : survivors) {
    auto effectiveToRemove = std::make_shared<std::vector<std::string>>(*_ToRemove);
    for (const std::string &other : survivors)
      if (other != target)
        effectiveToRemove->push_back(other);

    clang::Rewriter rewriter;
    rewriter.setSourceMgr(mgr, langOpts);
    for (const clang::CharSourceRange &range : _ClosureState->localIncludeRanges)
      rewriter.RemoveText(range);

    if (!effectiveToRemove->empty()) {
      RemoveVisitor remover(rewriter, effectiveToRemove);
      remover.TraverseDecl(context.getTranslationUnitDecl());
    }

    HeaderClosureConsumer closure(rewriter, effectiveToRemove, _ClosureState);
    closure.HandleTranslationUnit(context);

    std::string text;
    llvm::raw_string_ostream os(text);
    rewriter.getEditBuffer(mgr.getMainFileID()).write(os);
    _WriteOutput(target, text);
  }
}
