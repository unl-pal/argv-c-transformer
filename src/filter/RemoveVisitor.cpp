#include "include/RemoveVisitor.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <string>
#include <vector>

RemoveVisitor::RemoveVisitor(clang::ASTContext *C, clang::Rewriter &rewriter,
                             std::vector<std::string> *toRemove,
                             std::set<std::string> *neededTypes)
    : _C(C), _Mgr(rewriter.getSourceMgr()), _Rewriter(rewriter), _ToRemove(toRemove),
      _NeededTypes(neededTypes) {}

bool RemoveVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D)
    return false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    // Macro-expanded locations are not writable by the Rewriter
    if (D->getLocation().isMacroID())
      return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);

    for (std::string &name : *_ToRemove) {
      if (name == D->getNameAsString() && !D->isMain()) {
        // Start at column 1 to capture any leading whitespace on that line
        clang::SourceLocation lineStart = _Mgr.translateLineCol(
            _Mgr.getMainFileID(), _Mgr.getSpellingLineNumber(D->getLocation()), 1);
        clang::SourceRange range(lineStart, D->getEndLoc());

        // For forward declarations getEndLoc() is the ')'; extend by 1 to include ';'
        if (!D->hasBody())
          range.setEnd(D->getEndLoc().getLocWithOffset(1));

        if (range.isValid())
          _Rewriter.RemoveText(range);

        // Remove attached doc comment if present
        clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D);
        if (rawComment && rawComment->getSourceRange().isValid())
          _Rewriter.RemoveText(rawComment->getSourceRange());
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveVisitor>::VisitFunctionDecl(D);
}

bool RemoveVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_Mgr.isInMainFile(E->getExprLoc()))
    return clang::RecursiveASTVisitor<RemoveVisitor>::VisitCallExpr(E);

  if (clang::Decl *calleeDecl = E->getCalleeDecl()) {
    if (clang::FunctionDecl *func = calleeDecl->getAsFunction()) {
      std::string name = func->getNameAsString();
      clang::QualType returnType = E->getCallReturnType(*_C);

      for (std::string &removedName : *_ToRemove) {
        if (name != removedName)
          continue;

        std::optional<std::string> verifierTypeName = verifierSuffixForType(returnType);
        if (!verifierTypeName) {
          std::cout << "Warning: Call to removed function '" << name << "' has unsupported return type "
            << returnType.getAsString() << ", skipping replacement.\n";
          continue;
        }

        std::string replacement = "__VERIFIER_nondet_" + *verifierTypeName + "()";
        if (E->getSourceRange().isValid()) {
          _Rewriter.ReplaceText(E->getSourceRange(), replacement);
          _NeededTypes->insert(*verifierTypeName);
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveVisitor>::VisitCallExpr(E);
}

bool RemoveVisitor::shouldTraversePostOrder() { return false; }
