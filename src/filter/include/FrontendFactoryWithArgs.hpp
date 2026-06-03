#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/PCHContainerOperations.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Carries pipeline state into Clang's tool runner.
 *
 * Clang's {@code ClangTool::run()} only knows how to call {@code create()} on
 * a {@code FrontendActionFactory}. This subclass stores the config map, desired
 * types, and output stream so that each {@code FilterAction} it creates has
 * everything it needs without those objects being globals.
 */
class FrontendFactoryWithArgs : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the shared pipeline state.
   *
   * @param config  Pointer to the threshold map owned by {@code Filterer}.
   * @param types   Reference to the Clang BuiltinType values for requested types.
   * @param output  Reference to the output stream for the filtered file.
   */
  FrontendFactoryWithArgs(std::map<std::string, int> *config,
                          const std::vector<unsigned int> &types, llvm::raw_fd_ostream &output);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code FilterAction} loaded with the config and output stream.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

private:
  std::map<std::string, int> *_Config;
  const std::vector<unsigned int> &_Types;
  llvm::raw_fd_ostream &_Output;
};
