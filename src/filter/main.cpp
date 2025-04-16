#include "include/Filterer.hpp"
#include <iostream>

// Target for calling the Filterer Individually
int main(int argc, char** argv) {
  if (argc == 4) {
    Filterer filter;
    filter.run(argc, argv);

  } else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the File or Directory to Filter "
               "and the Location of the Configuration File\n"
               "and the Location of the Clang Resource Directory\n"
               "    This can be found by running 'clang -print-resource-dir'\n"
               "Example: `<filter-directory> <config-file> <Resource-Dir>`"
            << std::endl;
  return 1;
  }
}
