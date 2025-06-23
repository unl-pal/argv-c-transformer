#include "include/RemoveVisitor.hpp"

#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

RemoveFuncVisitor::RemoveFuncVisitor(clang::ASTContext       *C,
                                     clang::Rewriter rewriter,
                                     std::vector<std::string> toRemove)
    : _C(C), _mgr(_C->getSourceManager()), _Rewriter(rewriter), _toRemove(toRemove) {}

bool RemoveFuncVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if(!D) return false;
  if (_mgr.isInMainFile(D->getLocation())) {
    for (std::string& name : _toRemove) {
      if (name == D->getNameAsString()) {
        llvm::outs() << name << "\n";
        if (clang::RawComment *rawComment = _C->getRawCommentForDeclNoCache(D)) {
          _Rewriter.ReplaceText(rawComment->getSourceRange(), "");
        }
        clang::SourceRange range = D->getSourceRange();
        if (D->getStorageClass() == clang::SC_Extern) {
          range = clang::SourceRange(D->getOuterLocStart(), D->getEndLoc().getLocWithOffset(1));
        }
        _Rewriter.ReplaceText(range, "// === Removed Undesired Function ===\n");
        // TODO what if only one node can be removed per run?
        clang::Decl *decl = clang::dyn_cast<clang::Decl>(D);
        if (decl) {
          
        }
        _C->getTranslationUnitDecl()->removeDecl(D);
        return false;
        // return true;
      }
    }
  }
    return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitFunctionDecl(D);
}

// TODO CallExpr can be used to also ID the return type for replacing with the
// correct versions of the verifier
bool RemoveFuncVisitor::VisitCallExpr(clang::CallExpr *E) {
  /*if (E->EvaluateAsBooleanCondition(bool &Result, const ASTContext &Ctx)) {*/
  if (!E) return false;
  if (_mgr.isInMainFile(E->getExprLoc())) {
    if (E->getType()->isFunctionType()) {
      auto thing = E->getCallReturnType(*_C);
      if (thing->isCharType()) {
      } else if (thing->isIntegerType()) {
      } else if (thing->isFloatingType()) {
      } else if (thing->isStructureType()) {
      } else if (thing->isObjectPointerType()) {
      } else if (thing->isArrayType()) {
      } else if (thing->isNullPtrType()) {
      // } else if (thing->isDoubleType()) {
      } else {
      }
    }
  }
  return clang::RecursiveASTVisitor<RemoveFuncVisitor>::VisitCallExpr(E);
}
