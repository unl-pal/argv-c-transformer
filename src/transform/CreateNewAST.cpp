#include "include/CreateNewAST.hpp"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

// CreateNewAST::CreateNewAST(std::unique_ptr<clang::ASTUnit> *oldUnit,
//                            std::unique_ptr<clang::ASTUnit> *newUnit)
//     : _NewU(newUnit),
//       _OldU(oldUnit),
//       _NewC(*_NewU->getASTContext()),
//       _OldC(*_OldU->getASTContext()) {}

CreateNewAST::CreateNewAST(clang::Rewriter &R) : _R(R) {};

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
  newC->UnsignedIntTy
  };

  llvm::outs() << "Adding Verifiers\n";
  if (newC->Comments.empty()) {
    llvm::outs() << "No Comments\n";
    return false;
  }
  llvm::outs() << "Adding Verifiers\n";

  // clang::SourceLocation loc;
  // if (auto allComments = newC->Comments.getCommentsInFile(newC->getSourceManager().getFileID(newTD->getLocation()))) {
  //   llvm::outs() << "Adding Verifiers\n";
  //   // llvm::outs() << allComments->count(0);
  //   auto firstComment = allComments->begin();
  //   llvm::outs() << "Adding Verifiers\n";
  //   // llvm::outs() << firstComment << "\n";
  //   // if (firstComment->first) {
  //   //   llvm::outs() << "Adding Verifiers\n";
  //   //   // llvm::outs() << firstComment->second->getRawText(newC->getSourceManager());
  //   //   if (firstComment->second) {
  //   //     loc = firstComment->second->getEndLoc();
  //   //   }
  //   // llvm::outs() << "Adding Verifiers\n";
  //   // }
  //   // if (firstComment->second) {
  //   //   loc = firstComment->second->getEndLoc();
  //   // }
  // }

  clang::SourceLocation loc = oldTD->decls_begin()->getLocation();

    llvm::outs() << "Adding Verifiers\n";
  if (!loc.isValid()) {
    llvm::outs() << "No Comment\n";
    // return false;
  }
  // clang::SourceLocation loc = newTD->getLocation();
  int size = VerifierFuncs.size();
  for (int i=0; i<size; i++) {
    clang::IdentifierInfo *funcName = &newC->Idents.get(VerifierFuncs[i]);
    clang::DeclarationName declName(funcName);
    clang::QualType returnType = ReturnTypes[i];
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
  // if (!loc.isValid()) {
  //   return false;
  // }
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

// std::unique_ptr<clang::ASTUnit> CreateNewAST::getAST() {
//   _NewU->setASTContext(_NewC);
//   std::unique_ptr<clang::ASTUnit> result;
//   result.swap(_NewU);
//   return result;
// }
