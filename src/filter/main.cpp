#include "include/Filter.h"

#include <boost/utility/result_of.hpp>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <llvm/ADT/SmallString.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> getPathDirectories() {
  std::vector<std::string> directories;
  const char* pathEnv = std::getenv("PATH");
  if (pathEnv != nullptr) {
    std::string pathString(pathEnv);
    std::stringstream ss(pathString);
    std::string token;
    char delimiter = ':';
#ifdef _WIN32
    delimiter = ';';
#endif
    while (std::getline(ss, token, delimiter)) {
      directories.push_back(token);
    }
  }
  return directories;
}

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
      std::vector<std::string> args = std::vector<std::string>();
      /*args.push_back("-v");*/
      std::vector<std::string> paths = getPathDirectories();
      for (const std::string &dir : paths) {
        args.push_back("-I" + dir);
      }
      std::cout << fileContents << std::endl;
      std::unique_ptr<clang::ASTUnit> astUnit =
        clang::tooling::buildASTFromCodeWithArgs(fileContents, args, argv[1]);
        /*clang::tooling::buildASTFromCode(fileContents, argv[1]);*/
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
