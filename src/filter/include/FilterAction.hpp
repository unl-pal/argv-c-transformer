#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief ASTFrontendAction that runs the full filter consumer chain.
 *
 * Wires together the three consumers (count → filter → strip bodies) over a
 * single parsed AST. A shared {@code Rewriter} accumulates all edits; the
 * final buffer is flushed to the output file in {@code EndSourceFileAction}.
 */
class FilterAction : public clang::ASTFrontendAction {
public:
  /**
   * @brief Constructs the action with the shared pipeline state.
   *
   * @param config  Threshold map owned by {@code Filterer}.
   * @param types   Clang BuiltinType values for the requested verifier types.
   * @param output  Destination file stream for the filtered output.
   */
  FilterAction(std::map<std::string, int> *config, const std::vector<unsigned int> &types,
               llvm::raw_fd_ostream &output);

  /**
   * @brief Builds a {@code MultiplexConsumer} containing all three filter passes.
   *
   * Creates the shared state ({@code toFilter}, {@code toRemove}) as
   * {@code shared_ptr}s and hands them to the consumers in pipeline order;
   * each is freed once the last owning consumer is destroyed.
   *
   * @param compiler  The active compiler instance.
   * @param filename  Path of the file being processed.
   * @return Owning pointer to the multiplexed consumer.
   */
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler,
                                                        llvm::StringRef filename) override;

  /**
   * @brief Attaches the {@code Rewriter} to the compiler before AST actions run.
   *
   * Must be called before any consumer can make edits, because the Rewriter
   * needs the {@code SourceManager} and {@code LangOptions} that only exist
   * once the compiler instance is fully set up.
   *
   * @param compiler  The active compiler instance.
   * @return Result of the parent implementation.
   */
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override;

  /**
   * @brief Flushes the Rewriter's edited buffer to the output file.
   *
   * Called after all consumers have finished. Writes the modified source
   * text, with the bodies of filtered-out functions stripped, to the
   * destination stream.
   */
  void EndSourceFileAction() override;

private:
  std::map<std::string, int> *_Config;
  const std::vector<unsigned int> &_Types;
  clang::Rewriter _Rewriter;
  llvm::raw_fd_ostream &_Output;
};
