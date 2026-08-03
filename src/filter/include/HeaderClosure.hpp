// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/StringRef.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

/**
 * @file HeaderClosure.hpp
 * @brief Inlines the declarations and macros a filtered file actually uses out
 * of its project-local headers, so the output stands alone.
 *
 * Two rules, per docs/HeaderClosure.md:
 *
 * - **System headers are included by reference.** `#include <stdio.h>` names
 *   something the target machine provides; it is kept verbatim.
 * - **Local headers are inlined by value.** The target machine does not have
 *   them, so whatever surviving code references is copied in.
 *
 * This replaces the older "strip local includes, then reconstruct what was in
 * them" approach, in which each downstream mechanism recovered one projection
 * of a definition that had been deleted upstream of all of them (names via
 * diagnostics, headers via a curated registry, layouts not at all).
 *
 * Two closures run over the same roots and neither subsumes the other:
 *
 * - the **declaration closure** walks the AST, and cannot see macros at all —
 *   by the time an AST exists, `char buf[BUFSIZE]` is an array of 64 with a
 *   macro-expansion SourceLocation and no decl to reach from;
 * - the **macro closure** runs on the preprocessor and cannot see types.
 */

/** @brief One macro defined in a project-local header, captured verbatim. */
struct MacroRecord {
  /// Full `#define ...` line(s), exactly as spelled. Re-serializing the token
  /// list instead would break benign redefinition and lose formatting.
  std::string text;
  /// Raw SourceLocation encoding of the `#define`, used to restore source order.
  unsigned order = 0;
};

/**
 * @brief State handed from the preprocessor callbacks to the closure consumer.
 *
 * The preprocessor has run to completion by the time any ASTConsumer's
 * HandleTranslationUnit is called, so everything here is fully populated when
 * HeaderClosureConsumer reads it.
 */
struct HeaderClosureState {
  /// True once at least one project-local #include was stripped. When false
  /// there is no header content to inline and the closure is skipped entirely.
  bool strippedLocalInclude = false;
  /// Layer 1 of the system-header recovery: angled includes written inside a
  /// project-local header. Over-inclusive, but never names something
  /// un-includable, because it is a directive a human wrote.
  std::set<std::string> systemIncludes;
  /// Every macro defined in a local header, by name. A superset of what gets
  /// emitted; the consumer picks the reachable ones.
  std::map<std::string, MacroRecord> localMacros;
  /// Local-header macro expansions that landed in the main file, with the
  /// expansion location so uses inside rejected function bodies can be dropped.
  std::vector<std::pair<std::string, clang::SourceLocation>> macroUses;
};

/**
 * @brief PPCallbacks hook feeding the closure: strips local includes, records
 * system includes reachable through them, and captures local macros.
 *
 * Stripping happens here rather than in the transform stage because the
 * closure's replacement text has to be *parsed* by transform — a macro
 * recovered here must be visible to Sema when havocking runs, and verify
 * reparses but never re-havocs.
 */
class LocalHeaderPP : public clang::PPCallbacks {
public:
  /**
   * @brief Constructs the callback.
   *
   * @param SM       Source manager for the translation unit.
   * @param langOpts Language options, needed to re-lex captured spellings.
   * @param rewriter Shared rewriter the include directives are removed through.
   * @param state    Output state, read later by HeaderClosureConsumer.
   */
  LocalHeaderPP(clang::SourceManager &SM, const clang::LangOptions &langOpts,
                clang::Rewriter &rewriter, std::shared_ptr<HeaderClosureState> state);

  /** @brief Removes project-local includes from the main file; records angled
   *  includes written inside a local header for later re-emission. */
  void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
                          llvm::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
                          llvm::StringRef SearchPath, llvm::StringRef RelativePath,
                          const clang::Module *SuggestedModule, bool ModuleImported,
                          clang::SrcMgr::CharacteristicKind FileType) override;

  /** @brief Captures the exact spelling of every macro defined in a local header. */
  void MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective *MD) override;

  /** @brief Records main-file expansions of local-header macros. */
  void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &MD,
                    clang::SourceRange Range, const clang::MacroArgs *Args) override;

private:
  clang::SourceManager &_Mgr;
  const clang::LangOptions &_LangOpts;
  clang::Rewriter &_Rewriter;
  std::shared_ptr<HeaderClosureState> _State;
};

/**
 * @brief Emits the declaration and macro closure at the top of the filtered file.
 *
 * Roots are the surviving function bodies plus every kept signature and every
 * other main-file declaration. Rejected functions keep their signatures (see
 * RemoveVisitor), so their parameter types enter the closure even though
 * nothing will call them; that over-inclusion is accepted rather than change
 * filter's contract with transform.
 *
 * Runs last in the filter chain: it needs the reject list that
 * FilterFunctionsConsumer produces.
 */
class HeaderClosureConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer.
   *
   * @param rewriter Shared rewriter the closure block is inserted through.
   * @param toRemove Functions the filter rejected; their bodies are not roots.
   * @param state    Preprocessor-collected state from LocalHeaderPP.
   */
  HeaderClosureConsumer(clang::Rewriter &rewriter,
                        std::shared_ptr<std::vector<std::string>> toRemove,
                        std::shared_ptr<HeaderClosureState> state);

  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  clang::Rewriter &_Rewriter;
  std::shared_ptr<std::vector<std::string>> _ToRemove;
  std::shared_ptr<HeaderClosureState> _State;
};
