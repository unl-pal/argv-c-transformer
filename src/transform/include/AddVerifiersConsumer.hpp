#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <set>

class AddVerifiersConsumer : public clang::ASTConsumer {
public:
  /// Consumer that launches the visitor that will create the verrifier
  /// functions and add them to the code file
  /// @param - output stream to print to
  /// @param - types that needs verrifiers to be created
  AddVerifiersConsumer(llvm::raw_fd_ostream      &output,
                       std::set<clang::QualType> *neededTypes);

  /// Calls the AddVerifiersVisitor and supplies the needed context
  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  llvm::raw_fd_ostream &_Output;
  std::set<clang::QualType> *_NeededTypes;
};
