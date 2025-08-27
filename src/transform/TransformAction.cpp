#include "TransformAction.hpp"
#include "GenerateIncludeConsumer.hpp"
#include "AddVerifiersConsumer.hpp"
#include "ReplaceDeadCallsConsumer.hpp"
#include "GenerateCodeConsumer.hpp"

#include <IsThereMainConsumer.hpp>
#include <clang/AST/ASTContext.h>
#include <clang/AST/TemplateName.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
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
  // Identify all standard headers used in the code for tracking and inclusion purposes

  // This functionality and default meta data on standard libraries can be used
  // to replace the regex logic for filtering out files with unwanted inlcudes
  // but would require building the AST before checking rather than a simple
  // read

  if (_Mgr.isInMainFile(HashLoc)) {
    if (_AllStandardHeaders.count(FileName)) {
      if (!_AlreadyIncluded.count(FileName)) {
        _AlreadyIncluded.emplace(FileName);
        // llvm::outs() << "Found include directive: " << FileName << " (";
        if (IsAngled) {
          // llvm::outs() << "<>";
          // _Output << "#include <" << FileName << ">\n";
        } else {
          llvm::outs() << "\"\"";
          // _Output << "#include \"" << FileName << "\"\n";
        }
        // llvm::outs() << ") at " << HashLoc.printToString(_Mgr) << "\n" ;
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
TransformAction::TransformAction(llvm::raw_fd_ostream &output) : _Output(output), _Rewriter() {}

// Overridden function that uses a ConsumerMultiplexer instead of a single
// ASTConsumer to run many consumers, handlers and visitors over the same AST
std::unique_ptr<clang::ASTConsumer>
TransformAction::CreateASTConsumer(clang::CompilerInstance &compiler,
                                         llvm::StringRef          filename) {
  // llvm::outs() << "Creating Ast Consumer for: " << filename << "\n";
  clang::Preprocessor &pp = compiler.getPreprocessor();
  pp.addPPCallbacks(std::make_unique<IncludeFinder>(compiler.getSourceManager(), this->_Output));

  // If only directives are expanded then standard library header code is not
  // included in the AST. Look into other options or potential interest in this
  // functionality

  // pp.SetMacroExpansionOnlyInDirectives(); // Come Back To This

  llvm::outs() << "CreateASTConsumer Method is about to run on: " << filename << "\n";

  // All the verifiers functions that will be needed in the benchmark
  std::set<clang::QualType> *neededTypes = new std::set<clang::QualType>();

  // Temp vector is used to store the various consumers and copied to the multiplexor at a later stage.
  // This prevents the compiler from trying to infer the type incorrectly or attempting to optimize the code detrimentally as seem in previous runs
  std::vector<std::unique_ptr<clang::ASTConsumer>> tempVector;
  tempVector.emplace_back(std::make_unique<GenerateIncludeConsumer>(_Output));
  tempVector.emplace_back(std::make_unique<ReplaceDeadCallsConsumer>(neededTypes, _Rewriter));
  tempVector.emplace_back(std::make_unique<AddVerifiersConsumer>(_Output, neededTypes, _Rewriter));
  tempVector.emplace_back(std::make_unique<IsThereMainConsumer>(_Rewriter));
  // Code Generation is not currently in use as it is designed to create pro compiled files
  // tempVector.emplace_back(std::make_unique<GenerateCodeConsumer>(_Output));

  // Multiplexor of all consumers that will be run over the same AST
  std::unique_ptr<clang::MultiplexConsumer> result =
    std::make_unique<clang::MultiplexConsumer>(std::move(tempVector));
  // copy in the tempVector for previously stated reasons

  // Debug statement for when debug levels are implemented
  llvm::outs() << "CreateASTConsumer Method ran on: " << filename << "\n";
  return result;
}

// Function that runs before any of the consumers but after preprocessor steps
bool TransformAction::BeginSourceFileAction(clang::CompilerInstance &compiler) {
  llvm::outs() << "Begin Source File Action" << "\n";
  // Set the Rewriter fields before used in the consumers
  _Rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
  bool result = clang::ASTFrontendAction::BeginSourceFileAction(compiler);
  return result;
}

// Function that runs after all of the consumers but before the AST is cleaned up
void TransformAction::EndSourceFileAction() {
  llvm::outs() << "End Source File Action" << "\n";
  // Retrieve the edited buffer and write to the new output location
  _Rewriter.getEditBuffer(getCompilerInstance().getSourceManager().getMainFileID()).write(_Output);
}
