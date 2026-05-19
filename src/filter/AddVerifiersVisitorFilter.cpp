#include "AddVerifiersVisitorFilter.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

AddVerifiersVisitorFilter::AddVerifiersVisitorFilter(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(c), _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

bool AddVerifiersVisitorFilter::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();

  // First non "include" node in the tree to use as "insert verifier before here" location
  clang::Decl *firstNode;
  for (auto *decl : D->decls()) {
    // Verify the node is not a part of a non writable macro location
    if (mgr.isInMainFile(decl->getLocation()) && !mgr.isMacroBodyExpansion(decl->getLocation())) {
      firstNode = decl;
      break;
    }
  }

  firstNode->dumpColor();
  // Set the verifier insert before location to the first column of the first node
  clang::SourceLocation loc = mgr.translateLineCol(mgr.getMainFileID(), mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
  // If there is a comment for the firstnode adjust the placement location
  if (clang::RawComment *comment = _C->getRawCommentForDeclNoCache(firstNode)) {
    llvm::outs() << comment->getRawText(mgr) << "\n";
    loc = comment->getBeginLoc();
  }

  if (_NeededTypes->size()) {
    // insert blank line for new verifier code block
    _Rewriter.InsertTextBefore(loc, "\n");
  }

  std::string nondetName = "__VERIFIER_nondet_";
  // loops through all missing types to create verifiers for
  for (clang::QualType returnType : *_NeededTypes) {
    std::string returnTypeName = returnType.getAsString();
    llvm::outs() << returnTypeName << "\n";
    std::string newReturnTypeName;
    bool isPointer = returnType->isAnyPointerType();
    // Clear up issues with ClangAST naming and ArgC-Transformer src code needs
    if (returnTypeName == "_Bool") {
      newReturnTypeName = "bool";
    } else if (isPointer) {
      newReturnTypeName = "pointer";
      // Pointers all use the generic void pointer rather than the specific of the node
      returnTypeName = "void*";
    } else {
      for (unsigned i=0; i<returnTypeName.size(); i++) {
        char letter = returnTypeName[i];
        // More ClangAST name clean up
        if (letter == ' ') {
          newReturnTypeName += "";
        } else if (letter == '_') {
          newReturnTypeName += "";
        } else if (letter == '*') {
          newReturnTypeName += "";
        } else {
          newReturnTypeName += letter;
        } 
      }
    }

    // variables set and used to create the new function declaration
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + newReturnTypeName);
    clang::DeclarationName declName(funcName);
    clang::FunctionProtoType::ExtProtoInfo epi;
    clang::QualType funcQualType;
    // Set up pointer specific details as needed
    if (isPointer) {
      funcQualType = _C->getFunctionType(_C->VoidPtrTy, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    } else {
      funcQualType = _C->getFunctionType(returnType, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    }

    // Creates a new __VERIFIER function with the designated type
    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_C,
      D,
      loc,
      loc,
      declName,
      funcQualType,
      nullptr,
      clang::SC_Extern
    );

    // Mark the function as used to avoid issues later in the tree
    newFunction->setReferenced();
    newFunction->setIsUsed();

    // Create dummy Parameter VOID parameter variable
    clang::ParmVarDecl *vParm = clang::ParmVarDecl::Create(
      *_C,
      newFunction->getDeclContext(),
      newFunction->getLocation(),
      newFunction->getLocation(),
      nullptr,
      _C->VoidTy,
      nullptr,
      clang::SC_None,
      nullptr
    );

    // Use the dummy variables to finish setting up the declaration
    newFunction->setParams({vParm});
    vParm->setOwningFunction(newFunction);
    newFunction->addDecl(vParm);
    // Add the Verifier to the tree
    D->addDecl(newFunction);
    if (loc.isValid() && mgr.isInMainFile(loc)) {
      // Print the verifier to the valid pre firstnode location
      std::string verifierString = ""; // string to write to
      llvm::raw_string_ostream tempStream(verifierString); // stream to print output to string
      newFunction->print(tempStream); // use built in print
      llvm::outs() << verifierString << " - The String\n";
      _Rewriter.InsertTextBefore(loc, verifierString + ";\n"); // add ; for extern verifier
      llvm::outs() << "Inserted the text\n";
    }
  }
  return false;
}
