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
                                           std::shared_ptr<std::set<std::string>> existingIncludes,
                                           clang::Rewriter &rewriter, const HavocBounds &havoc)
    : _NeededSuffixes(neededSuffixes), _ExistingIncludes(existingIncludes), _Rewriter(rewriter),
      _Havoc(havoc) {}

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

  // A header this consumer needs may already be in the output (the source
  // included it and IncludeFinder kept it), and AddStdIncludesConsumer runs
  // after this one off the same set, so record what is emitted here too.
  auto includeText = [&](const std::string &header) -> std::string {
    if (!_ExistingIncludes->insert(header).second)
      return "";
    return "#include <" + header + ">\n";
  };

  // Earlier consumers mark the argv harness with "__havoc_argv" and any
  // pointer havocking with "__havoc_bounds". Emit the bounds those reference as
  // macros so a user can retune a generated benchmark without rerunning the
  // pipeline. They are emitted as a set rather than individually: the group is
  // small, and a benchmark whose knobs are all in one place is easier to retune
  // than one where the available knobs depend on which shapes it happens to use.
  if (_NeededSuffixes->count("__havoc_argv")) {
    decls += "#define __HAVOC_ARGC_MIN " + std::to_string(_Havoc.argcMin) + "\n" +
             "#define __HAVOC_ARGC_MAX " + std::to_string(_Havoc.argcMax) + "\n";
  }
  if (_NeededSuffixes->count("__havoc_argv") || _NeededSuffixes->count("__havoc_bounds")) {
    decls += "#define __HAVOC_STR_MAX " + std::to_string(_Havoc.strMax) + "\n" +
             "#define __HAVOC_ARRAY_ELEMS " + std::to_string(_Havoc.arrayElems) + "\n" +
             "#define __HAVOC_OPAQUE_BYTES " + std::to_string(_Havoc.opaqueBytes) + "\n";
  }

  // A struct named only inside a parameter list has prototype scope, so the
  // cast the harness emits for it would otherwise name a distinct, incompatible
  // type. Hoist every such tag to file scope. Redundant when the record is
  // already declared here, but a repeat forward declaration is legal C.
  for (const std::string &suffix : *_NeededSuffixes) {
    if (suffix.rfind("__havoc_fwd:", 0) == 0)
      decls += suffix.substr(std::string("__havoc_fwd:").size()) + ";\n";
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

  // Pointer havocking fills stack storage with __VERIFIER_nondet_memory (an
  // arbitrary nondet pointer *value* must never be dereferenced, so a filled
  // block is the only safe form). Both sites - harnessed parameters
  // (MainGenConsumer) and pointer-returning calls (HavocCallsVisitor) - inline
  // the storage and mark it "__havoc_mem"; a cstring additionally needs
  // nondet_size_t and abort and marks "__havoc_str". <stdlib.h> supplies size_t
  // (in the nondet_memory signature) and abort.
  bool needMemory = _NeededSuffixes->count("__havoc_mem");
  bool needSizeNondet = _NeededSuffixes->count("__havoc_str");
  if (needMemory) {
    decls.insert(0, includeText("stdlib.h"));
    if (needSizeNondet && !_NeededSuffixes->count("size_t"))
      decls += "extern size_t __VERIFIER_nondet_size_t(void);\n";
    decls += "extern void __VERIFIER_nondet_memory(void *, size_t);\n";
  }

  // AssertRewriter rewrote at least one assert(cond) to reach_error()
  if (_NeededSuffixes->count("__reach_error")) {
    decls += includeText("assert.h") + "void reach_error(void) { assert(0); }\n";
  }

  if (!decls.empty())
    _Rewriter.InsertTextBefore(loc, decls + "\n");
}
