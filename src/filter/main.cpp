#include "include/Filter.h"
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
  std::cout << "starting" << std::endl;
  if (argc > 1) {
    std::ifstream file(argv[1]);
    std::stringstream buffer;

    if (file.is_open()) {
      buffer << file.rdbuf();
      file.close();
      const std::string fileContents = buffer.str();

  std::cout << "file open and creating astUnit" << std::endl;
      std::unique_ptr<clang::ASTUnit> astUnit =
        clang::tooling::buildASTFromCode(fileContents, argv[1]);
      if (!astUnit) {
        std::cerr << "Failed to build AST" << std::endl;
        return 0;
      }
  std::cout << "Saving AST and creating visitor" << std::endl;
      astUnit->Save(std::string(argv[1]) + ".ast");
      clang::ASTContext &Context = astUnit->getASTContext();
      CountNodesVisitor visitorA(&Context);
      std::cout << "Diagnostics" << std::endl;
      astUnit->getDiagnostics();
      Context.PrintStats();
  std::cout << "Traversing AST" << std::endl;
      std::cout << visitorA.TraverseAST(Context) << std::endl;
  std::cout << "Printing Report" << std::endl;
      visitorA.PrintReport();
    } else {
      std::cerr << "Error" << std::endl;
    }
  }
  return 1;
}
