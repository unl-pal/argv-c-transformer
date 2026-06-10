#include "AddVerifiersVisitorFilter.hpp"

#include <clang/AST/RawCommentList.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <unordered_map>

// Maps verifier name suffix to the C type string used in the extern declaration.
static const std::unordered_map<std::string, std::string> kCTypeNames = {
    {"bool", "_Bool"},
    {"char", "char"},
    {"uchar", "unsigned char"},
    {"short", "short"},
    {"ushort", "unsigned short"},
    {"int", "int"},
    {"uint", "unsigned int"},
    {"long", "long"},
    {"ulong", "unsigned long"},
    {"longlong", "long long"},
    {"ulonglong", "unsigned long long"},
    {"float", "float"},
    {"double", "double"},
};

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
    auto it = kCTypeNames.find(typeName);
    if (it == kCTypeNames.end())
      continue;
    std::string decl = "extern " + it->second + " __VERIFIER_nondet_" + typeName + "(void);\n";
    _Rewriter.InsertTextBefore(loc, decl);
  }

  return false;
}
