#include "AddMainVisitor.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <vector>

AddMainVisitor::AddMainVisitor(std::vector<std::string> functionsToCall) : _FunctionsToCall(functionsToCall) {
}

bool AddMainVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
  if (!D) return false;

  // std::string returnTypeName = returnType.getAsString();
  // llvm::outs() << returnTypeName << "\n";
  // std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
  // clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + returnTypeName);
  // clang::DeclarationName declName(funcName);
  // clang::FunctionProtoType::ExtProtoInfo epi;
  // clang::QualType funcQualType = _C->getFunctionType(returnType, clang::ArrayRef<clang::QualType>(), epi);
  //
  // clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
  //   *_C,
  //   D,
  //   loc,
  //   loc,
  //   declName,
  //   funcQualType,
  //   nullptr,
  //   clang::SC_Extern
  // );
  // newFunction->setReferenced();
  // newFunction->setIsUsed();
  // D->addDecl(newFunction);
  // _Rewriter.InsertTextBefore(loc, "extern " + newFunction->getReturnType().getAsString() + " " + newFunction->getNameAsString() + "();\n");

  // return clang::RecursiveASTVisitor<AddMainVisitor>::VisitTranslationUnitDecl(D);
  return true;
}
