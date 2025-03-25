#include "include/Transformer.hpp"
#include "include/ReGenCode.h"
#include "include/Transform.hpp"

#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <iostream>
#include <fstream>
#include <memory>

/// stream file contents to contents shared pointer or return false if file does not open
bool Transformer::getFileContents(std::string fileName,
                               std::shared_ptr<std::string> contents) {
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
/// returns false if the AST fails to build
bool Transformer::transformFile(std::filesystem::path path,
                             std::vector<std::string> &args) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path)) return false;
  std::shared_ptr<std::string> fileContents = std::make_shared<std::string>();
  std::filesystem::path full = std::filesystem::current_path() / path;
  getFileContents((full).string(), fileContents);
  /*std::cout << *fileContents << std::endl;*/

  std::filesystem::path prePath = std::filesystem::path("preprocessed");
  for (const std::filesystem::path &component : path) {
    if (component.string() != path.begin()->string() && component.string() != "..") {
      prePath /= component;
    }
  }
  std::unique_ptr<clang::ASTUnit> oldAstUnit =
      clang::tooling::buildASTFromCodeWithArgs(*fileContents, args,
                                               prePath.string());
                                               // path.string());
  prePath.replace_extension(".i");
  std::unique_ptr<clang::ASTUnit> newAstUnit =
      clang::tooling::buildASTFromCodeWithArgs("", std::vector<std::string>({}),
                                               prePath.string());

  if(!oldAstUnit || !newAstUnit) {
    std::cerr << "Failed to Build AST" << std::endl;
    return false;
  }


  clang::ASTContext &oldContext = oldAstUnit->getASTContext();
  clang::ASTContext &newContext = newAstUnit->getASTContext();

  clang::Rewriter R;
  R.setSourceMgr(oldAstUnit->getSourceManager(), oldAstUnit->getLangOpts());
  // R.setSourceMgr(newAstUnit->getSourceManager(), newAstUnit->getLangOpts());
  TransformerVisitor transformerVisitor(&newContext, &oldContext, R);
  transformerVisitor.TraverseAST(oldContext);

  std::filesystem::create_directories(prePath.parent_path());
  // newContext.getTranslationUnitDecl()->dumpColor();
  std::cout << "Writing File" << std::endl;
  // R.setSourceMgr(newContext.getSourceManager(), newAstUnit->getLangOpts());
  // R.setSourceMgr(newContext.getSourceManager(), newAstUnit->getLangOpts());
  // R.overwriteChangedFiles();
  std::cout << "Wrotteded it" << std::endl;


  prePath.replace_extension(".i");
  std::error_code ec;
  std::filesystem::create_directories(prePath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(prePath.string()), ec);
  ReGenCodeVisitor codeReGenVisitor(&newContext, output);
  codeReGenVisitor.TraverseAST(newContext);

  /*oldAstUnit->Save("output.ast");*/
  return true;
}

/// Recursive algorithm for traversing the file structure and searching for 
/// relavent c files to transform
///     ideally files will have been filtered but some logic exists to prevent
///     mishaps just incase
/// Returns false if any C files failed transformation
bool Transformer::transformAll(std::filesystem::path path,
                            std::vector<std::string> &args) {
  if (std::filesystem::exists(path)) {
    if (std::filesystem::is_directory(path)) {
      for (const std::filesystem::directory_entry &entry :
           std::filesystem::directory_iterator(path)) {
        /*std::cout << "Dir " << std::endl;*/
        transformAll(entry.path(), args);
      }
    } else if (std::filesystem::is_regular_file(path)) {
      if (path.has_extension() && path.extension() == ".c") {
        /*std::cout << "File " << std::endl;*/
        return transformFile(path, args);
      }
    }
  }
  return true;
}

/// checks the users path for path to libc files for ast generation
std::vector<std::string> Transformer::getPathDirectories() {
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

void Transformer::parseConfig() {
}

/// Main function should be transfered to a driver for use via the full implementation
int Transformer::run(std::string filePath) {
  std::filesystem::path path(filePath);
  if (std::filesystem::exists(path)) {
    parseConfig();
    /// Set args for AST creation
    std::vector<std::string> args = std::vector<std::string>();
    std::vector<std::string> paths = getPathDirectories();
    for (const std::string &dir : paths) {
      args.push_back("-I" + dir);
    }
    /// run the transformer on the file structure
    if (transformAll(path, args)) {
      return 0;
    }
  }
  return 1;
}
