#include "include/Filterer.hpp"
#include <iostream>

/// Target for calling the Filterer Individually
int main(int argc, char** argv) {
  if (argc == 3) {
    Filterer filter;
    filter.run(argc, argv);

  } else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the File or Directory to Filter "
               "and the Location of the Configuration File\n"
               "Example: `<filter> <directory> <config-file>`"
            << std::endl;
  return 1;
  }
}
