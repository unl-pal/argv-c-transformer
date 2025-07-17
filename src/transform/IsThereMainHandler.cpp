#include "IsThereMainHandler.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

// IsThereMainHandler::IsThereMainHandler(clang::Rewriter &rewriter) : _hasMain(false), _Rewriter(rewriter) {}
// IsThereMainHandler::IsThereMainHandler(std::set<clang::CallExpr*> &callsToMake) : _hasMain(false), _CallsToMake(callsToMake) {}
IsThereMainHandler::IsThereMainHandler(std::set<clang::DeclRefExpr*> &callsToMake) : _hasMain(false), _CallsToMake(callsToMake) {}

void IsThereMainHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  if (const clang::FunctionDecl *main = results.Nodes.getNodeAs<clang::FunctionDecl>("main")) {
    llvm::outs() << "FOUND MAIN!!\n";
    main->dumpColor();
    _hasMain = true;
  } else {
    // Currently always creates the call and tracking for non referenced or used
    // functions so they can be added to the main function for reachability if
    // desired, just in case

    // There may be situations that the functions are not being called due to
    // another function that was supposed to call it being removed for which we
    // may need to add it to the main
    if (const clang::FunctionDecl *func = results.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
      clang::SourceManager *mgr = results.SourceManager;
      clang::ASTContext *context = results.Context;
      clang::SourceLocation loc = mgr->getLocForEndOfFile(mgr->getMainFileID());
      if (mgr->isInMainFile(func->getLocation())) {
        llvm::outs() << "FOUND " << func->getNameAsString() << "!!\n";
        if (!func->isReferenced() && !func->isUsed()) {
          func->dump();
          // clang::QualType someType;
          // clang::ArrayRef<clang::Expr*> args;
          // for (clang::ParmVarDecl *parm : func->parameters()) {
          //   clang::QualType tempType = parm->getType();
          //   // clang::DeclRefExpr *call = clang::DeclRefExpr::Create(*context, tempType, loc, (clang::FunctionDecl*)(func), false, loc, someType, clang::ExprValueKind::VK_LValue);
          //   // args.vec().push_back()
          // }
          // if (func->parameters().size()) {
          //   clang::ParmVarDecl *parm = func->parameters().front();
          //   someType = parm->getType();
          // } else {
          //   someType = context->VoidTy;
          // }
          clang::DeclRefExpr *call = clang::DeclRefExpr::Create(*context, func->getQualifierLoc(), clang::SourceLocation(), (clang::FunctionDecl*)(func), false, loc, func->getType(), clang::ExprValueKind::VK_LValue, (clang::NamedDecl*)(func)); // clang::CallExpr *callExpr = clang::CallExpr::Create(const ASTContext &Ctx, Expr *Fn, ArrayRef<Expr *> Args, QualType Ty, ExprValueKind VK, SourceLocation RParenLoc, FPOptionsOverride FPFeatures)
          // clang::CallExpr *callExpr = clang::CallExpr::Create(*context, call, args, call->getFoundDecl()->getAsFunction()->getType(), clang::ExprValueKind::VK_LValue, loc, clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
          _CallsToMake.emplace(call);
        }
      }
    }
  }
}

    // clang::ASTContext *context = results.Context;
    // clang::TranslationUnitDecl *TD = context->getTranslationUnitDecl();
    // llvm::outs() << "Building Main\n";
    // clang::SourceLocation loc = mgr->getLocForEndOfFile(mgr->getMainFileID());
    // clang::IdentifierInfo *funcName = &context->Idents.get("main");
    // clang::DeclarationName declName(funcName);
    // clang::FunctionProtoType::ExtProtoInfo epi;
    // clang::QualType funcQualType = context->IntTy;
    //
    // clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
    //   *context,
    //   TD,
    //   loc,
    //   loc,
    //   declName,
    //   funcQualType,
    //   nullptr,
    //   clang::SC_Extern
    // );
    // newFunction->setReferenced();
    // newFunction->setIsUsed();
    // newFunction->setWillHaveBody(true);
    // TD->addDecl(newFunction);
    // _Rewriter.InsertTextAfter(loc, newFunction->getReturnType().getAsString() + " " + newFunction->getNameAsString() + "() {\n");
    // clang::SourceRange range;
    // loc = newFunction->getLocation();
    // range.setBegin(loc);
    //
    // clang::Stmt *body = newFunction->getBody();
    // for (auto node : results.Nodes.getMap()) {
    //   if (const clang::FunctionDecl *func = node.second.get<clang::FunctionDecl>()) {
    //
    //     std::string name = func->getNameAsString();
    //     clang::IdentifierInfo *funcCallName = &context->Idents.get(name);
    //     clang::DeclarationName funcCallNameString(funcCallName);
    //     // clang::FunctionDecl *func = TD->lookup(funcCallNameString).find_first<clang::FunctionDecl>();
    //     _Rewriter.InsertTextAfter(loc, func->getNameAsString() + "(");
    //     int i = 0;
    //     std::string verifierName = "__Verifier_non_det_";
    //     newFunction->getBody();
    //     for (clang::ParmVarDecl *parm : func->parameters()) {
    //       // clang::ValueDecl *tempDecl = func;
    //       clang::DeclRefExpr *call = clang::DeclRefExpr::Create(*context, func->getQualifierLoc(), loc, (clang::FunctionDecl*)(func), false, loc, parm->getType(), clang::ExprValueKind::VK_LValue);
    //       body->addStmtClass(call->getStmtClass());
    //       std::string comma;
    //       i++? comma = ", " : comma = "";
    //       std::string tempName = comma + verifierName + std::string(parm->getType()->getTypeClassName());
    //       _Rewriter.InsertTextAfter(loc, tempName);
    //       loc = loc.getLocWithOffset(tempName.size());
    //     }
    //     _Rewriter.InsertText(loc, ");\n");
    //     // loc.isPairOfFileLocations(SourceLocation Start, SourceLocation End)
    //   }
    // }
    // _Rewriter.InsertTextAfter(loc, "}");
    // // loc = loc.getLocWithOffset(tempName.size());
    //
    // // return clang::RecursiveASTVisitor<AddMainVisitor>::VisitTranslationUnitDecl(D);
    // // return true;
    // // for (clang::FunctionDecl &func : normalFunctions) {
    // //
    // // }
//   }
// }

bool IsThereMainHandler::HasMain() {
  return _hasMain;
}
