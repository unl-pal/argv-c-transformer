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
 */

/** @brief One macro defined in a project-local header, captured verbatim. */
struct MacroRecord {
  std::string text; // definition
  unsigned order = 0; // Raw SourceLocation to restore source order
};

/**
 * @brief State handed from the preprocessor callbacks to the closure consumer.
 */
struct HeaderClosureState {
  bool strippedLocalInclude = false; // flag to check if closure is needed
  /// local headers' system includes
  std::set<std::string> systemIncludes;
  /// every macro defined in local headers, keyed by name
  std::map<std::string, MacroRecord> localMacros;
  /// Macros referenced in the main file with their location
  std::vector<std::pair<std::string, clang::SourceLocation>> macroUses;
};

/**
 * @brief PPCallbacks hook feeding the closure: strips local includes, records
 * system includes reachable through them, and captures local macros.
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

  /** @brief Override that captures, during PP, the exact spelling of every macro defined in a local header. */
  void MacroDefined(const clang::Token &MacroNameTok, const clang::MacroDirective *MD) override;

  /** @brief Override that records main-file expansions of local-header macros. */
  void MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &MD,
                    clang::SourceRange Range, const clang::MacroArgs *Args) override;

private:
  clang::SourceManager &_Mgr;
  const clang::LangOptions &_LangOpts;
  clang::Rewriter &_Rewriter;
  std::shared_ptr<HeaderClosureState> _State;
};

/**
 * @brief Emits declarations and macros at the top of the filtered file.
 *
 * Runs last in the filter chain as it needs the reject list that
 * FilterFunctionsConsumer produces.
 */
class HeaderClosureConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer.
   *
   * @param rewriter Shared rewriter the closure block is inserted through.
   * @param toRemove Functions the filter rejected
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
