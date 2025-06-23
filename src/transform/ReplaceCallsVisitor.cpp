#include "include/ReplaceCallsVisitor.hpp"

#include <algorithm>
#include <clang/AST/Expr.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes)
    : _C(C), _NeededTypes(neededTypes) {
  llvm::outs() << "Created the ReplaceDeadCallsVisitor\n";
};

bool ReplaceDeadCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  llvm::outs() << "Handling ReplaceDeadCallsVisitor" << "\n";
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseDecl(D);
}

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Find the Call Expressions for the removed functions and update them
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  llvm::outs() << "Found a Call\n";
  E->dumpColor();
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction();
    if (func != nullptr) {
      llvm::outs() << "Found a Function\n";
      func->dumpColor();
      auto refDecl = E->getDirectCallee();
      if (refDecl) {
        if (refDecl->declarationReplaces(func)) {
          llvm::outs() << "Made it to the replaces part\n";
          llvm::outs() << "Passed the declarationReplaces" << "\n";
          clang::QualType funcType = func->getReturnType();
          std::string myType = funcType.getAsString();
          std::replace(myType.begin(), myType.end(), ' ', '_');
          clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_" + myType);
          clang::DeclarationName newName(newInfo);
          llvm::outs() << "Setting the new name" << "\n";
          func->setDeclName(newName);
          llvm::outs() << newName.getAsString() << "\n";
          _NeededTypes->insert(funcType);
          E->shrinkNumArgs(0);
          E->dumpColor();
        }
      }
    }
  }
  return true;
}

bool ReplaceDeadCallsVisitor::shouldTraversePostOrder() {
  return true;
}
