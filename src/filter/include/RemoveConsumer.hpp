#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

class RemoveConsumer : public clang::ASTConsumer {
public:
  RemoveConsumer(clang::Rewriter &rewriter, std::vector<std::string> *toRemove);

  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  clang::Rewriter &_Rewriter;
  std::vector<std::string> *_toRemove;
};
