#include "AddMainVisitor.hpp"
#include "AllFunctionsToCallHandler.hpp"
#include "IsThereMainConsumer.hpp"
#include "IsThereMainHandler.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
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
#include <sstream>
#include <vector>

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
  // if (Context.Idents.find("main") != Context.Idents.end()) {
  //   auto lookup = Context.Idents.find("main");
  //   llvm::outs() << "Found Lookup\n";
  //   // if (lookup) {
  //     // llvm::outs() << "Found main name\n";
  //     if (clang::DeclarationName main = lookup->second) {
  //       llvm::outs() << "Found main info\n";
  //       if (clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl()) {
  //         llvm::outs() << "Found TD\n";
  //         if (clang::FunctionDecl *func = TD->lookup(main).find_first<clang::FunctionDecl>()) {
  //           llvm::outs() << "Found main\n";
  //           func->dumpColor();
  //         }
  //       }
  //     }
  //   // }
  // }
  // context.getTranslationUnitDecl()->dumpColor();
  // MatchFinder.addMatcher(decl().bind("something"), &Handler);
  MatchFinder.addMatcher(functionDecl(unless(isMain())).bind("functions"), &Handler);
  // MatchFinder.addMatcher(functionDecl(isMain()).bind("main"), &Handler);
  MatchFinder.addMatcher(functionDecl(matchesName("main")).bind("main"), &Handler);
  // llvm::outs() << "Add Ze Mache\n";
  MatchFinder.matchAST(Context);
  // llvm::outs() << "Run Ze Mache\n";
  clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl();
  clang::SourceManager &mgr = Context.getSourceManager();
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
    for (clang::ParmVarDecl *var : vars) {
      clang::QualType varType = var->getType();
      std::string varTypeString = varType->isBooleanType() ? "bool" : varType.getAsString();
      std::replace(varTypeString.begin(), varTypeString.end(), ' ', '_');
      clang::IdentifierInfo *info = &Context.Idents.get("__VERIFIER_nondet_" + varTypeString);
      clang::DeclarationName name(info);
      clang::DeclContextLookupResult result = TD->lookup(name);
      clang::FunctionDecl *verifier;
      llvm::outs() << "PreProto\n";
      if (result.empty()) {
      // if (result.find_first<clang::FunctionDecl>()) {
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
        // clang::QualType funcQualType = Context.getFunctionType(Context.IntTy, {Context.VoidTy}, epi);
        clang::FunctionProtoType::ExtProtoInfo varEpi;
        clang::QualType funcQualType = Context.getFunctionType(varType, clang::ArrayRef<clang::QualType>({Context.VoidTy}), varEpi);
        // clang::QualType funcQualType = Context.getFunctionType(Context.getCanonicalParamType(varType), {Context.VoidTy}, varEpi);
        verifier = clang::FunctionDecl::Create(Context, TD, insertFirst, insertFirst.getLocWithOffset(1), name, funcQualType, Context.CreateTypeSourceInfo(varType), clang::SC_Extern);
        verifier->setWillHaveBody(false);
        // verifier = clang::FunctionDecl::Create(Context, TD, insertFirst, insertFirst, name, funcQualType, nullptr, clang::SC_Extern);
        // verifier = clang::FunctionDecl::Create(Context, TD, insertFirst, insertFirst, name, varType, nullptr, clang::SC_Extern);
        // verifier->setParams();

        clang::ParmVarDecl *vParm = clang::ParmVarDecl::Create(Context, verifier->getDeclContext(), verifier->getLocation(), verifier->getLocation(), nullptr, Context.VoidTy, nullptr, clang::SC_None, nullptr);
        verifier->setParams({vParm});
        vParm->setOwningFunction(verifier);
        verifier->addDecl(vParm);

        // verifier->addDecl(clang::ParmVarDecl::Create(Context, verifier->getDeclContext(), verifier->getInnerLocStart(), verifier->getLocation(), &Context.Idents.get(""), Context.VoidTy, Context.CreateTypeSourceInfo(Context.VoidTy), clang::SC_None, nullptr));
        TD->addDecl(verifier);
        if (verifier) {
          llvm::outs() << "Well at least there is that\n";
          verifier->dumpColor();
        }
        if (insertFirst.isValid()) {
          // _Rewriter.InsertTextBefore(insertFirst, varType.getAsString() + " " + verifier->getNameAsString() + "();\n");
          std::string verifierString = "";
          llvm::raw_string_ostream verifierStream(verifierString);
          llvm::outs() << "PreProto\n";
          verifier->dumpColor();
          verifier->getAsFunction()->print(verifierStream, 0, true);
          // verifier->print(tempStream, 0, true);
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
    // args.vec().(tempArgs);
    llvm::outs() << "Args Size: " << tempArgs.size() << "\n";
    clang::CallExpr *callExpr = clang::CallExpr::Create(Context, call, tempArgs, call->getType(), clang::ExprValueKind::VK_LValue, call->getLocation(), clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto));
    callExpr->dumpColor();
    stmts.emplace_back(callExpr);
    // clang::Decl *callee = callExpr->getCalleeDecl();
    

    // mainFunc->addDecl(callee);

    // llvm::outs() << 
    // _Rewriter.InsertTextAfter(Context.getSourceManager().getLocForEndOfFile(Context.getSourceManager().getMainFileID()), callExpr)
  }
  clang::FunctionDecl* mainFunc;
  if (!Handler.HasMain()) {
    llvm::outs() << "Not Haz Ze Maene\n";
    llvm::outs() << "Building Main\n";
    clang::SourceLocation loc = mgr.getLocForEndOfFile(mgr.getMainFileID());
    clang::IdentifierInfo *funcName = &Context.Idents.get("main");
    clang::DeclarationName declName(funcName);
    clang::FunctionProtoType::ExtProtoInfo epi;
    // clang::FunctionProtoType::ExtParameterInfo epi;
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
      // false,
      // false,
      // false,
      // clang::ConstexprSpecKind::Unspecified,
      // nullptr
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
  mainFunc->dumpColor();
  std::string mainString = "";
  llvm::raw_string_ostream tempStream(mainString);
  mainFunc->print(tempStream, 0, true);
  if (!Handler.HasMain()) {
    _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainString);
  } else {
    _Rewriter.ReplaceText(oldRange, mainString);
  }
}
