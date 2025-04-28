#include "Filterer.hpp"
#include "Transformer.hpp"
#include <iostream>

int main(int argc, char** argv) {
	if (argc == 4) {
    Filterer filter;
    filter.run(argv[1], argv[2], argv[3]);

    Transformer transformer;
    transformer.run(argv[1]);
  } else if (argc > 1) {
    Filterer filter;
    filter.run(argv[1]);

    Transformer transformer;
    transformer.run(argv[1]);
	} else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the:\n"
      "File or Directory to Filter\n"
      "Location of the Clang Resource Directory\n"
      "    This can be found by running 'clang -print-resource-dir'\n"
      "Location of the Configuration File\n"
      "    Example: `<filter-directory> <config-file> <Resource-Dir>`"
            << std::endl;
  return 1;
  }
	return 0;
}
