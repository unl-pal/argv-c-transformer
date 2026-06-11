#include "include/ReplaceDeadCallsVisitor.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <optional>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(
    clang::ASTContext *C, std::shared_ptr<std::set<std::string>> neededSuffixes,
    clang::Rewriter &rewriter)
    : _C(C), _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {};

bool ReplaceDeadCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseDecl(D);
}

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Find the Call Expressions for the removed functions and update them
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    if (clang::Decl *calleeDecl = E->getCalleeDecl()) {
      if (clang::FunctionDecl *func = calleeDecl->getAsFunction()) {
        // A "dead" call targets a function reduced to a bare declaration (no
        // body) by the filter step, or an implicit declaration Clang
        // synthesized for a call with no prototype.
        if (_C->getSourceManager().isInMainFile(func->getLocation()) &&
            (!func->isDefined() || func->isImplicit()) &&
            func->getStorageClass() != clang::SC_Extern) {
          clang::QualType returnType = E->getCallReturnType(*_C);
          // Unsupported return types (pointers, structs, ...) have no
          // __VERIFIER_nondet_* equivalent; leave the call as-is.
          if (std::optional<std::string> suffix = verifierSuffixForType(returnType)) {
            _Rewriter.ReplaceText(E->getSourceRange(), "__VERIFIER_nondet_" + *suffix + "()");
            _NeededSuffixes->emplace(*suffix);
          }
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
}

bool ReplaceDeadCallsVisitor::shouldTraversePostOrder() {
  return true;
  // return false;
}
