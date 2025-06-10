#include "include/RemoveNodeHandler.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>

void
RemoveNodeHandler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const clang::FunctionDecl *func =
    Result.Nodes.getNodeAs<clang::FunctionDecl>("node");
  clang::ASTContext &context = func->getASTContext();
  clang::TranslationUnitDecl *td = context.getTranslationUnitDecl();
  td->removeDecl((clang::FunctionDecl*)(func));
  // context.DeclMustBeEmitted(func);

  clang::SourceManager &mgr = context.getSourceManager();
  // clang::FunctionDecl *tempFunc;
  // clang::FunctionDecl *tempFunc = func->Create(context, const issue func->getDeclContext(), func->getSourceRange().getBegin(), func->getSourceRange().getEnd(), func->getName(), func->getReturnType(), func->getTypeSourceInfo(), clang::SC_Auto);
  // td->removeDecl();
  // func->getDeclContext()->
  // clang::DeclContext *td = func->getParent();
  // td->removeDecl(func);
}

void RemoveNodeHandler::onEndOfTranslationUnit() {

}
