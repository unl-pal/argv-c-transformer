#include "IsThereMainHandler.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>

IsThereMainHandler::IsThereMainHandler(std::set<clang::DeclRefExpr*> &callsToMake) : _hasMain(false), _CallsToMake(callsToMake) {}

void IsThereMainHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  if (results.Nodes.getNodeAs<clang::FunctionDecl>("main")) {
    llvm::outs() << "FOUND MAIN!!\n";
    _hasMain = true;
  } else {
    if (const clang::FunctionDecl *func = results.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
      clang::SourceManager *mgr = results.SourceManager;

      // if (clang::FunctionType::MacroQualified == func->getType()->getTypeClass()) {
      //   llvm::outs() << "This is a Macro thingy\n";
      // }

      clang::ASTContext *context = results.Context;
      if (mgr->isInMainFile(func->getLocation())) {
        llvm::outs() << "FOUND " << func->getNameAsString() << "!!\n";
        if (!func->isReferenced() && !func->isUsed()) {
          clang::NestedNameSpecifierLoc qualLoc;
          if (func->getQualifierLoc()) {
            qualLoc = func->getQualifierLoc();
          } else {
            qualLoc = clang::NestedNameSpecifierLoc();
          }

          clang::FunctionProtoType::ExtProtoInfo epi;

          clang::QualType funcQualType = context->getFunctionType(
            func->getType(),
            clang::ArrayRef<clang::QualType>({context->VoidTy}),
            epi
          );

          clang::DeclRefExpr *call = clang::DeclRefExpr::Create(
            *context,
            qualLoc,
            clang::SourceLocation(),
            (clang::FunctionDecl*)(func),
            false,
            func->getLocation(),
            funcQualType,
            clang::ExprValueKind::VK_LValue
          );

          _CallsToMake.emplace(call);
        }
      }
    }
  }
}

bool IsThereMainHandler::HasMain() {
  return _hasMain;
}
