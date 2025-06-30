#include "include/Filterer.hpp"
#include <iostream>

// Target for calling the Filterer Individually
int main(int argc, char** argv) {
  if (argc == 2) {
    Filterer filter(argv[1]);
    filter.run();
  } else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the File or Directory to Filter "
               "and the Location of the Configuration File\n"
               "Example: `./build/filter <config-file>`"
            << std::endl;
  return 1;
  }
  return 0;
}
