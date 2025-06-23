#include "RemoveVisitor.hpp"
#include "RemoveConsumer.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

RemoveConsumer::RemoveConsumer(clang::Rewriter rewriter, std::vector<std::string> toRemove) : _Rewriter(rewriter), _toRemove(toRemove) {}

void RemoveConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  RemoveFuncVisitor Visitor(&Context, _Rewriter, _toRemove);
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
}
