#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

class ArgsFrontendFactory : public clang::tooling::FrontendActionFactory {
public:
  ArgsFrontendFactory(llvm::raw_fd_ostream &output);

  std::unique_ptr<clang::FrontendAction> create() override;

  bool runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) override;

private:
  llvm::raw_fd_ostream &_Output;
};
