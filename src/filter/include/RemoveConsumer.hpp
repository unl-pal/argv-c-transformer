#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>
#include <set>

class RemoveConsumer : public clang::ASTConsumer {
public:
  /// Constructs Consumer setting the rewriter and vector of functions that will
  /// be removed by the visitor
  RemoveConsumer(clang::Rewriter &rewriter, std::vector<std::string> *toRemove, std::set<clang::QualType> *neededTypes);

  /// Calls the RemoveVisitor to remove all functions specified by the toRemove
  /// vector using the rewriter object to modify the source code
  void HandleTranslationUnit(clang::ASTContext &Context);

private:
  clang::Rewriter &_Rewriter;
  std::vector<std::string> *_toRemove;
  std::set<clang::QualType> *_NeededTypes;
};
