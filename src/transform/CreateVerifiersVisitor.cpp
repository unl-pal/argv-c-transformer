#include "CreateVerifiersVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

/*
 * This code needs to be modified to take in a list of used nondet types and
 * then produce only those ones
 *
 * This code needs to be redeveloped to only be run on the Translation unit for
 * the benchmark file rather than also the Verifier file as that is the cause of
 * the duplicates
 */

CreateVerifiersVisitor::CreateVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes)
    : _C(c), _Output(output), _NeededTypes(neededTypes) {
  for (clang::QualType type : *_NeededTypes) {
    type->dump();
  }
}

// TODO fix the double run due to including the Verifiers file
bool CreateVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
//   if (_C->getSourceManager().isInMainFile(D->getLocation())) {
//     return true;
//   }
  llvm::outs() << "Running the Handle TU in AddVerifiersVisitor\n";
  // std::vector<clang::CanQualType> ReturnTypes = {
  // _C->CharTy,
  // _C->DoubleTy,
  // _C->FloatTy,
  // _C->IntTy,
  // _C->LongTy,
  // _C->LongLongTy,
  // _C->ShortTy,
  // _C->VoidTy,
  // _C->UnsignedIntTy
  // };

  clang::SourceManager &mgr = _C->getSourceManager();
  clang::SourceLocation loc = mgr.getLocForStartOfFile(mgr.getMainFileID());

  // int size = ReturnTypes.size();
  // for (int i=0; i<size; i++) {
  //   clang::QualType returnType = ReturnTypes[i];
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
    llvm::outs() << "Function: " << newFunction->getNameAsString() << " was created\n";
  }
  // D->dump();
  // return true;
  return false;
}
