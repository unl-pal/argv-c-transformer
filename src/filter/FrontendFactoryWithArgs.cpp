#include "FrontendFactoryWithArgs.hpp"
#include "FilterAction.hpp"

#include <clang/Frontend/FrontendAction.h>
#include <memory>

FrontendFactoryWithArgs::FrontendFactoryWithArgs(std::map<std::string, int> *config,
                                                 const std::vector<unsigned int> &types,
                                                 llvm::raw_fd_ostream &output)
    : _Config(config), _Types(types), _Output(output) {}

std::unique_ptr<clang::FrontendAction> FrontendFactoryWithArgs::create() {
  return std::make_unique<FilterAction>(_Config, _Types, _Output);
}
