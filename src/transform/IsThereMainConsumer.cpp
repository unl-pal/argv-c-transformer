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

  // Identify the main function via a matcher and assign to the "main" tag
  MatchFinder.addMatcher(
    functionDecl(
      isMain()
    ).bind("main"),
    &Handler);

  // Identify all other functions and add to the "function" tag for call check
  MatchFinder.addMatcher(
    functionDecl(
      unless(
        anyOf(
          isMain(),
          isExpandedFromMacro("*")
        ))).bind("functions"),
    &Handler);

  // Run all matchers on the context
  MatchFinder.matchAST(Context);

  // Variables used in the for loop and beyond
  clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl();
  clang::SourceManager &mgr = Context.getSourceManager();
  std::vector<clang::Stmt*> stmts;

  llvm::outs() << "Calls to Make: " << callsToMake.size() << "\n";
  for (clang::DeclRefExpr *call : callsToMake) { // ends line ... needs rework
    std::vector<clang::Expr*> tempArgs({});
    std::vector<clang::ParmVarDecl*> vars({});
    // get the function declaration for the call to make to get all param info
    if (clang::NamedDecl *namedDecl = call->getFoundDecl()) {
      if (clang::FunctionDecl *func = namedDecl->getAsFunction()) {
        if (!func->param_empty()) {
          // Set param info into the vars vector for use in call
          vars = func->parameters().vec();
        }
      }
    }

    // For each 
    for (clang::ParmVarDecl *var : vars) {
      // get the parameter original type
      clang::QualType varType = var->getOriginalType();
      // outdated as we can handle pointers
      if (varType->isPointerType() || varType->isArrayType()) {
        break;
      }

      // Clean up discrepencies between ClangAST naming and ArgC src code
      std::string varTypeString = varType->isBooleanType() ? "bool" : varType.getAsString();
      std::replace(varTypeString.begin(), varTypeString.end(), ' ', '_');
      std::replace(varTypeString.begin(), varTypeString.end(), '*', '\0');
      if (!std::strcmp(&varTypeString.at(varTypeString.size() - 1), "_")) {
        varTypeString.pop_back();
      }

      clang::IdentifierInfo *info = &Context.Idents.get("__VERIFIER_nondet_" + varTypeString);

      // Get the VERIFIER function declaration info if it exists or create if missing
      clang::DeclarationName name(info);
      clang::DeclContextLookupResult result = TD->lookup(name);
      clang::FunctionDecl *verifier;
      // If no verifier function declaration found build and add one
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

        clang::QualType funcQualType = Context.getFunctionType(
          varType,
          {Context.VoidTy},
          varEpi
        );

        verifier = clang::FunctionDecl::Create(
          Context,
          TD,
          insertFirst,
          insertFirst.getLocWithOffset(1),
          name,
          funcQualType,
          Context.CreateTypeSourceInfo(varType),
          clang::SC_Extern
        );

        verifier->setLocation(insertFirst);
        verifier->setReferenced();
        verifier->setIsUsed();

        clang::ParmVarDecl *vParm = clang::ParmVarDecl::Create(
          Context,
          verifier->getDeclContext(),
          verifier->getLocation(),
          verifier->getLocation(),
          nullptr,
          Context.VoidTy,
          nullptr,
          clang::SC_None,
          nullptr
        );

        verifier->setParams({vParm});
        vParm->setOwningFunction(verifier);
        verifier->addDecl(vParm);

        verifier->addDecl(clang::ParmVarDecl::Create(
          Context,
          verifier->getDeclContext(),
          verifier->getInnerLocStart(),
          verifier->getLocation(),
          &Context.Idents.get(""),
          Context.VoidTy,
          Context.CreateTypeSourceInfo(Context.VoidTy),
          clang::SC_None, nullptr
        ));

        TD->addDecl(verifier);
        if (insertFirst.isValid()) {
          std::string verifierString = "";
          llvm::raw_string_ostream verifierStream(verifierString);
          // verifier->dumpColor();
          // Print in this case is building the string that the Rewriter uses
          verifier->getAsFunction()->print(verifierStream, 0, true);
          _Rewriter.InsertTextBefore(insertFirst, verifierString + ";\n");
        }
      } else {
        verifier = result.find_first<clang::FunctionDecl>();
      }

      // create a call for the verifier function found or created
      clang::DeclRefExpr *verifierCall = clang::DeclRefExpr::Create(
        Context,
        verifier->getQualifierLoc(),
        clang::SourceLocation(),
        verifier,
        false,
        verifier->getLocation(),
        var->getType(),
        clang::ExprValueKind::VK_LValue,
        verifier,
        nullptr
      );

      clang::CallExpr *tempExpr = clang::CallExpr::Create(
        Context,
        verifierCall,
        {},
        var->getType(),
        clang::ExprValueKind::VK_LValue,
        var->getLocation(),
        clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto)
      );
      tempArgs.push_back(tempExpr);
    }

    if (tempArgs.size() < vars.size()) {
      break;
    }

    // For each parameter add a corresponding verifier call
    llvm::outs() << "Args Size: " << tempArgs.size() << "\n";
    clang::CallExpr *callExpr = clang::CallExpr::Create(
      Context,
      call,
      tempArgs,
      call->getType(),
      clang::ExprValueKind::VK_LValue,
      call->getLocation(),
      clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_Auto)
    );
    stmts.emplace_back(callExpr);
  } // end of for (clang::DeclRefExpr *call : callsToMake) on line 67

  clang::FunctionDecl* mainFunc;
  // Create main function if missing
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

    clang::ParmVarDecl *parm = clang::ParmVarDecl::Create(
      Context,
      mainFunc->getDeclContext(),
      mainFunc->getInnerLocStart(),
      mainFunc->getLocation(),
      nullptr,
      Context.VoidTy,
      nullptr,
      clang::SC_None,
      nullptr
    );

    std::vector<clang::ParmVarDecl*> parms = {parm};
    llvm::outs() << "Parameters size: " << parms.size() << "\n";
    mainFunc->setParams({parm});
    mainFunc->addDecl(parm);

    mainFunc->addDecl(clang::ParmVarDecl::Create(
      Context,
      mainFunc->getDeclContext(),
      mainFunc->getInnerLocStart(),
      mainFunc->getLocation(),
      &Context.Idents.get(""),
      Context.VoidTy,
      Context.CreateTypeSourceInfo(Context.VoidTy),
      clang::SC_None,
      nullptr
    ));

    TD->addDecl(mainFunc);

    clang::ReturnStmt *returnStmt = clang::ReturnStmt::Create(
      Context,
      mainFunc->getLocation(),
      clang::IntegerLiteral::Create(Context,
                                    llvm::APInt::doubleToBits(0),
                                    Context.IntTy,
                                    mainFunc->getLocation()),
      clang::VarDecl::CreateDeserialized(Context, TD->getGlobalID())
    );

    // add a return statement to the newly formed body
    stmts.emplace_back(returnStmt);
  } else {
    mainFunc = TD->lookup(&Context.Idents.get("main")).find_first<clang::FunctionDecl>();
    // add original body to the beginning of the new body
    stmts.emplace(stmts.begin(), mainFunc->getBody());
  }

  // save the current location occupied by the new or existing main function
  clang::SourceRange oldRange = mainFunc->getSourceRange();

  // create the body from all the added statements and old body if exists
  clang::CompoundStmt *body = clang::CompoundStmt::Create(
    Context,
    stmts,
    clang::FPOptionsOverride::getFromOpaqueInt(clang::SC_None),
    mainFunc->getBeginLoc(),
    mainFunc->getBodyRBrace()
  );

  // Set main to use the new body
  mainFunc->setBody(body);

  // use string and string stream to use the built in print to stream function
  std::string mainString = "";
  llvm::raw_string_ostream tempStream(mainString);
  mainFunc->print(tempStream, 0, false);
  if (!Handler.HasMain()) {
    // llvm::outs() << "Did Not Have Main\n";
    // Place the new main function at the end of the main file if newly created
    _Rewriter.InsertTextBefore(mgr.getLocForEndOfFile(mgr.getMainFileID()), mainString);
  } else {
    if (oldRange.isValid() && _Rewriter.isRewritable(mainFunc->getLocation())) {
      // Replace the old main function with the new main if already existed
      _Rewriter.ReplaceText(oldRange, mainString);
    } else {
      llvm::outs() << "Oops\n";
    }
  }
}
