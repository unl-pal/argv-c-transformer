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
  // If the current node being handled is tagged as main set the has main to true
  if (results.Nodes.getNodeAs<clang::FunctionDecl>("main")) {
    llvm::outs() << "FOUND MAIN!!\n";
    _hasMain = true;
    // If the function is tagged as a standard function check to see if it is used and or referenced such that it is accessible from main
  } else {
    if (const clang::FunctionDecl *func = results.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
      clang::SourceManager *mgr = results.SourceManager;

      clang::ASTContext *context = results.Context;
      // verify function is in the main file and not an expansion or header
      if (mgr->isInMainFile(func->getLocation())) {
        llvm::outs() << "FOUND " << func->getNameAsString() << "!!\n";
        if (!func->isReferenced() && !func->isUsed()) {
          clang::NestedNameSpecifierLoc qualLoc;
          if (func->getQualifierLoc()) {
            qualLoc = func->getQualifierLoc();
          } else {
            qualLoc = clang::NestedNameSpecifierLoc();
          }

          // Use default values to create a necessary call with all meta data
          clang::FunctionProtoType::ExtProtoInfo epi;

          // qualtype contains the function return type as well as other meta data
          clang::QualType funcQualType = context->getFunctionType(
            func->getType(),
            clang::ArrayRef<clang::QualType>({context->VoidTy}),
            epi
          );

          // Create the function call itself using the function decl found
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

          // Save the call to the accessible list for later usage
          _CallsToMake.emplace(call);
        }
      }
    }
  }
}

bool IsThereMainHandler::HasMain() {
  return _hasMain;
}
