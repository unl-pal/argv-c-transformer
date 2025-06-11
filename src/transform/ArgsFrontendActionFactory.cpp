#include "ArgsFrontendActionFactory.hpp"
#include <GenerateIncludeAction.hpp>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

ArgsFrontendFactory::ArgsFrontendFactory(llvm::raw_fd_ostream &output)
    : _Output(output) {}

std::unique_ptr<clang::FrontendAction> ArgsFrontendFactory::create() {
  return std::make_unique<GenerateIncludeAction>(_Output); // should this be a consumer instead??
}
