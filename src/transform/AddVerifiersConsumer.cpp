// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "AddVerifiersConsumer.hpp"

#include "HavocPolicy.hpp"
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
  clang::SourceLocation firstNodeBegin;
  for (clang::Decl *decl : TD->decls()) {
    // macro-expanded return types cause wrong location, so normalize using the expansion location
    clang::SourceLocation begin = mgr.getExpansionLoc(decl->getBeginLoc());
    if (mgr.isInMainFile(decl->getLocation()) && !mgr.isMacroBodyExpansion(begin)) {
      firstNode = decl;
      firstNodeBegin = begin;
      break;
    }
  }
  if (firstNode) {
    loc = mgr.translateLineCol(mgr.getMainFileID(),
                               mgr.getExpansionLineNumber(firstNodeBegin), 1);
    // If the first node has a doc comment, insert before the comment instead
    if (clang::RawComment *comment = Context.getRawCommentForDeclNoCache(firstNode))
      loc = comment->getBeginLoc();
  } else {
    loc = mgr.translateLineCol(mgr.getMainFileID(), 1, 1);
  }

  std::string decls;

  // "__havoc_memory"/"size_t" put a bare `size_t` in the emitted externs, and
  // both that path and "__havoc_argv" emit `abort()` calls (argc/argv/length
  // bound checks). <stdlib.h> covers both size_t and abort.
  if (_NeededSuffixes->count("__havoc_memory") || _NeededSuffixes->count("size_t") ||
      _NeededSuffixes->count("__havoc_argv"))
    decls += "#include <stdlib.h>\n";

  // MainGenConsumer marks the argv harness with "__havoc_argv"; emit the
  // bounds it references as macros so a user can retune a generated benchmark
  // without rerunning the pipeline.
  if (_NeededSuffixes->count("__havoc_argv")) {
    decls += "#define __HAVOC_ARGC_MIN " + std::to_string(kArgcMin) + "\n" +
             "#define __HAVOC_ARGC_MAX " + std::to_string(kArgcMax) + "\n" +
             "#define __HAVOC_STR_MAX " + std::to_string(kStrMax) + "\n";
  }

  // emit verifier externs
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

  // HavocCallsVisitor and MainGenConsumer mark stack-block pointer havocking
  // with "__havoc_memory"; emit the extern + bound macro they rely on. The
  // blocks are filled per the SV-COMP __VERIFIER_nondet_memory contract.
  if (_NeededSuffixes->count("__havoc_memory")) {
    decls += "#define __HAVOC_BLOCK_MAX " + std::to_string(kBlockMax) + "\n";
    decls += "extern void __VERIFIER_nondet_memory(void *, size_t);\n";
  }
  if (_NeededSuffixes->count("size_t"))
    decls += "extern size_t __VERIFIER_nondet_size_t(void);\n";

  // AssertRewriter rewrote at least one assert(cond) to reach_error()
  if (_NeededSuffixes->count("__reach_error")) {
    decls += "#include <assert.h>\n"
             "void reach_error(void) { assert(0); }\n";
  }

  if (!decls.empty())
    _Rewriter.InsertTextBefore(loc, decls + "\n");
}
