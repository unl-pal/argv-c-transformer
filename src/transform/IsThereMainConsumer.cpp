#include "AddMainVisitor.hpp"
#include "AllFunctionsToCallHandler.hpp"
#include "IsThereMainConsumer.hpp"
#include "IsThereMainHandler.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/AttrKinds.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Sema/DeclSpec.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

IsThereMainConsumer::IsThereMainConsumer(clang::Rewriter &rewriter) : _Rewriter(rewriter) 
{
}

using namespace clang::ast_matchers;
void IsThereMainConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Looking for Main Function\n";
  clang::ast_matchers::MatchFinder MatchFinder;
  std::set<clang::DeclRefExpr*> callsToMake;
  IsThereMainHandler Handler(callsToMake);
  // MatchFinder.addMatcher(clang::ast_matchers::translationUnitDecl(clang::ast_matchers::traverse(clang::TraversalKind::TK_IgnoreUnlessSpelledInSource, clang::ast_matchers::decl().bind("allDecls"))), &Handler); // Currently not in use...
  MatchFinder.addMatcher(functionDecl(isMain()).bind("main"), &Handler);
  // MatchFinder.addMatcher(functionDecl(unless(anyOf(isMain(), isExpansionInMainFile(), isExpandedFromMacro("*")))).bind("functions"), &Handler);
  MatchFinder.addMatcher(functionDecl(unless(isMain())).bind("functions"), &Handler);
  MatchFinder.matchAST(Context);
  clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl();
  clang::SourceManager &mgr = Context.getSourceManager();
  std::vector<clang::Stmt*> stmts;
  for (clang::DeclRefExpr *call : callsToMake) {
    std::vector<clang::Expr*> tempArgs({});
    std::vector<clang::ParmVarDecl*> vars({});
    if (clang::NamedDecl *namedDecl = call->getFoundDecl()) {
      if (clang::FunctionDecl *func = namedDecl->getAsFunction()) {
        if (!func->param_empty()) {
          vars = func->parameters().vec();
        }
      }
    }
    llvm::outs() << "Calls to Make: " << callsToMake.size() << "\n";
    for (clang::ParmVarDecl *var : vars) {
      clang::QualType varType = var->getType();
      if (varType->isAnyPointerType()) {
        break;
      }
      // if (!varType->isBuiltinType() &&
      //   !varType->isBooleanType() &&
      //   !varType->isCharType() &&
      //   !varType->isVoidType()) {
      //   // break;
      //   // varType = Context.VoidPtrTy;
      // } // Attempting to replace custom return values and parameters with void*
      varType->dump();
      std::string varTypeString = varType->isBooleanType() ? "bool" : varType.getAsString();
      std::replace(varTypeString.begin(), varTypeString.end(), ' ', '_');
      std::replace(varTypeString.begin(), varTypeString.end(), '*', '\0');
      if (!std::strcmp(&varTypeString.at(varTypeString.size() - 1), "_")) {
        varTypeString.pop_back();
      }
      clang::IdentifierInfo *info = &Context.Idents.get("__VERIFIER_nondet_" + varTypeString);
      clang::DeclarationName name(info);
      clang::DeclContextLookupResult result = TD->lookup(name);
      clang::FunctionDecl *verifier;
      llvm::outs() << "PreProto\n";
      if (result.empty()) {
        clang::SourceLocation insertFirst;
        for (clang::Decl *decl : TD->decls()) {
          insertFirst = decl->getLocation();
          if (!mgr.isMacroArgExpansion(insertFirst)) {
            if (mgr.isInMainFile(insertFirst)) {
              int firstLine = mgr.getSpellingLineNumber(insertFirst);
              insertFirst = mgr.translateLineCol(mgr.getMainFileID(), firstLine, 1);
              break;
            }
          }
        }
        clang::FunctionProtoType::ExtProtoInfo varEpi;
        clang::QualType funcQualType = Context.getFunctionType(Context.getCanonicalParamType(varType), {Context.VoidTy}, varEpi);
        verifier = clang::FunctionDecl::Create(Context, TD, insertFirst, insertFirst.getLocWithOffset(1), name, funcQualType, Context.CreateTypeSourceInfo(varType), clang::SC_Extern);

        verifier->setLocation(insertFirst);
        verifier->setReferenced();
        verifier->setIsUsed();

        clang::ParmVarDecl *vParm = clang::ParmVarDecl::Create(Context, verifier->getDeclContext(), verifier->getLocation(), verifier->getLocation(), nullptr, Context.VoidTy, nullptr, clang::SC_None, nullptr);
        verifier->setParams({vParm});
        vParm->setOwningFunction(verifier);
        verifier->addDecl(vParm);
        verifier->addDecl(clang::ParmVarDecl::Create(Context, verifier->getDeclContext(), verifier->getInnerLocStart(), verifier->getLocation(), &Context.Idents.get(""), Context.VoidTy, Context.CreateTypeSourceInfo(Context.VoidTy), clang::SC_None, nullptr));

        TD->addDecl(verifier);
        if (verifier) {
          llvm::outs() << "Well at least there is that\n";
          // verifier->dumpColor();
        }
        if (insertFirst.isValid()) {
          std::string verifierString = "";
          llvm::raw_string_ostream verifierStream(verifierString);
          llvm::outs() << "PreProto\n";
          verifier->dumpColor();
          verifier->getAsFunction()->print(verifierStream, 0, true);
          llvm::outs() << "PreProto\n";
          _Rewriter.InsertTextBefore(insertFirst, verifierString + ";\n");
        }
      } else {
        verifier = result.find_first<clang::FunctionDecl>();
      }

      clang::DeclRefExpr *verifierCall = clang::DeclRefExpr::Create(Context, verifier->getQualifierLoc(), clang::SourceLocation(), verifier, false, verifier->getLocation(), var->getType(), clang::ExprValueKind::VK_LValue, verifier, nullptr);
      clang::CallExpr *tempExpr = clang::CallExpr::Create(Context, verifierCall, {}, var->getType(), clang::ExprValueKind::VK_LValue, var->getLocation(), clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
      tempExpr->dumpColor();
      tempArgs.push_back(tempExpr);
    }
    if (tempArgs.size() < vars.size()) {
      break;
    }
    llvm::outs() << "Args Size: " << tempArgs.size() << "\n";
    clang::CallExpr *callExpr = clang::CallExpr::Create(Context, call, tempArgs, call->getType(), clang::ExprValueKind::VK_LValue, call->getLocation(), clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
    callExpr->dumpColor();
    stmts.emplace_back(callExpr);
  }

  clang::FunctionDecl* mainFunc;
  if (!Handler.HasMain()) {
    llvm::outs() << "Building Main\n";
    clang::SourceLocation loc = mgr.getLocForEndOfFile(mgr.getMainFileID());
    clang::IdentifierInfo *funcName = &Context.Idents.get("main");
    clang::DeclarationName declName(funcName);
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType = Context.getFunctionType(Context.IntTy, {Context.VoidTy}, epi);

    mainFunc = clang::FunctionDecl::Create(
      Context,
      TD,
      loc,
      loc.getLocWithOffset(1),
      declName,
      funcQualType,
      Context.CreateTypeSourceInfo(Context.IntTy),
      clang::SC_None//,
    );
    mainFunc->setWillHaveBody(true);
    clang::ParmVarDecl *parm = clang::ParmVarDecl::Create(Context, mainFunc->getDeclContext(), mainFunc->getInnerLocStart(), mainFunc->getLocation(), nullptr, Context.VoidTy, nullptr, clang::SC_None, nullptr);
    std::vector<clang::ParmVarDecl*> parms = {parm};
    llvm::outs() << "Parameters size: " << parms.size() << "\n";
    mainFunc->setParams({parm});
    mainFunc->addDecl(parm);
    mainFunc->addDecl(clang::ParmVarDecl::Create(Context, mainFunc->getDeclContext(), mainFunc->getInnerLocStart(), mainFunc->getLocation(), &Context.Idents.get(""), Context.VoidTy, Context.CreateTypeSourceInfo(Context.VoidTy), clang::SC_None, nullptr));
    TD->addDecl(mainFunc);
    clang::ReturnStmt *returnStmt = clang::ReturnStmt::Create(Context, mainFunc->getLocation(), clang::IntegerLiteral::Create(Context, llvm::APInt::doubleToBits(0), Context.IntTy, mainFunc->getLocation()), clang::VarDecl::CreateDeserialized(Context, TD->getGlobalID()));
    stmts.emplace_back(returnStmt);
  } else {
    mainFunc = TD->lookup(&Context.Idents.get("main")).find_first<clang::FunctionDecl>();
    stmts.emplace(stmts.begin(), mainFunc->getBody());
  }
  clang::SourceRange oldRange = mainFunc->getSourceRange();
  clang::CompoundStmt *body = clang::CompoundStmt::Create(Context, stmts, clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_None), mainFunc->getBeginLoc(), mainFunc->getBodyRBrace());
  mainFunc->setBody(body);
  llvm::outs() << "Body Printed\n";
  std::string mainString = "";
  llvm::raw_string_ostream tempStream(mainString);
  mainFunc->print(tempStream, 0, false);
  // llvm::outs() << mainString << "Main Printed\n";
  if (!Handler.HasMain()) {
    llvm::outs() << "Did Not Have Main\n";
    _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainString);
  } else {
    if (oldRange.isValid() && _Rewriter.isRewritable(mainFunc->getLocation())) {
      llvm::outs() << "Range is Valid\n";
      // oldRange.dump(mgr);
      _Rewriter.RemoveText(oldRange);
      llvm::outs() << "Removed\n";
      _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainString);
      llvm::outs() << "Replaced\n";
    } else {
      llvm::outs() << "Oops\n";
    }
  }
}
