#include "include/Transformer.hpp"
#include "ClangToolUtils.hpp"
#include <iostream>

/// Main function should be transfered to a driver for use via the full implementation
int main(int argc, char **argv) {
  checkClangVersion();
  if (argc == 2) {
    Transformer transformer(argv[1]);
    transformer.run();
  } else {
    std::cout << "Incorrect Number of Args" << std::endl;
    std::cout << "Please Give the Location of the Configuration File" << std::endl;
    return 1;
  }
  return 0;
}
