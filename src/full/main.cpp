// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "Filterer.hpp"
#include "Transformer.hpp"
#include "ClangToolUtils.hpp"
#include <iostream>

int main(int argc, char **argv) {
  checkClangVersion();
  if (argc == 2) {
    Filterer filter(argv[1]);
    filter.run();

    Transformer transformer(argv[1]);
    transformer.run();
  } else {
    std::cout << "Incorrect Number of Args" << std::endl;
    std::cout << "Please Give the Location of the Configuration File\n"
                 "    Example: `./build/full <config-file>`"
              << std::endl;
    return 1;
  }
  return 0;
}
