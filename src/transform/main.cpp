#include "include/Transformer.hpp"
#include <iostream>

/// Main function should be transfered to a driver for use via the full implementation
int main(int argc, char** argv) {
  if (argc == 2) {
    Transformer transformer;
    transformer.run(argv[1]);
  } else {
    std::cout << "Incorrect Number of Args" << std::endl;
    std::cout << "Please Give the Location of the File or Directory to Transform";
  }
  return 1;
}
