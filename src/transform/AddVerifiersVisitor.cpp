#include "AddVerifiersVisitor.hpp"
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

AddVerifiersVisitor::AddVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(c), _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

bool AddVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();
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
    std::string returnTypeName = returnType->isBooleanType() ? "bool" : returnType.getAsString();
    llvm::outs() << returnTypeName << "\n";
    std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + returnTypeName);
    clang::DeclarationName declName(funcName);
    if (D->lookup(declName).find_first<clang::FunctionDecl>()) {
      continue;
    }
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType = _C->getFunctionType(returnType, {_C->VoidTy}, epi);

    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_C,
      D,
      loc,
      loc,
      declName,
      funcQualType,
      _C->CreateTypeSourceInfo(returnType),
      clang::SC_Extern
    );
    newFunction->setReferenced();
    newFunction->setIsUsed();
    D->addDecl(newFunction);
    // _Rewriter.InsertTextBefore(loc, "extern " + newFunction->getReturnType().getAsString() + " " + newFunction->getNameAsString() + "();\n");
    std::string verifierString = "";
    llvm::raw_string_ostream tempStream(verifierString);
    newFunction->getAsFunction()->print(tempStream, 0, true);
    _Rewriter.InsertTextBefore(loc, verifierString + ";\n");
  }
  return false;
}
