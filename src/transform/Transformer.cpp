#include "include/Transformer.hpp"
#include "include/RegenCode.hpp"
#include "include/CreateNewAST.hpp"
#include "include/RemoveUnusedVisitor.hpp"
#include "ReplaceCallsVisitor.hpp"
#include "GenerateIncludeHandler.hpp"
#include "GenerateIncludeAction.hpp"
#include "CodeGeneratorAction.hpp"
#include "ArgsFrontendActionFactory.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <llvm/ADT/StringRef.h>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <vector>

// stream file contents to contents shared pointer or return false if file does not open
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

// Take an individual file and apply all transformations to it by generating 
// the ast, visitors and regenerating the source code as precompiled .i file
// returns false if the AST fails to build
bool Transformer::transformFile(std::filesystem::path path,
                             std::vector<std::string> &args) {
  std::cout << "Transforming: " << path.string() << std::endl;
  if (!std::filesystem::exists(path)) return false;
  std::shared_ptr<std::string> fileContents = std::make_shared<std::string>();
  std::filesystem::path full = std::filesystem::current_path() / path;
  getFileContents(full.string(), fileContents);
  /*std::cout << *fileContents << std::endl;*/

  std::filesystem::path srcPath = std::filesystem::path("benchmark");
  std::filesystem::path preprocessedPath = std::filesystem::path("preprocessed");
  // Keeps people from being able to write files outside of the project folder for now
  // Eliminates the filteredFiles part of the path
  for (const std::filesystem::path &component : path) {
    if (component.string() != "filteredFiles" && component.string() != "..") {
      preprocessedPath /= component;
      srcPath /= component;
    }
  }


  std::unique_ptr<clang::ASTUnit> oldAstUnit =
    clang::tooling::buildASTFromCodeWithArgs(*fileContents, args
                                             // );
                                             , srcPath.string());

  // preprocessedPath.replace_extension(".i");
  std::unique_ptr<clang::ASTUnit> newAstUnit =
    clang::tooling::buildASTFromCodeWithArgs("// Benchmark", args, preprocessedPath.string());
    // clang::tooling::buildASTFromCodeWithArgs(*fileContents, std::vector<std::string>({}),

  if(!oldAstUnit || !newAstUnit) {
    std::cerr << "Failed to Build AST" << std::endl;
    return false;
  }

  clang::ASTContext &oldContext = oldAstUnit->getASTContext();
  clang::ASTContext &newContext = newAstUnit->getASTContext();

  clang::Rewriter R;
  R.setSourceMgr(oldContext.getSourceManager(), oldAstUnit->getLangOpts());
  // TransformerVisitor transformerVisitor(&newContext, &oldContext, R);
  // transformerVisitor.TraverseAST(oldContext);

  clang::Rewriter creatorR;
  creatorR.setSourceMgr(newContext.getSourceManager(), newAstUnit->getLangOpts());

  clang::SourceManagerForFile SMF(preprocessedPath.string(), *fileContents);
  CreateNewAST creator(creatorR, SMF);
  // CreateNewAST creator(creatorR);
  creator.AddVerifiers(&newContext, &oldContext);
  creator.AddBoolDef(&newContext, &oldContext);
  creator.AddAllDecl(&newContext, &oldContext);

  std::cout << "Replace Calls" << std::endl;
  ReplaceDeadCallsVisitor replacer(&newContext, R);
  replacer.TraverseAST(newContext);
  // newContext.getTranslationUnitDecl()->dumpColor();
  
  RemoveUnusedVisitor remover(&newContext);
  std::cout << "Remover Visitor" << std::endl;
  bool done = false;
  remover.TraverseAST(newContext);
  std::cout << "Remover Visitor" << std::endl;
  while (!done) {
    done = remover.TraverseAST(newContext);
  }

  std::cout << "Writing File" << std::endl;

  std::error_code ec;
  std::filesystem::create_directories(preprocessedPath.parent_path());
  llvm::raw_fd_ostream output(llvm::StringRef(preprocessedPath.string()), ec);
  RegenCodeVisitor codeRegenVisitor(&newContext, output);
  codeRegenVisitor.TraverseAST(newContext);

  std::filesystem::create_directories(srcPath.parent_path());
  llvm::raw_fd_ostream srcOutput(llvm::StringRef(srcPath.string()), ec);
  R.setSourceMgr(oldContext.getSourceManager(), newAstUnit->getLangOpts());
  R.InsertTextBefore(oldContext.getTranslationUnitDecl()->getLocation(), "// Benchmark File");
  R.getEditBuffer(newContext.getSourceManager().getFileID(newAstUnit->getStartOfMainFileID())).write(srcOutput);

  static llvm::cl::OptionCategory myToolCategory("my-tool");

  clang::IgnoringDiagConsumer diagConsumer;

  std::vector<std::string> sources;
  sources.push_back(path);

  std::string resourceDir = std::getenv("CLANG_RESOURCES");

  std::vector<std::string> compOptionsArgs({
    "clang",
    path.string(),
    "verifier.c",
    "-extra-arg=-fparse-all-comments",
    "-extra-arg=-resource-dir=" + resourceDir,
    "-extra-arg=-xc"
  });

  int argc = compOptionsArgs.size();

  char** argv = new char*[argc + 1];

  for (int i=0; i<argc; i++) {
    argv[i] = new char[compOptionsArgs[i].length() + 1];
    std::strcpy(argv[i], compOptionsArgs[i].c_str());
  }

  argv[argc] = nullptr;

  if (argv == nullptr) {
    return 1;
  }

  llvm::Expected<clang::tooling::CommonOptionsParser> expectedParser = clang::tooling::CommonOptionsParser::create(argc, (const char**)argv, myToolCategory);

  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return false;
  }

  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  clang::tooling::ClangTool tool(optionsParser.getCompilations(), sources);

  tool.setDiagnosticConsumer(&diagConsumer);

  ArgsFrontendFactory factory(output);
  llvm::outs() << tool.run(&factory) << "\n";
  /*oldAstUnit->Save("output.ast");*/
  return true;
}

// Recursive algorithm for traversing the file structure and searching for 
// relavent c files to transform
//     ideally files will have been filtered but some logic exists to prevent
//     mishaps just incase
// Returns false if any C files failed transformation
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

void Transformer::parseConfig() {
}

// Main function should be transfered to a driver for use via the full implementation
int Transformer::run(std::string filePath) {
  std::filesystem::path path(filePath);
  if (std::filesystem::exists(path)) {
    parseConfig();
    // Set args for AST creation
    std::vector<std::string> args = std::vector<std::string>();
    args.push_back("-fparse-all-comments");
    args.push_back("-resource-dir=" + std::string(std::getenv("CLANG_RESOURCES")));
    // run the transformer on the file structure
    if (transformAll(path, args)) {
      return 0;
    }
  }
  return 1;
}
