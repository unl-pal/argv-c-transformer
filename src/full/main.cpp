#include "Filterer.hpp"
#include "Transformer.hpp"
#include <iostream>

// TODO OVER HAUL THIS WHOLE THING TO MAKE SENSE AGAIN
int main(int argc, char** argv) {
	if (argc == 4) {
    Filterer filter(argv[1]);
    filter.run();

    Transformer transformer(argv[1]);
    transformer.run();
  } else if (argc > 1) {
    Filterer filter(argv[1]);
    filter.run();

    Transformer transformer(argv[1]);
    transformer.run();
	} else {
  std::cout << "Incorrect Number of Args" << std::endl;
  std::cout << "Please Give the Location of the Configuration File\n"
      "    Example: `<filter-directory> <config-file> <Resource-Dir>`"
            << std::endl;
  return 1;
  }
	return 0;
}
