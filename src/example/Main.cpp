#include "include/Action.hpp"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

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
// Many of the guides show how to create these cmd tools as sub parts of the
// clang/llvm repo.
//
// Would doing this method help or hinder us?
//
// What are the pros and cons of this process?
//
int main() {
  // TODO - LEARN
  // Set up the sources to be run, are these run together or individually?
  std::vector<std::string> Sources;
  Sources.push_back("samples/full.c");

  std::cout << "didn't even try the const array" << std::endl;
  std::string resourceDir = std::getenv("CLANG_RESOURCES");
  std::vector<std::string> args({
    "clang",
    "samples/", // I am passing a whole Dir, is that an issue or slow?
    "-extra-arg=-fparse-all-comments",
    "-extra-arg=-resource-dir=" + resourceDir,
    "-extra-arg=-xc"
  });

  int argc = args.size();
  char** argv = new char*[argc + 1];

  for (int i=0; i<argc; ++i) {
    argv[i] = new char[args[i].length() + 1];
    std::strcpy(argv[i], args[i].c_str());
    std::cout << "new argv[" << i << "] = " << argv[i] << std::endl;
  }

  argv[argc] = nullptr;

  if (argv != nullptr) {
    std::cout << "Char** is not null" << std::endl;
  } else {
    std::cout << "Failed to Create the argv" << std::endl;
  }

  std::cout << "the const array has been created" << std::endl;

  // Aguments for this can be preset rather than from commandline
  // -p command specifies build path
  // automatic location for compilation database using source file paths
  // llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);
  llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, (const char**)(argv), MyToolCategory);

  if (!ExpectedParser) {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  std::cout << "Expected Parser Made" << std::endl;

  // this gets the actual parser from the expected which is done to handle the
  // chance of failure more gracefully I believe and should probably still be
  // checked at somepoint in the process
  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();

  // ATTEMPT TO UNDERSTAND THE COMMAND USED FOR THE PARSER
  // LOTS OF INFERENCES AND MISSING PARTS
  // clang::tooling::CompilationDatabase &Database = OptionsParser.getCompilations();
  // int size = Database.getCompileCommands("samples/full.c").size();
  // for (int i=0; i<size; i++) {
  //   std::cout << *Database.getCompileCommands("samples/full.c").data()->CommandLine.data() << std::endl;
  //   std::cout << Database.getCompileCommands("samples/full.c").data()->Directory.data() << std::endl;
  //   std::cout << Database.getCompileCommands("samples/full.c").data()->Filename.data() << std::endl;
  //   std::cout << Database.getCompileCommands("samples/full.c").data()->Heuristic.data() << std::endl;
  //   std::cout << Database.getCompileCommands("samples/full.c").data()->Output.data() << std::endl;
  // }

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
  //   UNCOMMENT FOR EXAMPLE RUN
  // llvm::outs() << Tool.run(
  //   clang::tooling::newFrontendActionFactory<Action>().get()) << "\n";

  // Result is: 
  //   0 - Success
  //   1 - Error
  //   2 - Some Files Are Skipped Due to Missing Compiler Commands
  //   UNCOMMENT FOR EXAMPLE RUN
  // llvm::outs() << Tool.run(
    // clang::tooling::newFrontendActionFactory<Action>().get()) << "\n";

  return 0;
}
