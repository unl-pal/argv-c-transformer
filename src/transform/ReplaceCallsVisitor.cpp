#include "include/ReplaceCallsVisitor.hpp"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes)
    : _C(C), _NeededTypes(neededTypes) {
  llvm::outs() << "Created the ReplaceDeadCallsVisitor\n";
};

bool ReplaceDeadCallsVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  llvm::outs() << "Handling ReplaceDeadCallsVisitor" << "\n";
  // return ReplaceDeadCallsVisitor::VisitDecl(D);
  return true;
  // clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitTranslationUnitDecl(C.getTranslationUnitDecl());
}

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
}

// Call Expr is the parent of the function decl ref and the args used
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
      if (((func->isImplicit() || !func->isDefined())) && !func->isInlineDefinitionExternallyVisible()) {
        clang::QualType funcType = func->getReturnType();
        std::string myType = funcType.getAsString();
        std::replace(myType.begin(), myType.end(), ' ', '_');
        clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_" + myType);
        clang::DeclarationName newName(newInfo);
        func->setDeclName(newName);
        llvm::outs() << newName.getAsString() << "\n";
        _NeededTypes->emplace(funcType);
        // E->getReferencedDeclOfCallee()
        E->shrinkNumArgs(0);
        E->setCallee(clang::CallExpr::Create(*_C, E, {}, funcType, clang::ExprValueKind::VK_LValue, E->getRParenLoc(), clang::FPOptionsOverride::FPOptionsOverride::getFromOpaqueInt(clang::StorageClass::SC_Extern)));
        // clang::CallExpr::Create(const ASTContext &Ctx, Expr *Fn, ArrayRef<Expr *> Args, QualType Ty, ExprValueKind VK, SourceLocation RParenLoc, FPOptionsOverride FPFeatures)
      }
    }
  }
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
}
