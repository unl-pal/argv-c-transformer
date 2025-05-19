#include "include/CreateNewAST.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/CanonicalType.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

CreateNewAST::CreateNewAST(clang::Rewriter &R, clang::SourceManagerForFile &SMF)
    : _R(R), _SMF(SMF) {};

bool CreateNewAST::AddVerifiers(clang::ASTContext *newC, clang::ASTContext *oldC) {
  clang::TranslationUnitDecl *newTD = newC->getTranslationUnitDecl();
  clang::TranslationUnitDecl *oldTD = oldC->getTranslationUnitDecl();

  std::vector<clang::CanQualType> ReturnTypes = {
  newC->CharTy,
  newC->DoubleTy,
  newC->FloatTy,
  newC->IntTy,
  newC->LongTy,
  newC->LongLongTy,
  newC->ShortTy,
  newC->VoidTy,
  newC->UnsignedIntTy
  };

  // clang::SourceLocation loc = _SMF.get().getIncludeLoc(FileID FID);
  // TODO this may solve the other issues with benchmark source and comments
  clang::SourceLocation loc = _SMF.get().getLocForStartOfFile(newC->getSourceManager().getMainFileID());
  // _SMF.get().dump();
  if (loc.isValid()) {
    llvm::outs() << "Valid Location\n";
  }

  int size = ReturnTypes.size();
  std::string nondetName = "__VERIFIER_nondet_";
  for (int i=0; i<size; i++) {
    clang::QualType returnType = ReturnTypes[i];
    std::string returnTypeName = returnType.getAsString();
    std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    clang::IdentifierInfo *funcName = &newC->Idents.get(nondetName + returnTypeName);
    clang::DeclarationName declName(funcName);
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType = newC->getFunctionType(returnType, clang::ArrayRef<clang::QualType>(), epi);

    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *newC,
      oldTD,
      loc,
      loc,
      declName,
      funcQualType,
      nullptr,
      clang::SC_Extern
    );
    newFunction->setReferenced();
    newFunction->setIsUsed();
    newTD->addDecl(newFunction);
    _R.InsertTextAfter(loc, newFunction->getQualifiedNameAsString() + "();");
    loc = newFunction->getEndLoc();
  }
  return true;
}

bool CreateNewAST::AddBoolDef(clang::ASTContext *newC, clang::ASTContext *oldC) {
  clang::TranslationUnitDecl *newTD = newC->getTranslationUnitDecl();
  clang::SourceLocation loc = newTD->getEndLoc();
  clang::TypedefDecl* newTypeDef = clang::TypedefDecl::Create(
    *oldC,
    newC->getTranslationUnitDecl(),
    loc,
    loc,
    &newC->Idents.get("bool"),
    oldC->getTrivialTypeSourceInfo(oldC->BoolTy)
  );
  newTypeDef->setIsUsed();
  newTypeDef->setReferenced();
  newTD->addDecl(newTypeDef);
  // _R.InsertTextAfter(loc, newTypeDef->getQualifiedNameAsString() + ";");
  return true;
}

bool CreateNewAST::AddAllDecl(clang::ASTContext *newC, clang::ASTContext *oldC) {
  for (clang::Decl *decl : oldC->getTranslationUnitDecl()->decls()) {
      newC->getTranslationUnitDecl()->addDecl(decl);
  }
  if (newC->getTranslationUnitDecl()->decls_empty()) {
    return false;
  }
  return true;
}
