#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

/**
 * @brief Carries the output stream into Clang's tool runner.
 *
 * Clang's {@code ClangTool::run()} only knows how to call {@code create()} on
 * a {@code FrontendActionFactory}. This subclass stores the output stream so
 * that each {@code TransformAction} it creates can write the rewritten source
 * without that stream being a global.
 */
class ArgsFrontendFactory : public clang::tooling::FrontendActionFactory {
public:
  /**
   * @brief Constructs the factory, binding the output stream.
   *
   * @param output Reference to the output stream for the transformed file.
   */
  ArgsFrontendFactory(llvm::raw_fd_ostream &output);

  /**
   * @brief Called by {@code ClangTool} once per source file to create the action.
   *
   * Returns a new {@code TransformAction} loaded with the output stream.
   *
   * @return Owning pointer to the created action.
   */
  std::unique_ptr<clang::FrontendAction> create() override;

  /**
   * @brief Runs the invocation, logging before delegating to the base implementation.
   *
   * @param Invocation     The compiler invocation used to build the AST.
   * @param Files          File manager for the files included in the AST.
   * @param PCHContainerOps Precompiled header options, if needed.
   * @param DiagConsumer   Destination for diagnostic messages. Used to keep
   *                        user output free of compiler errors from the code
   *                        being transformed; error counts are still tracked.
   * @return true if the invocation succeeded.
   */
  bool runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation,
                     clang::FileManager *Files,
                     std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps,
                     clang::DiagnosticConsumer *DiagConsumer) override;

private:
  llvm::raw_fd_ostream &_Output;
};
