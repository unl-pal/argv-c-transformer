#include "CreateVerifiersVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

CreateVerifiersVisitor::CreateVerifiersVisitor(clang::ASTContext           *c,
                                               clang::SourceManagerForFile &smf,
                                               llvm::raw_fd_ostream &output)
    : _C(c), _SMF(smf), _Output(output) {}

bool CreateVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  // clang::Sour
  std::vector<clang::CanQualType> ReturnTypes = {
  _C->CharTy,
  _C->DoubleTy,
  _C->FloatTy,
  _C->IntTy,
  _C->LongTy,
  _C->LongLongTy,
  _C->ShortTy,
  _C->VoidTy,
  _C->UnsignedIntTy
  };

  clang::SourceLocation loc = _SMF.get().getLocForStartOfFile(_SMF.get().getMainFileID());

  int size = ReturnTypes.size();
  std::string nondetName = "__VERIFIER_nondet_";
  for (int i=0; i<size; i++) {
    clang::QualType returnType = ReturnTypes[i];
    std::string returnTypeName = returnType.getAsString();
    std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + returnTypeName);
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
    loc = newFunction->getEndLoc();
    _Output << newFunction->getNameAsString() << "();\n";
  }
  return true;
}
