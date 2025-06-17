#include "GenerateIncludeAction.hpp"
#include "GenerateIncludeConsumer.hpp"
#include "AddVerifiersConsumer.hpp"
#include "ReplaceCallsVisitor.hpp"
#include "ReplaceDeadCallsConsumer.hpp"
#include "RegenCode.hpp"

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

// Overriden function for handling InclusionDirectives such as
// import and include statements when found by the Preprocessor
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
  if (_Mgr.isInMainFile(HashLoc)) {
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
  }
}

// Constructor for the IncludeFinder that sets up the source manager and output
// stream for regenerating the source code
IncludeFinder::IncludeFinder(clang::SourceManager &SM, llvm::raw_fd_ostream &output)
      : _Mgr(SM), _Output(output) {}

// Constructor for GenerateIncludeAction that sets up the output stream for
// regenerating source code
GenerateIncludeAction::GenerateIncludeAction(llvm::raw_fd_ostream &output) : _Output(output) {}

// Overridden function that uses a ConsumerMultiplexer instead of a single
// ASTConsumer to run many consumers, handlers and visitors over the same AST
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

  std::set<clang::QualType> *neededTypes = new std::set<clang::QualType>();

  neededTypes->emplace(compiler.getASTContext().IntTy);

  // Multiplexor of all consumers that will be run over the same AST
  std::unique_ptr<clang::MultiplexConsumer> result =
    std::make_unique<clang::MultiplexConsumer>((
      std::make_unique<GenerateIncludeConsumer>(_Output),
      std::make_unique<ReplaceDeadCallsConsumer>(neededTypes),
      std::make_unique<AddVerifiersConsumer>(_Output, neededTypes)
      // TODO Implement the next set of consumers to be run over this tree
      // std::make_unique<GenerateComplexTypeStringsConsumer>();
      // std::make_unique<RegenCodeConsumer>();
    ));

  // Debug statement for when debug levels are implemented
  // llvm::outs() << "CreateASTConsumer Method ran on: " << filename << "\n";
  return result;
}

// May be needed or implemented later to force only the preprocessor to run on code
// bool GenerateIncludeAction::usesPreprocessorOnly() const {
//   return 1;
// }

// Function that runs before any of the consumers but after preprocessor steps
bool GenerateIncludeAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  llvm::outs() << "Begin Source File Action\n";
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  llvm::outs() << "Post Begin Source File Action\n";
  return result;
}

// Function that runs after all of the consumers but before the AST is cleaned up
void GenerateIncludeAction::EndSourceFileAction() {
  llvm::outs() << "Ending Source File Action\n";
}
