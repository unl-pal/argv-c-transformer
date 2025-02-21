  #include "include/pretty_print_visitor.hpp"

#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <iostream>
#include <fstream>


int main(int argc, char** argv) {
  if (argc > 1) {
      std::ifstream file(argv[1]);
      std::stringstream buffer;

      if (file.is_open()) {
      buffer << file.rdbuf();
      file.close();
      const std::string fileContents = buffer.str();
      std::cout << fileContents << std::endl;

      std::unique_ptr<clang::ASTUnit> astUnit =
        clang::tooling::buildASTFromCode(fileContents, argv[1]);
      astUnit->Save("output.ast");

      if(!astUnit) {
        std::cerr << "Failed to Build AST" << std::endl;
        return 1;
      }

      clang::ASTContext &Context = astUnit->getASTContext();
        PrintASTVisitor visitorA(&Context);
      std::cout << "-------------- NEXT STEP --------------" << std::endl;
      visitorA.TraverseAST(Context);
    } else {
      std::cerr << "Error" << std::endl;
    }
  }
  return 1;
}
