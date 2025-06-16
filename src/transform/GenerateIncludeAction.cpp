#include "GenerateIncludeAction.hpp"
#include "GenerateIncludeConsumer.hpp"
#include <RegenCode.hpp>
#include <ReplaceCallsVisitor.hpp>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/TemplateName.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <vector>

void IncludeFinder::InclusionDirective(clang::SourceLocation HashLoc,
                        const clang::Token & IncludeTok,
                        llvm::StringRef FileName,
                        bool IsAngled,
                        clang::CharSourceRange FilenameRange,
                        clang::OptionalFileEntryRef File,
                        llvm::StringRef SearchPath,
                        llvm::StringRef RelativePath,
                        const clang::Module * SuggestedModule,
                        bool ModuleImported,
                        clang::SrcMgr::CharacteristicKind FileType) {
  // SearchPath == clang::DefaultArguments
  if (_AllStandardHeaders.count(FileName)) {
    if (!_AlreadyIncluded.count(FileName)) {
      _AlreadyIncluded.emplace(FileName);
      llvm::outs() << "Found include directive: " << FileName << " (";
      if (IsAngled) {
        llvm::outs() << "<>";
        _Output << "#include <" << FileName << ">\n";
      } else {
        llvm::outs() << "\"\"";
        _Output << "#include \"" << FileName << "\"\n";
      }
      llvm::outs() << ") at " << HashLoc.printToString(_Mgr) << "\n";
    }
  }
  // if (FileType == clang::SrcMgr::C_ExternCSystem) {
  // }
}

IncludeFinder::IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output)
      : _Mgr(SM), _Output(output) {}

GenerateIncludeAction::GenerateIncludeAction(llvm::raw_fd_ostream &output) : _Output(output) {}

std::unique_ptr<clang::ASTConsumer>
GenerateIncludeAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                         llvm::StringRef          filename) {
  llvm::outs() << "Creating Ast Consumer for: " << filename << "\n";
  clang::Preprocessor &pp = compiler.getPreprocessor();
  // pp.PrintStats();
  // pp.getPreprocessorOpts();
  pp.addPPCallbacks(std::make_unique<IncludeFinder>(compiler.getSourceManager(), this->_Output));

  llvm::outs() << "Added Callbacks for: " << filename << "\n";
  // TODO implement a comment handler in code regen
  // pp.addCommentHandler(CommentHandler *Handler)

  llvm::outs() << "CreateASTConsumer Method is about to run on: " << filename << "\n";
  // std::unique_ptr<clang::MultiplexConsumer> result;
  // result->Initialize(compiler.getASTContext());

  std::vector<std::unique_ptr<clang::ASTConsumer>> tempV = std::vector<std::unique_ptr<clang::ASTConsumer>>();
  tempV.push_back(std::make_unique<GenerateIncludeConsumer>(_Output));

  // tempV.push_back(std::make_unique<GenerateVerifiersConsumer>(_Output));
  // tempV.push_back(std::make_unique<ReplaceDeadCallsConsumer>());
  // tempV.push_back(std::make_unique<GenerateComplexTypeStringsConsumer>());
  // tempV.push_back(std::make_unique<RegenCodeConsumer>());

  // auto result =
  // std::unique_ptr<clang::MultiplexConsumer> result =
  // std::unique_ptr<clang::ASTConsumer> result =
    // std::make_unique<clang::MultiplexConsumer>(tempV);

  std::unique_ptr<clang::ASTConsumer> result = std::make_unique<GenerateIncludeConsumer>(_Output);

  // TODO see if needed
  // llvm::outs() << "CreateASTConsumer Method ran on: " << filename << "\n";
  return result;
  // return nullptr;
}

// bool GenerateIncludeAction::usesPreprocessorOnly() const {
//   return 1;
// }

bool GenerateIncludeAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  llvm::outs() << "Begin Source File Action\n";
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  llvm::outs() << "Post Begin Source File Action\n";
  return result;
}

void GenerateIncludeAction::EndSourceFileAction() {
  llvm::outs() << "Ending Source File Action\n";
}
