#include "AddVerifiersVisitorFilter.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/RawCommentList.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <optional>
#include <string>

AddVerifiersVisitorFilter::AddVerifiersVisitorFilter(clang::ASTContext *c,
                                                     std::shared_ptr<std::set<std::string>> neededTypes,
                                                     clang::Rewriter &rewriter)
    : _C(c), _NeededTypes(neededTypes), _Rewriter(rewriter) {}

bool AddVerifiersVisitorFilter::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();

  // Find the first writable non-include node to use as the insertion point
  clang::Decl *firstNode = nullptr;
  for (auto *decl : D->decls()) {
    if (mgr.isInMainFile(decl->getLocation()) && !mgr.isMacroBodyExpansion(decl->getLocation())) {
      firstNode = decl;
      break;
    }
  }
  if (!firstNode)
    return false;

  clang::SourceLocation loc = mgr.translateLineCol(
      mgr.getMainFileID(), mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
  // If the first node has a doc comment, insert before the comment instead
  if (clang::RawComment *comment = _C->getRawCommentForDeclNoCache(firstNode))
    loc = comment->getBeginLoc();

  _Rewriter.InsertTextBefore(loc, "\n");

  for (const std::string &typeName : *_NeededTypes) {
    std::optional<std::string> cType = cTypeForSuffix(typeName);
    if (!cType)
      continue;
    std::string decl = "extern " + *cType + " __VERIFIER_nondet_" + typeName + "(void);\n";
    _Rewriter.InsertTextBefore(loc, decl);
  }

  return false;
}
