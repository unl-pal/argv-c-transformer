#include "AddVerifiersVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <cstring>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

AddVerifiersVisitor::AddVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(c), _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

bool AddVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::Decl *firstNode = nullptr;
  for (auto *decl : D->decls()) {
    if (!mgr.isMacroArgExpansion(decl->getLocation())) {
      if (mgr.isInMainFile(decl->getLocation())) {
        firstNode = decl;
        break;
      }
    }
  }
  
  clang::SourceLocation loc;
  if (firstNode && firstNode->getSourceRange().isValid()){
    // firstNode->dumpColor();

    loc = mgr.translateLineCol(mgr.getMainFileID(), mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
    if (clang::RawComment *comment = _C->getRawCommentForDeclNoCache(firstNode)) {
      // if (comment) {
        llvm::outs() << comment->getRawText(mgr) << "\n";
        loc = comment->getBeginLoc();
      // }
    }
  } else {
    loc = mgr.translateLineCol(mgr.getMainFileID(), 1, 1);
  }

  if (_NeededTypes->size()) {
    _Rewriter.InsertTextBefore(loc, "\n");
  }

  std::string nondetName = "__VERIFIER_nondet_";
  for (clang::QualType returnType : *_NeededTypes) {
    // std::string returnTypeName;

    // Should probably have a enum or def somewhere with all supported
    // verrifiers to draw on for situations like this, or in config?
    if (!returnType->isBuiltinType() &&
        !returnType->isBooleanType() &&
        !returnType->isArrayType() &&
        !returnType->isAnyCharacterType() &&
        !returnType->isVoidType() &&
        !returnType->isAnyPointerType()
    ) {
      continue;
    }

    bool isPointer = returnType->isAnyPointerType();
    std::string returnTypeName = returnType->isBooleanType() ? "bool" : returnType.getAsString();
    returnTypeName = isPointer ? "pointer" : returnTypeName;
    llvm::outs() << returnTypeName << "\n";
    std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    std::replace(returnTypeName.begin(), returnTypeName.end(), '*', '\0');
    if (!std::strcmp(&returnTypeName.at(returnTypeName.size() - 1), "_")) {
      returnTypeName.pop_back();
    }
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + returnTypeName);
    clang::DeclarationName declName(funcName);
    if (!D->lookup(declName).empty()) {
      continue;
    }
    clang::FunctionProtoType::ExtProtoInfo epi;

    clang::QualType funcQualType;
    if (isPointer) {
      funcQualType = _C->getFunctionType(_C->VoidPtrTy, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    } else {
      funcQualType = _C->getFunctionType(returnType, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    }

    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_C,
      D,
      loc,
      loc.getLocWithOffset(1),
      declName,
      funcQualType,
      _C->CreateTypeSourceInfo(_C->VoidTy),
      clang::SC_Extern
    );

    newFunction->setReferenced();
    newFunction->setIsUsed();

    clang::ParmVarDecl *vParm = clang::ParmVarDecl::Create(
      *_C,
      newFunction->getDeclContext(),
      newFunction->getLocation(),
      newFunction->getLocation(),
      nullptr,
      _C->VoidTy,
      nullptr,
      clang::SC_None,
      nullptr
    );

    newFunction->setParams({vParm});
    vParm->setOwningFunction(newFunction);
    newFunction->addDecl(vParm);
    D->addDecl(newFunction);
    if (loc.isValid() && mgr.isInMainFile(loc)) {
      std::string verifierString = "";
      llvm::raw_string_ostream tempStream(verifierString);
      // newFunction->getAsFunction()->print(tempStream, 0, true);
      newFunction->print(tempStream);
      llvm::outs() << verifierString << " - The String\n";
      _Rewriter.InsertTextBefore(loc, verifierString + ";\n");
      llvm::outs() << "Inserted the text\n";
    }
  }
  return false;
}
