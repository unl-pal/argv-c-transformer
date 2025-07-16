#include "AddMainVisitor.hpp"
#include "AllFunctionsToCallHandler.hpp"
#include "IsThereMainConsumer.hpp"
#include "IsThereMainHandler.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

IsThereMainConsumer::IsThereMainConsumer(clang::Rewriter &rewriter) : _Rewriter(rewriter) 
{
}

using namespace clang::ast_matchers;
void IsThereMainConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Looking for Main Function\n";
  clang::ast_matchers::MatchFinder MatchFinder;
  // IsThereMainHandler Handler(_Rewriter);
  // std::set<clang::CallExpr*> callsToMake;
  std::set<clang::DeclRefExpr*> callsToMake;
  IsThereMainHandler Handler(callsToMake);
  // TODO REMOVE ME
  if (Context.Idents.find("main") != Context.Idents.end()) {
    auto lookup = Context.Idents.find("main");
    llvm::outs() << "Found Lookup\n";
    // if (lookup) {
      // llvm::outs() << "Found main name\n";
      if (clang::DeclarationName main = lookup->second) {
        llvm::outs() << "Found main info\n";
        if (clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl()) {
          llvm::outs() << "Found TD\n";
          if (clang::FunctionDecl *func = TD->lookup(main).find_first<clang::FunctionDecl>()) {
            llvm::outs() << "Found main\n";
            func->dumpColor();
          }
        }
      }
    // }
  }
  // context.getTranslationUnitDecl()->dumpColor();
  // MatchFinder.addMatcher(decl().bind("something"), &Handler);
  MatchFinder.addMatcher(functionDecl(unless(isMain())).bind("functions"), &Handler);
  MatchFinder.addMatcher(functionDecl(isMain()).bind("main"), &Handler);
  // llvm::outs() << "Add Ze Mache\n";
  MatchFinder.matchAST(Context);
  // llvm::outs() << "Run Ze Mache\n";
  // if (!Handler.HasMain()) {
    // llvm::outs() << "Not Haz Ze Maene\n";
  //   clang::ast_matchers::MatchFinder FunctionsMatchFinder;
  //   AllFunctionsToCallHandler FunctionsHandler;
  //   FunctionsMatchFinder.addMatcher(functionDecl().bind("functions"),
  //   &FunctionsHandler); FunctionsMatchFinder.matchAST(Context); llvm::outs()
  //   << "Gotz Nameze\n"; if (FunctionsHandler.GetNames().size()) {
  //     AddMainVisitor Visitor(&Context, FunctionsHandler.GetNames(),
  //     _Rewriter);
  //     Visitor.VisitTranslationUnitDecl(Context.getTranslationUnitDecl());
  //   }
  // }
  //
  // TODO - This is getting stupider by the minute
  // need to verify that a verifier exists or create and add one if there is not
  // in order for the nondet calls of the args/parameters on referenced
  // functions in main are there and available while all the AST info on types
  // and what not exists and is easily available
  //
  // This is a bigger headache than planned and is leading to a ver conplicated
  // consumer
  //
  // This needs to be broken out into more functions and tasks to make this
  // easier to understand and maintain if possible
  //
  for (clang::DeclRefExpr *call : callsToMake) {
    clang::ArrayRef<clang::Expr*> args;
    for (clang::ParmVarDecl *var : call->getFoundDecl()->getAsFunction()->parameters()) {
      clang::IdentifierInfo *info = &Context.Idents.get("__VERIFIER_non_det_" + var->getType().getAsString());
      clang::DeclarationName name(info);
      clang::Decl *tempD = Context.getTranslationUnitDecl();
      clang::FunctionDecl *verifier = Context.getTranslationUnitDecl()->lookup(name).find_first<clang::FunctionDecl>() ? : clang::FunctionDecl::Create(Context, tempD->getDeclContext(), tempD->getBeginLoc(), tempD->getEndLoc(), name, var->getType(), nullptr, clang::SC_Extern);
      clang::DeclRefExpr *verifierCall = clang::DeclRefExpr::Create(Context, verifier->getQualifierLoc(), tempD->getLocation(), verifier, false, tempD->getLocation(), var->getType(), clang::ExprValueKind::VK_PRValue);
      clang::CallExpr *tempExpr = clang::CallExpr::Create(Context, verifierCall, {}, var->getType(), clang::ExprValueKind::VK_LValue, var->getLocation(), clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
      args.vec().push_back(tempExpr);
    }
    clang::CallExpr *callExpr = clang::CallExpr::Create(Context, call, args, call->getFoundDecl()->getAsFunction()->getType(), clang::ExprValueKind::VK_LValue, call->getLocation(), clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
  }
}
