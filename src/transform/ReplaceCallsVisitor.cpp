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
#include <cstddef>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsVisitor::ReplaceDeadCallsVisitor(clang::ASTContext *C, std::set<clang::QualType> *neededTypes)
    : _C(C), _NeededTypes(neededTypes) {
  llvm::outs() << "Created the ReplaceDeadCallsVisitor\n";
};

bool ReplaceDeadCallsVisitor::VisitTranslationUnit(clang::TranslationUnitDecl *D) {
  llvm::outs() << "Handling ReplaceDeadCallsVisitor" << "\n";
  // D->dumpDeclContext();
  // return ReplaceDeadCallsVisitor::VisitDecl(D);
  // return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitTranslationUnitDecl(D);
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseDecl(D);
  // return true;
}

bool ReplaceDeadCallsVisitor::VisitDecl(clang::Decl *D) {
  // llvm::outs() << "Found a DECL\n";
  // return true;
  return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitDecl(D);
  // return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseDecl(D);
}

// Call Expr is the parent of the function decl ref and the args used
bool ReplaceDeadCallsVisitor::VisitCallExpr(clang::CallExpr *E) {
  llvm::outs() << "Found a Call\n";
  E->dumpColor();
  if (_C->getSourceManager().isInMainFile(E->getExprLoc())) {
    // if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
    clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction();
    if (func != nullptr) {
      llvm::outs() << "Found a Function\n";
      func->dumpColor();
      // if (!E->getAsBuiltinConstantDeclRef(*_C)->declarationReplaces(E->getCallee()->getReferencedDeclOfCallee()->getAsFunction())) {
      auto refDecl = E->getDirectCallee();
      if (refDecl) {
        if (refDecl->declarationReplaces(func)) {
          llvm::outs() << "Made it to the replaces part\n";
        // }
      // }
      // if (!E->getAsBuiltinConstantDeclRef(*_C)->declarationReplaces(func)) {
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
        // E->getReferencedDeclOfCallee()->isFirstDecl();
        // E->getReferencedDeclOfCallee()->getAsFunction()->setDeclName(newName);
        // E->getCalleeDecl();
        // E->getDirectCallee();
          E->shrinkNumArgs(0);
          E->dumpColor();
        // E->getAsBuiltinConstantDeclRef(*_C)->declarationReplaces(E->getCallee()->getReferencedDeclOfCallee()->getAsFunction());
        // E->setCallee(E);
        // auto value = clang::ExprValueKind::VK_LValue;
        // auto loc = E->getRParenLoc();
        // auto options = clang::FPOptionsOverride::getFromOpaqueInt(clang::StorageClass::SC_Auto);
        // auto options = clang::FPOptionsOverride::getFromOpaqueInt(clang::StorageClass::SC_Extern);
        // llvm::outs() << "Setting the new callee" << "\n";
        // E->setCallee(clang::CallExpr::Create(*_C, E, {}, funcType, value, loc, options));
        // clang::CallExpr::Create(const ASTContext &Ctx, Expr *Fn, ArrayRef<Expr *> Args, QualType Ty, ExprValueKind VK, SourceLocation RParenLoc, FPOptionsOverride FPFeatures)
        // llvm::outs() << func->getNameAsString() << "\n";
        }
      }
      // if (((func->isImplicit() || !func->isDefined())) && !func->isInlineDefinitionExternallyVisible()) {
        // clang::QualType funcType = func->getReturnType();
        // std::string myType = funcType.getAsString();
        // std::replace(myType.begin(), myType.end(), ' ', '_');
        // clang::IdentifierInfo *newInfo = &_C->Idents.get("__VERIFIER_nondet_" + myType);
        // clang::DeclarationName newName(newInfo);
        // llvm::outs() << "Setting the new name" << "\n";
        // // func->setDeclName(newName);
        // // llvm::outs() << newName.getAsString() << "\n";
        // _NeededTypes->emplace(funcType);
        // // E->getReferencedDeclOfCallee()->isFirstDecl();
        // // E->getReferencedDeclOfCallee()->getAsFunction()->setDeclName(newName);
        // // E->getCalleeDecl();
        // // E->getDirectCallee();
        // E->shrinkNumArgs(0);
        // // E->setCallee(E);
        // auto value = clang::ExprValueKind::VK_LValue;
        // auto loc = E->getRParenLoc();
        // // auto options = clang::FPOptionsOverride::getFromOpaqueInt(clang::StorageClass::SC_Auto);
        // auto options = clang::FPOptionsOverride::getFromOpaqueInt(clang::StorageClass::SC_Extern);
        // llvm::outs() << "Setting the new callee" << "\n";
        // E->setCallee(clang::CallExpr::Create(*_C, E, {}, funcType, value, loc, options));
        // // clang::CallExpr::Create(const ASTContext &Ctx, Expr *Fn, ArrayRef<Expr *> Args, QualType Ty, ExprValueKind VK, SourceLocation RParenLoc, FPOptionsOverride FPFeatures)
        // llvm::outs() << func->getNameAsString() << "\n";
      // }
    // } else {
      // llvm::outs() << "Callee" << "\n";
      // E->getCallee()->dumpColor();
      // llvm::outs() << "Referenced Decl of Callee" << "\n";
      // E->getCallee()->getReferencedDeclOfCallee()->dumpColor();
      // llvm::outs() << "Function Dump" << "\n";
      // func->dumpColor();
      // // clang::DeclRefExpr *dre = E->getAsBuiltinConstantDeclRef(*_C)->declarationReplaces(E->getCallee()->getReferencedDeclOfCallee()->getAsFunction());
      // llvm::outs() << "Declaration Replaces" << "\n";
      // E->getAsBuiltinConstantDeclRef(*_C)->declarationReplaces(E->getCallee()->getReferencedDeclOfCallee()->getAsFunction());
    }
  }
  // return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::TraverseStmt(E);
  // return clang::RecursiveASTVisitor<ReplaceDeadCallsVisitor>::VisitCallExpr(E);
  return true;
}

bool ReplaceDeadCallsVisitor::shouldTraversePostOrder() {
  return true;
}
