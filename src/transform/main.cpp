#include "include/pretty_print_visitor.hpp"
#include "include/ReGenCode.h"

#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <iostream>
#include <fstream>
#include <filesystem>


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
      /*  PrintASTVisitor visitorA(&Context);*/
      /*std::cout << "-------------- NEXT STEP --------------" << std::endl;*/
      /*visitorA.TraverseAST(Context);*/

      std::filesystem::create_directory("preprocessed");
      std::ofstream newFile(std::string(argv[1]) + ".i");
      /*std::FILE tmp(std::string(argv[1]) + ".i");*/
      std::error_code ec;
      /*auto dir = llvm::sys::fs::create_directory(llvm::Twine("preprocessed"));*/
      /*auto file = llvm::sys::fs::createUniqueFile("preprocessed" + llvm::StringRef(argv[1]) + ".i");*/
      /*llvm::raw_fd_ostream output(llvm::StringRef(std::string(argv[1]) + ".i"), ec, llvm::sys::fs::CreationDisposition::CD_CreateNew);*/
      llvm::raw_fd_ostream output(llvm::StringRef(std::string(argv[1]) + ".i"), ec);
      ReGenCodeVisitor visitorD(&Context, output);
      visitorD.TraverseAST(Context);

    } else {
      std::cerr << "Error" << std::endl;
    }
  }
  return 1;
}
