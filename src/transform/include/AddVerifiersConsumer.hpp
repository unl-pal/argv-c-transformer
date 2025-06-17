#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <set>

class AddVerifiersConsumer : public clang::ASTConsumer {
public:
  AddVerifiersConsumer(llvm::raw_fd_ostream      &output,
                       std::set<clang::QualType> *neededTypes);

  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  llvm::raw_fd_ostream &_Output;
  std::set<clang::QualType> *_NeededTypes;
};
