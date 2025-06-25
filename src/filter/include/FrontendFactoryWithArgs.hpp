#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <vector>

class FrontendFactoryWithArgs : public clang::tooling::FrontendActionFactory {
public:
  FrontendFactoryWithArgs( std::map<std::string, int>    *_Config,
                          const std::vector<unsigned int> &_Types,
                          llvm::raw_fd_ostream &output);

  std::unique_ptr<clang::FrontendAction> create() override;

  bool runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) override;

private:
  clang::Rewriter _Rewriter;
  std::map<std::string, int> *_Config;
  const std::vector<unsigned int> &_Types;
  llvm::raw_fd_ostream &_Output;
};
