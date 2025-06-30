#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

class ArgsFrontendFactory : public clang::tooling::FrontendActionFactory {
public:
  /// Wrapper for the FrontendFactory that allows the creation of a custom
  /// facotry that utilizes the necessary variables to run the
  /// transformationAction code
  /// @param - output stream that all code will be printed to
  ArgsFrontendFactory(llvm::raw_fd_ostream &output);

  /// Called by clangs's tool to create the action - must be overridden
  std::unique_ptr<clang::FrontendAction> create() override;

  /// Initializes the Action
  /// @param - Invocation the compiler invocation used to create AST
  /// @param - Files included in the AST
  /// @param - PCHContainerOps precompiled header options if needed
  /// @param - DiagConsumer location that the diagnostic messages are printed
  /// to. This is used to keep the user output from becoming cluttered with
  /// compiler errors in the code being filtered and transmormed by sending the
  /// errors to the abyss. The number of errors can still be tracked
  bool runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) override;

private:
  llvm::raw_fd_ostream &_Output;
};
