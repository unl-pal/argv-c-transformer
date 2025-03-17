#include "include/Transform.hpp"
#include "include/pretty_print_visitor.hpp"
#include "include/ReGenCode.h"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <string>
#include <vector>

/// stream file contents to contents shared pointer or return false if file does not open
bool getFileContents(std::string fileName, std::shared_ptr<std::string> contents) {
  std::ifstream file(fileName);
  std::stringstream buffer;
  if (file.is_open()) {
    buffer << file.rdbuf();
    file.close();
    *contents = buffer.str();
    return true;
  } else {
    std::cerr << "File Failed to Open" << std::endl;
    return false;
  }
}

/// Take an individual file and apply all transformations to it by generating 
/// the ast, visitors and regenerating the source code as precompiled .i file
bool transformFile(std::filesystem::path path, std::vector<std::string> &args) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path)) return false;
  std::shared_ptr<std::string> fileContents = std::make_shared<std::string>();
  std::filesystem::path full = std::filesystem::current_path() / path;
  getFileContents((full).string(), fileContents);
  /*std::cout << *fileContents << std::endl;*/
  std::unique_ptr<clang::ASTUnit> astUnit = clang::tooling::buildASTFromCodeWithArgs(*fileContents, args, path.string());
  if(!astUnit) {
    std::cerr << "Failed to Build AST" << std::endl;
    return false;
  }

  clang::ASTContext &Context = astUnit->getASTContext();

  clang::Rewriter R;
  R.setSourceMgr(Context.getSourceManager(), astUnit->getLangOpts());
  TransformerVisitor transformerVisitor(&Context, R);
  transformerVisitor.TraverseAST(Context);

  std::filesystem::path prePath = std::filesystem::path("preprocessed");
  for (const std::filesystem::path &component : path) {
    if (component.string() != path.begin()->string() && component.string() != "..") {
      prePath /= component;
    }
  }
  prePath.replace_extension(".i");
  std::error_code ec;
  std::filesystem::create_directories(prePath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(prePath.string()), ec);
  ReGenCodeVisitor codeReGenVisitor(&Context, output);
  codeReGenVisitor.TraverseAST(Context);
  /*astUnit->Save("output.ast");*/
  return true;
}

/// Recursive algorithm for traversing the file structure and searching for 
/// relavent c files to transform
///     ideally files will have been filtered but some logic exists to prevent
///     mishaps just incase
bool transformAll(std::filesystem::path path, std::vector<std::string> &args) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      bool result = true;
      for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(path)) {
        /*std::cout << "Dir " << std::endl;*/
        result &= transformAll(entry.path(), args);
      }
      return result;
    } else if (std::filesystem::is_regular_file(path)) {
      if (path.has_extension() && path.extension() == ".c") {
        /*std::cout << "File " << std::endl;*/
        return transformFile(path, args);
      }
    } else {
      return false;
    }
  }
  return false;
}

/// checks the users path for path to libc files for ast generation
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

/// Main function should be transfered to a driver for use via the full implementation
int main(int argc, char** argv) {
  if (argc == 2) {
    std::filesystem::path path(argv[1]);
    if (std::filesystem::exists(path)) {
      /// Set args for AST creation
      std::vector<std::string> args = std::vector<std::string>();
      std::vector<std::string> paths = getPathDirectories();
      for (const std::string &dir : paths) {
        args.push_back("-I" + dir);
      }
      /// run the transformer on the file structure
      transformAll(path, args);
    }
  }
  return 1;
}
