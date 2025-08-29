#include "AddVerifiersVisitor.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <cstring>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <string>

AddVerifiersVisitor::AddVerifiersVisitor(clang::ASTContext *c, llvm::raw_fd_ostream &output, std::set<clang::QualType> *neededTypes, clang::Rewriter &rewriter)
    : _C(c), _Output(output), _NeededTypes(neededTypes), _Rewriter(rewriter) {
}

bool AddVerifiersVisitor::HandleTranslationUnit(clang::TranslationUnitDecl *D) {
  clang::SourceManager &mgr = _C->getSourceManager();
  clang::Decl *firstNode = nullptr;
  for (auto *decl : D->decls()) {
    if (!mgr.isMacroArgExpansion(decl->getLocation())) {
      if (mgr.isInMainFile(decl->getLocation())) {
        firstNode = decl;
        break;
      }
    }
  }
  
  clang::SourceLocation loc;
  if (firstNode && firstNode->getSourceRange().isValid()){

    loc = mgr.translateLineCol(mgr.getMainFileID(), mgr.getSpellingLineNumber(firstNode->getLocation()), 1);
    if (clang::RawComment *comment = _C->getRawCommentForDeclNoCache(firstNode)) {
        llvm::outs() << comment->getRawText(mgr) << "\n";
        loc = comment->getBeginLoc();
    }
  } else {
    loc = mgr.translateLineCol(mgr.getMainFileID(), 1, 1);
  }

  if (_NeededTypes->size()) {
    _Rewriter.InsertTextBefore(loc, "\n");
  }

  std::string nondetName = "__VERIFIER_nondet_";
  for (clang::QualType returnType : *_NeededTypes) {

    // Should probably have a enum or def somewhere with all supported
    // verrifiers to draw on for situations like this, or in config?
    if (!returnType->isBuiltinType() &&
        !returnType->isBooleanType() &&
        !returnType->isAnyCharacterType() &&
        !returnType->isAnyPointerType()
    ) {
      continue;
    }

    bool isPointer = returnType->isAnyPointerType();
    // Fix ClangAST and ArgC names to src code discrepencies
    std::string returnTypeName = returnType->isBooleanType() ? "bool" : returnType.getAsString();
    returnTypeName = isPointer ? "pointer" : returnTypeName;
    llvm::outs() << returnTypeName << "\n";
    std::replace(returnTypeName.begin(), returnTypeName.end(), ' ', '_');
    std::replace(returnTypeName.begin(), returnTypeName.end(), '*', '\0');
    if (!std::strcmp(&returnTypeName.at(returnTypeName.size() - 1), "_")) {
      returnTypeName.pop_back();
    }

    // Get the look up info for the potentially needed verifier
    clang::IdentifierInfo *funcName = &_C->Idents.get(nondetName + returnTypeName);
    clang::DeclarationName declName(funcName);
    // Check if the verifier is already in the tree
    if (!D->lookup(declName).empty()) {
      continue;
    }
    clang::FunctionProtoType::ExtProtoInfo epi;

    // Handle all pointers to void* type conversion
    clang::QualType funcQualType;
    if (isPointer) {
      funcQualType = _C->getFunctionType(_C->VoidPtrTy, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    } else {
      funcQualType = _C->getFunctionType(returnType, clang::ArrayRef<clang::QualType>({_C->VoidTy}), epi);
    }

    // Create the new verifier with the appropriate return type
    clang::FunctionDecl* newFunction = clang::FunctionDecl::Create(
      *_C,
      D,
      loc,
      loc.getLocWithOffset(1),
      declName,
      funcQualType,
      _C->CreateTypeSourceInfo(_C->VoidTy),
      clang::SC_Extern
    );

    // Set used and reference for AST meta knowledge
    newFunction->setReferenced();
    newFunction->setIsUsed();

    // Create dummy parameter variable to finish verifier signature
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

    // Add the dummy void parameter to the verifier code
    newFunction->setParams({vParm});
    vParm->setOwningFunction(newFunction);
    newFunction->addDecl(vParm);
    D->addDecl(newFunction);
    // Check for valid write location and add verifier to the main file
    if (loc.isValid() && mgr.isInMainFile(loc)) {
      // Use string and string stream to utilize built-in print to stream to create string
      std::string verifierString = "";
      llvm::raw_string_ostream tempStream(verifierString);
      newFunction->print(tempStream);
      llvm::outs() << verifierString << " - The String\n";
      _Rewriter.InsertTextBefore(loc, verifierString + ";\n");
      llvm::outs() << "Inserted the text\n";
    }
  }
  return false;
}
