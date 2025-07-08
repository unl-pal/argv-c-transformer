#include "AddVerifiersVisitorFilter.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

AddVerifiersVisitorFilter::AddVerifiersVisitorFilter(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(c), _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

bool AddVerifiersVisitorFilter::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();
  // clang::SourceLocation loc = mgr.getLocForStartOfFile(mgr.getMainFileID());
  // clang::Decl *firstNode = D->getNextDeclInContext();
  clang::Decl *firstNode;
  for (auto *decl : D->decls()) {
    if (mgr.isInMainFile(decl->getLocation())) {
      firstNode = decl;
      break;
    }
  }

  firstNode->dumpColor();
  clang::SourceLocation loc = mgr.translateLineCol(mgr.getMainFileID(), mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
  if (clang::RawComment *comment = _C->getRawCommentForDeclNoCache(firstNode)) {
    llvm::outs() << comment->getRawText(mgr) << "\n";
    loc = comment->getBeginLoc();
  }

  if (_NeededTypes->size()) {
    _Rewriter.InsertTextBefore(loc, "\n");
  }

  std::string nondetName = "__VERIFIER_nondet_";
  for (clang::QualType returnType : *_NeededTypes) {
    std::string returnTypeName = returnType.getAsString();
    llvm::outs() << returnTypeName << "\n";
    std::string newReturnTypeName;
    if (returnTypeName == "_Bool") {
      newReturnTypeName = "bool";
    } else {
      for (unsigned i=0; i<returnTypeName.size(); i++) {
        char letter = returnTypeName[i];
        if (letter == ' ') {
          newReturnTypeName += "";
        } else if (letter == '_') {
          newReturnTypeName += "";
        } else if (letter == '*') {
          newReturnTypeName += "";
        } else {
          newReturnTypeName += letter;
        } 
      }
    }
    // std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    // auto done = returnTypeName.begin();
    // while (done != returnTypeName.end()) {
    //   done = std::remove(returnTypeName.begin(), returnTypeName.end(), '_');
    // }
    // done = returnTypeName.begin();
    // while (done != returnTypeName.end()) {
    //   done = std::remove(returnTypeName.begin(), returnTypeName.end(), '*');
    // }
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + newReturnTypeName);
    clang::DeclarationName declName(funcName);
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType = _C->getFunctionType(returnType, clang::ArrayRef<clang::QualType>(), epi);

    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_C,
      D,
      loc,
      loc,
      declName,
      funcQualType,
      nullptr,
      clang::SC_Extern
    );
    newFunction->setReferenced();
    newFunction->setIsUsed();
    D->addDecl(newFunction);
    _Rewriter.InsertTextBefore(loc, "extern " + newReturnTypeName + " " + newFunction->getNameAsString() + "();\n");
    // loc = newFunction->getBeginLoc();
    // _Output << "extern "  << newFunction->getReturnType() << " " << newFunction->getNameAsString() << "();\n";
  }
  return false;
}
