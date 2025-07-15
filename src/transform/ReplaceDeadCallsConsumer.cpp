#include "ReplaceDeadCallsConsumer.hpp"
#include "ReplaceDeadCallsVisitor.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Type.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>

ReplaceDeadCallsConsumer::ReplaceDeadCallsConsumer(std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter) : _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

void ReplaceDeadCallsConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  llvm::outs() << "Running the ReplaceDeadCalls\n";
  // TODO REMOVE ME
  // Context.getTranslationUnitDecl()->dumpColor();
  ReplaceDeadCallsVisitor Visitor(&Context, _NeededTypes, _Rewriter);
  Visitor.VisitTranslationUnit(Context.getTranslationUnitDecl());
  llvm::outs() << "Ran the ReplaceDeadCalls\n";
  // TODO REMOVE ME
  if (Context.Idents.find("main") != Context.Idents.end()) {
    auto lookup = Context.Idents.find("main");
    llvm::outs() << "Found main name\n";
    // if (lookup) {
      llvm::outs() << "Found main name\n";
      if (clang::DeclarationName main = lookup->second) {
        llvm::outs() << "Found main name\n";
        if (clang::TranslationUnitDecl *TD = Context.getTranslationUnitDecl()) {
          llvm::outs() << "Found main name\n";
          if (clang::FunctionDecl *func = TD->lookup(main).find_first<clang::FunctionDecl>()) {
            llvm::outs() << "Found main name\n";
            func->dumpColor();
          }
        }
      }
    // }
  }
}
