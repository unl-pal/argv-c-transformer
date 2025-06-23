#include "AddVerifiersVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

AddVerifiersVisitor::AddVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes)
    : _C(c), _Output(output), _NeededTypes(neededTypes) {
  // for (clang::QualType type : *_NeededTypes) {
  //   type->dump();
  // }
}

bool AddVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::SourceLocation loc = mgr.getLocForStartOfFile(mgr.getMainFileID());

  std::string nondetName = "__VERIFIER_nondet_";
  for (clang::QualType returnType : *_NeededTypes) {
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
    _Output << "extern "  << newFunction->getReturnType() << " " << newFunction->getNameAsString() << "();\n";
  }
  return false;
}
