#include "include/Action.hpp"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>

// TODO - LEARN
// This feels so random and arbitrary, what is this dictating
static llvm::cl::OptionCategory MyToolCategory("my-tool");

// TODO - LEARN
// main is currently set to take advantage of the commandline argumensts so that
// this tool can be run as a commandline tool as a stand alone. But where are
// the files being kept and are they still a sub part of the clang repo just as
// a persons local tool?
//
// Does clang have to be rebuilt for this tool to work or can we build alongside 
// using as a dependency?
//
// Many of the guides show how to create these cmd tools as sub parts of the clang/llvm repo. 
//
// Would doing this method help or hinder us? 
//
// What are the pros and cons of this process?
//
int main(int argc, const char** argv) {
  // AddVerifiersTool tool;

  // TODO - LEARN
  // Set up the sources to be run, are these run together or individually?
  std::vector<std::string> Sources;
  Sources.push_back("samples/full.c");

  std::cout << "didn't even try the const array" << std::endl;
  // char* dirArg = "--extra-arg=-resource-dir=";
  char* resourceDir = std::getenv("CLANG_RESOURCES");
  // std::cout << "didn't even try the const array" << std::endl;
  // char* tempDir = std::strcat(dirArg, resourceDir);
  // std::cout << "didn't even try the const array" << std::endl;
  //
  // char* otherArgV[] = {
  //   "samples/full.c",
  //   "--extra-arg=-fparse-all-comments",
  //   tempDir
  // };

  std::string extraArgs = " --extra-arg=-fparse-all-comments --extra-arg=\"-resource-dir ";
  std::string resources = (std::string)(resourceDir);
  // std::string tempS = extraArgs + resources + "\"";
  std::string tempS = "";
  std::cout << "Made Strings" << std::endl;
  std::cout << "Got length" << std::endl;
  char** tempChar = (char**)("samples/full.c");
  for (char c : tempS) {
    std::cout << c;
    tempChar += c;
  }
  std::cout << std::endl;
  const char **myV = (const char**)(tempChar);


  if (myV != nullptr) {
    std::cout << "Char** is not null" << std::endl;
  } else {
    std::cout << "Failed to Create the argv" << std::endl;
  }
  int myC = 1;

  std::cout << "the const array has been created" << std::endl;

  // Aguments for this can be preset rather than from commandline
  // -p command specifies build path
  // automatic location for compilation database using source file paths
  llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);
  // llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(myC, myV, MyToolCategory);

  if (!ExpectedParser) {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  std::cout << "Expected Parser Made" << std::endl;

  // this gets the actual parser from the expected which is done to handle the
  // chance of failure more gracefully I believe and should probably still be
  // checked at somepoint in the process
  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();

  // OptionsParser.getCompilations() to retrieve CompilationDatabase
  // OptionParser.getSourcePathList() to list input files

  // Give compilations and sources to the tool
  // TODO - LEARN
  // the sources that I have can be gathered in a vector from the directories
  // and used here I believe but will that attempt to do them all in the same
  // compilation or will it run them individially? creating a number of sources
  // Can the Sources be run together if so desired?
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), Sources);

  // Running of the actual tool, what all is this doing behind the scenes and
  // when are all the parts actually generated?
  llvm::outs() << Tool.run(
    clang::tooling::newFrontendActionFactory<Action>().get()) << "\n";

  // Result is: 
  //   0 - Success
  //   1 - Error
  //   2 - Some Files Are Skipped Due to Missing Compiler Commands
  llvm::outs() << Tool.run(
    clang::tooling::newFrontendActionFactory<Action>().get()) << "\n";

  return 0;
}
