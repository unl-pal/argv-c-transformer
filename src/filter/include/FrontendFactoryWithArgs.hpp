#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <vector>

/// Wrapper for Frontend Action that allows us to create a factory with the
/// attributes that we need for the tool's consumers to run
class FrontendFactoryWithArgs : public clang::tooling::FrontendActionFactory {
public:
  /// Constructor for the Action taking in a config file, desired types and
  /// output stream as arguments to be used by the tool
  FrontendFactoryWithArgs( std::map<std::string, int>    *_Config,
                          const std::vector<unsigned int> &_Types,
                          llvm::raw_fd_ostream &output);

  /// Function called by clang's tool to create the action
  std::unique_ptr<clang::FrontendAction> create() override;

  /// Function called by clang's tool to initiate the action
  bool runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) override;

private:
  clang::Rewriter _Rewriter;
  std::map<std::string, int> *_Config;
  const std::vector<unsigned int> &_Types;
  llvm::raw_fd_ostream &_Output;
};
