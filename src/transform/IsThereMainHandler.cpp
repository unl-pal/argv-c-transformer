#include "IsThereMainHandler.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

IsThereMainHandler::IsThereMainHandler(clang::Rewriter &rewriter) : _hasMain(false), _Rewriter(rewriter) {}

void IsThereMainHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &results) {
  clang::ASTContext *context = results.Context;
  clang::SourceManager *mgr = results.SourceManager;
  clang::TranslationUnitDecl *TD = context->getTranslationUnitDecl();
  // std::set<clang::FunctionDecl*> normalFunctions;
  std::set<clang::DeclarationName> normalFunctions;
  if (const clang::FunctionDecl *decl = results.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
    if (mgr->isInMainFile(decl->getLocation())) {
      // normalFunctions.emplace(TD->lookup(decl->getDeclName()).find_first<clang::FunctionDecl>());
      normalFunctions.emplace(decl->getDeclName());
      decl->dumpColor();
    }
    if (decl->isMain()) {
      llvm::outs() << "FOUND MAIN!!\n";
      this->_hasMain = true;
    }
  }

  if (!HasMain()) {
    llvm::outs() << "Building Main\n";
  //   // clang::SourceLocation loc = TD->decls_end()->getPreviousDecl()->getLocation();
    clang::SourceLocation loc = mgr->getLocForEndOfFile(mgr->getMainFileID());
  //   clang::IdentifierInfo *funcName = &context->Idents.get("main");
  //   clang::DeclarationName declName(funcName);
  //   clang::FunctionProtoType::ExtProtoInfo epi;
  //   clang::QualType funcQualType = context->IntTy;
  //
  //   clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
  //     *context,
  //     TD,
  //     loc,
  //     loc,
  //     declName,
  //     funcQualType,
  //     nullptr,
  //     clang::SC_Extern
  //   );
  //   newFunction->setReferenced();
  //   newFunction->setIsUsed();
  //   newFunction->setWillHaveBody(true);
  //   TD->addDecl(newFunction);
  //   _Rewriter.InsertTextAfter(loc, newFunction->getReturnType().getAsString() + " " + newFunction->getNameAsString() + "() {\n");
  //   clang::SourceRange range;
  //   loc = newFunction->getLocation();
  //   range.setBegin(loc);
  //
  //   clang::Stmt *body = newFunction->getBody();
  //   for (clang::FunctionDecl *func : normalFunctions) {
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
  //       clang::DeclRefExpr *call = clang::DeclRefExpr::Create(*context, func->getQualifierLoc(), loc, func, false, loc, parm->getType(), clang::ExprValueKind::VK_LValue);
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
  //   _Rewriter.InsertTextAfter(loc, "}");
  //   // loc = loc.getLocWithOffset(tempName.size());
  //
  //   // return clang::RecursiveASTVisitor<AddMainVisitor>::VisitTranslationUnitDecl(D);
  //   // return true;
  //   // for (clang::FunctionDecl &func : normalFunctions) {
  //   //
  //   // }
  }
}

bool IsThereMainHandler::HasMain() {
  return this->_hasMain;
}
