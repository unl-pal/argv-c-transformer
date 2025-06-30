#include "FrontendFactoryWithArgs.hpp"
#include "FilterAction.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <memory>

// Constructor for our front end that allows for the output stream variable to
// be passed to all consumers and actions to come
FrontendFactoryWithArgs::FrontendFactoryWithArgs(
  std::map<std::string, int> *config,
  const std::vector<unsigned int> &types,
  llvm::raw_fd_ostream &output)
    : _Config(config), _Types(types), _Output(output) {}

// overridden create method necessary to out put our frontend action
std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  llvm::outs() << "Created Rewriter" << "\n";
  return std::make_unique<FilterAction>(_Config, _Types, _Output);
}

bool FrontendFactoryWithArgs::runInvocation(std::shared_ptr<clang::CompilerInvocation> Invocation, clang::FileManager *Files, std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps, clang::DiagnosticConsumer *DiagConsumer) {
  llvm::outs() << "Running Invocation\n";

  // return true;
  return clang::tooling::FrontendActionFactory::runInvocation(Invocation, Files, PCHContainerOps, DiagConsumer);
}
