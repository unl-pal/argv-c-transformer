#include "include/IfRewrite.hpp"
#include "include/MyRewrite.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/raw_ostream.h>

#include <fmt/format.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\\nMore help text...\\n");

void colorPrint(std::string stmt) {
  std::cout << "\033[33m" << fmt::format("{}", stmt) << "\033[37m" << std::endl;
}

int main(int argc, const char **argv) {
  colorPrint("Before All");
  clang::Expected<clang::tooling::CommonOptionsParser> ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) { 
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  colorPrint("After Expected Parser Before Parser");
  clang::tooling::CommonOptionsParser& OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  /*Tool.getFiles().getFile(StringRef Filename);*/
  std::vector<std::unique_ptr<clang::ASTUnit>> asts;
  /*clang::ASTUnit::getSema();*/
  /*auto nowWhat = clang::ASTUnit::LoadASTOnly;*/
  /*auto view = clang::frontend::ASTView;*/
  /*clang::ASTReader reader;*/
  /*clang::ASTWriter write;*/
  /*std::unique_ptr<clang::ASTUnit> ast = clang::ASTUnit::LoadFromCommandLine(clang::ASTUnit::WhatToLoad::LoadASTOnly);*/

  /*colorPrint("Building ASTs with Tool");*/
  /*Tool.buildASTs(asts);*/

  /*unit->LoadFromASTFile(ovconst std::string &Fileno_ame, const PCHContainerReader &PCHContainerRdr, WhatToLoad ToLoad, IntrusiveRefCntPtr<DiagnosticsEngine> Diags, const FileSystemOptions &FileSystemOpts, std::shared_ptr<HeaderSearchOptions> HSOpts)*/
  /*unit = unit->LoadFromCommandLine(const char **ArgBegin, const char **ArgEnd, std::shared_ptr<PCHContainerOperations> PCHContainerOps, IntrusiveRefCntPtr<DiagnosticsEngine> Diags, StringRef ResourceFilesPath)*/
  /*auto reader = unit->getASTReader();*/
  /*auto newReader = reader.get();*/
  /*unit->LoadASTOnly;*/
  /*clang::ASTUnit::WhatToLoad preOnly = unit->LoadPreprocessorOnly;*/
  /*clang::ASTUnit::WhatToLoad everything = unit->LoadEverything;*/
  /*OptionsParser.getCompilations().getAllFiles()*/

  std::vector<std::pair<std::string, clang::Twine>> code = std::vector<std::pair<std::string, clang::Twine>>();
  std::vector<std::string> pathList = OptionsParser.getSourcePathList();
  for (std::string& fileName : pathList) {
    std::string path = std::filesystem::current_path().string() + "/" + fileName;
    colorPrint(path);
    std::ifstream file(path);
    std::stringstream buffer;
    if (file.is_open()) {
      buffer << file.rdbuf();
      file.close();
      const std::string fileContents = buffer.str();

      colorPrint("Before Build From Code");
      std::unique_ptr<clang::ASTUnit> astUnit =
        clang::tooling::buildASTFromCode(fileContents, fileName);
      colorPrint("After Build From Code");

      if(!astUnit) {
        std::cerr << "Failed to Build AST" << std::endl;
        return 1;
      }

      if (astUnit->Save(fileName + ".ast")) {
        colorPrint("Failed to Save");
      }

      /*astUnit->getBufferForFile(StringRef Filename);*/
      /*astUnit->isMainFileAST();*/

      clang::Rewriter Rewrite;
      Rewrite.setSourceMgr(astUnit->getSourceManager(), astUnit->getLangOpts());

      clang::ASTContext &Context = astUnit->getASTContext();
      Context.PrintStats();
      colorPrint("\nMatches: ");
      /*clang::MangleContext mangle = Context.createMangleContext();*/
      IfRewriteVisitor visitorA(Rewrite, &Context);
      /*return visitorA.TraverseIfStmt(Context.get);*/
      /*visitorA.TraverseDecl(Context.getTranslationUnitDecl());*/
      if (int reason = !visitorA.TraverseAST(Context)) {
        colorPrint("returned early on fail: " + std::to_string(reason));
      } else {
        Rewrite.overwriteChangedFiles();
      }
    } else {
      colorPrint("Error");
    }
  }
  /*Rewrite.setSourceMgr(SourceManager &SM, const LangOptions &LO)*/

  /*    colorPrint("Next Loop");*/
  /*for (auto &file : code) {*/
  /*  clang::Rewriter Rewrite;*/
  /*  clang::StringRef name = file.first;*/
  /*  llvm::SmallVector<char> Out;*/
  /*  clang::StringRef content = file.second.toStringRef(Out);*/
  /*  clang::SourceManagerForFile mgr(name, content);*/
  /*  clang::SourceManager &srcMgr = mgr.get();*/
  /*  clang::CompilerInstance Compiler;*/
  /*  clang::LangOptions &LO = Compiler.getLangOpts();*/
  /*  Rewrite.setSourceMgr(srcMgr, LO);*/
  /*  MyRewriteMain myWriter(Rewrite);*/
  /*  clang::ast_matchers::MatchFinder Finder;*/
  /*  Finder.addMatcher(clang::ast_matchers::ifStmt().bind("ifStmt"), &myWriter);*/
  /*  clang::tooling::runToolOnCode(clang::tooling::newFrontendActionFactory(&Finder)->create(),*/
  /*                                file.second,*/
  /*                                file.first*/
  /*                                );*/
  /*}*/

  /*int count = 0;*/
  /*for (std::unique_ptr<clang::ASTUnit> &unit : asts) {*/
  /*  colorPrint("For Loop of ASTs: " + std::to_string(count++));*/
  /*  colorPrint("    Is Main File: " + std::to_string(unit->isMainFileAST()));*/
  /*  clang::SourceManager &srcMgr = unit->getSourceManager();*/
  /*  clang::LangOptions langOps = unit->getLangOpts();*/
  /*  clang::ASTUnit::WhatToLoad astOnly = unit->LoadASTOnly;*/
  /*  auto comments = unit->getASTContext().CommentsLoaded;*/
  /*  colorPrint("    Comments Loaded: " + std::to_string(comments));*/
  /*  clang::ASTMutationListener *mutaListen = unit->getASTContext().getASTMutationListener();*/
  /*  colorPrint("    Create Rewrite");*/
  /*  clang::Rewriter Rewrite;*/
  /*  Rewrite.setSourceMgr(srcMgr, langOps);*/
  /*  MyRewriteMain myWriter(Rewrite);*/
  /*  clang::ast_matchers::MatchFinder Finder;*/
  /*  Finder.addMatcher(clang::ast_matchers::ifStmt().bind("ifStmt"), &myWriter);*/
  /*  colorPrint("    Run Tool");*/
  /*  unit->getASTContext().PrintStats();*/
  /*  Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());*/
  /*  colorPrint("    OverWrite");*/
  /*  colorPrint(std::to_string(Rewrite.overwriteChangedFiles()));*/
  /*}*/

  /*unit->LoadFromASTFile(const std::string &Filename, const PCHContainerReader &PCHContainerRdr, WhatToLoad ToLoad, IntrusiveRefCntPtr<DiagnosticsEngine> Diags, const FileSystemOptions &FileSystemOpts, std::shared_ptr<HeaderSearchOptions> HSOpts)*/
  /*clang::tooling::runToolOnCode(std::unique_ptr<FrontendAction> ToolAction, const Twine &Code);*/
  /*clang::tooling::runToolOnCode(clang::tooling::newFrontendActionFactory(&Finder).get(), unit->);*/
  /*clang::tooling::runToolOnCodeWithArgs(clang::tooling::newFrontendActionFactory(&Finder).get(), );*/
  /*clang::tooling::runToolOnCode(clang::tooling::newFrontendActionFactory(&Finder).get());*/
  /*clang::tooling::runToolOnCodeWithArgs();*/

  /*clang::CompilerInstance Compiler;*/
  /*clang::SourceManager &SourceMgr = Compiler.getSourceManager();*/
  /*clang::LangOptions &LO = Compiler.getLangOpts();*/
  /*clang::Rewriter Rewrite;*/
  /*Rewrite.setSourceMgr(SourceMgr, LO);*/
  /*MyRewriteMain Writer(Rewrite);*/
  /*clang::ast_matchers::MatchFinder Finder;*/
  /*Finder.addMatcher(clang::ast_matchers::ifStmt().bind("ifStmt"), &Writer);*/
  /*Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());*/
  /*Rewrite.overwriteChangedFiles();*/
  /*Rewrite.getEditBuffer(SourceMgr.getMainFileID()).write(raw_ostream &Stream);*/

  return 0;
}
