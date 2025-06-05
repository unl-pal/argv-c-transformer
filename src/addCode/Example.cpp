#include "include/Example.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <string>

bool Visitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  D->dump();
  return true;
}

bool Visitor::VisitDecl(clang::Decl *D) {
  D->dump();
  return true;
}

bool Visitor::WalkUpFromDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<Visitor>::WalkUpFromDecl(D);
}

bool Visitor::TraverseDecl(clang::Decl *D) {
  return clang::RecursiveASTVisitor<Visitor>::TraverseDecl(D);
}

bool Visitor::VisitStmt(clang::Stmt *S) {
  S->dump();
  return true;
}

bool Visitor::shouldTraversePostOrder() {
  return true;
}

std::unique_ptr<clang::ASTConsumer>
Action::CreateASTConsumer(clang::CompilerInstance &Compiler,
                          llvm::StringRef          Filename) {

  return std::make_unique<ConsumerVisitor>();
}

bool Action::BeginSourceFileAction(clang::CompilerInstance &Compiler) {
  llvm::outs() << "Begin Source File Action\n";
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(Compiler);
  llvm::outs() << "Ran - Begin Source File Action\n";
  return result;
}

void Action::EndSourceFileAction() {
  clang::ASTFrontendAction::EndSourceFileAction();
  llvm::outs() << "End Source File Action";
}

void ConsumerVisitor::HandleTranslationUnit(clang::ASTContext &Context) {
  Visitor.HandleTranslationUnit(Context.getTranslationUnitDecl());
}

void
Handler::run(const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  const clang::FunctionDecl *Function =
    Result.Nodes.getNodeAs<clang::FunctionDecl>("root");
  Function->dump();
}

void ConsumerMatcher::HandleTranslationUnit(
  clang::ASTContext &Context) {
  clang::ast_matchers::MatchFinder MatchFinder;
  Handler                          Handler;
  MatchFinder.addMatcher(clang::ast_matchers::functionDecl(), &Handler);
  MatchFinder.matchAST(Context);
}

// class MyMatchFinder : public clang::ast_matchers::internal::ASTMatchFinder {
// public:
//   MyMatchFinder(clang::ast_matchers::MatchFinder Finder);
//   void addMatcher();
// };

// TODO - LEARN
// This feels so random and arbitrary, what is this dictating
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

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

  // Aguments for this can be preset rather than from commandline
  // -p command specifies build path
  // automatic location for compilation database using source file paths
  llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);

  if (!ExpectedParser) {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  // this gets the actual parser from the expected which is done to handle the
  // chance of failure more gracefully I believe and should probably still be
  // checked at somepoint in the process
  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();

  // OptionsParser.getCompilations() to retrieve CompilationDatabase
  // OptionParser.getSourcePathList() to list input files

  // TODO - LEARN
  // Set up the sources to be run, are these run together or individually?
  std::vector<std::string> Sources;
  Sources.push_back("samples/full.c");

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
