#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>

class IsThereMainConsumer : public clang::ASTConsumer {
public:
  /// Object for identifying if there is a main function already in the source code
  /// @param rewriter rewriter for changing source code of AST
  IsThereMainConsumer(clang::Rewriter &rewriter);

  /// Starts the action for identifying the main function
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  clang::Rewriter &_Rewriter;
};
