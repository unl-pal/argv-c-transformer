#include "AddVerifiersConsumer.hpp"

#include "VerifierNames.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/RawCommentList.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <optional>
#include <string>

AddVerifiersConsumer::AddVerifiersConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes,
                                           clang::Rewriter &rewriter)
    : _NeededSuffixes(neededSuffixes), _Rewriter(rewriter) {}

void AddVerifiersConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  if (_NeededSuffixes->empty())
    return;

  clang::SourceManager &mgr = Context.getSourceManager();
  clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl();

  // Insert before the first main-file decl, which keeps the externs below
  // the include block (includes are not decls). Fall back to the start of
  // the file when there are no decls at all.
  clang::SourceLocation loc;
  clang::Decl *firstNode = nullptr;
  for (clang::Decl *decl : TD->decls()) {
    if (mgr.isInMainFile(decl->getLocation()) && !mgr.isMacroBodyExpansion(decl->getLocation())) {
      firstNode = decl;
      break;
    }
  }
  if (firstNode) {
    loc = mgr.translateLineCol(mgr.getMainFileID(),
                               mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
    // If the first node has a doc comment, insert before the comment instead
    if (clang::RawComment *comment = Context.getRawCommentForDeclNoCache(firstNode))
      loc = comment->getBeginLoc();
  } else {
    loc = mgr.translateLineCol(mgr.getMainFileID(), 1, 1);
  }

  std::string decls;
  for (const std::string &suffix : *_NeededSuffixes) {
    std::optional<std::string> cType = cTypeForSuffix(suffix);
    if (!cType)
      continue;
    std::string name = "__VERIFIER_nondet_" + suffix;
    // The filter step may already have injected this extern; skip duplicates
    if (!TD->lookup(clang::DeclarationName(&Context.Idents.get(name))).empty())
      continue;
    decls += "extern " + *cType + " " + name + "(void);\n";
  }

  // HavocCallsVisitor marks pointer-returning call replacements with the
  // helper names; emit the helper definitions they rely on. The helpers
  // hand out valid havocked blocks per the SV-COMP __VERIFIER_nondet_memory
  // contract (an arbitrary nondet pointer value must never be dereferenced).
  bool needCString = _NeededSuffixes->count("__havoc_cstring");
  bool needBlock = needCString || _NeededSuffixes->count("__havoc_block");
  if (needBlock) {
    decls += "extern void __VERIFIER_nondet_memory(void *, unsigned long);\n"
             "extern void *malloc(unsigned long);\n"
             "static void *__havoc_block(unsigned long size) {\n"
             "  void *block = malloc(size);\n"
             "  __VERIFIER_nondet_memory(block, size);\n"
             "  return block;\n"
             "}\n";
  }
  if (needCString) {
    decls += "static char *__havoc_cstring(unsigned long size) {\n"
             "  char *s = __havoc_block(size);\n"
             "  s[size - 1] = '\\0';\n"
             "  return s;\n"
             "}\n";
  }

  if (!decls.empty())
    _Rewriter.InsertTextBefore(loc, decls + "\n");
}
