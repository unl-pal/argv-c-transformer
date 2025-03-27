#include "include/srcCodeGenerator.hpp"
#include <algorithm>
#include <clang-c/Index.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/CodeGenOptions.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>

// srcCodeGeneratorVisitor::srcCodeGeneratorVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output, std::shared_ptr<clang::Preprocessor> PP)
srcCodeGeneratorVisitor::srcCodeGeneratorVisitor(clang::ASTContext *C, llvm::raw_fd_ostream &output, clang::Preprocessor *PP)
    : _C(C),
  _Mgr(_C->getSourceManager()),
  _Output(output),
  _PP(PP),
  _Diag(_C->getDiagnostics()) {
}

bool srcCodeGeneratorVisitor::VisitTranslationUnitDecl(clang::TranslationUnitDecl *D) {
  if (!D) return true;
  llvm::outs() << "translation\n";
  // if (auto comment = _C->getRawCommentForAnyRedecl(D)) {
    // llvm::outs() << comment->getFormattedText(_Mgr, _Diag) << "\n";
    // _Output << "//"<< comment->getFormattedText(_Mgr, _Diag) << "\n";
  // auto anotherCom = _C->Comments.getCommentsInFile(FileID File)
  // }
  if (!_C->Comments.empty()) {
    auto comments =  *_C->Comments.getCommentsInFile(_Mgr.getMainFileID());
    if (comments.size()) {
      std::regex pattern("\\n");
      for (const auto &[_, comment] : comments) {
        std::string lines;
        if (comment->isMerged()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "is merged\n" << lines << "\n";
          _Output << "// " << std::regex_replace(lines, pattern, "\n// ") << "\n";
        } if (comment->isTrailingComment()) {
          llvm::outs() << "isTrailingComment\n" << lines << "\n";
          lines = comment->getFormattedText(_Mgr, _Diag);
        } if (comment->isAttached()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "isAttached\n" << lines << "\n";
        } if (comment->isOrdinary()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "isOrdinary\n" << lines << "\n";
        } if (comment->isDocumentation()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "isDocumentation\n" << lines << "\n";
        } if (comment->isAlmostTrailingComment()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "is almost trainling comment\n" << lines << "\n";
        } if (comment->isInvalid()) {
          lines = comment->getFormattedText(_Mgr, _Diag);
          llvm::outs() << "is invalid\n" << lines << "\n";
        }
      }
    }
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitTranslationUnitDecl(D);
}

bool srcCodeGeneratorVisitor::VisitExternCContextDecl(clang::ExternCContextDecl *D) {
  if (!D) return true;
  D->print(_Output);
  _Output << ";\n";
  if (D->isUsed() || D->isReferenced()) {
    if (const clang::RawComment *comment = _C->getRawCommentForAnyRedecl(D)) {
      _Output << comment->getFormattedText(_Mgr, _Diag);
    }
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitExternCContextDecl(D);
}

bool srcCodeGeneratorVisitor::VisitDecl(clang::Decl *D) {
  if (!D) return false;
  bool yes = false;
  if (_Mgr.isInMainFile(D->getLocation())) {
    if (D->hasBody()) {
      yes = false;
    } else if (D->getDeclContext()->getParent() &&
               D->getDeclContext()->isRecord()) {
      yes = false;
    // } else if (!D->getDeclContext()->isTranslationUnit() &&
    //            !D->getParentFunctionOrMethod()) {
    //   yes = true;
    }
  } else if (D->isFunctionOrFunctionTemplate() || D->getKind() == D->Typedef) {
    if (D->isUsed() && D->isReferenced()) {
      yes = true;
    // } else if (D->isReferenced()) {
    //   yes = true;
    // } else if (D->getDeclContext()->isExternCContext()) {
      // yes = true;
    }
  }
  if (yes) {
    // if (auto comment = _C->getRawCommentForAnyRedecl(D)) {
    //   llvm::outs() << comment->getFormattedText(_Mgr, _Diag) << "\n";
    //   _Output << "//"<< comment->getFormattedText(_Mgr, _Diag) << "\n";
    // }
    // if (clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(D)) {
    //   FD->print(_Output);
    //   if (!FD->hasBody()) {
    //     _Output << ";" << "\n";
    //   }
    // } else if (clang::dyn_cast<clang::VarDecl>(D) && D->getParentFunctionOrMethod()) {
    //   // } else {
    //     // auto allComments = _C->ParsedComments;
    //     // for (auto decl : FD->decls()) {
    //     //   if (allComments[decl]) {
    //     //     _Output << _C->getRawCommentForAnyRedecl(decl)->getFormattedText(_Mgr, _Diag) << "\n";
    //     //   }
    //     //   if (clang::NamedDecl *ND = clang::dyn_cast<clang::NamedDecl>(decl)) {
    //     //     ND->print(_Output);
    //     //     _Output << ";\n";
    //     //   } else {
    //     //     D->print(_Output);
    //     //   }
    //       // FD->getBody()->getSourceRange().printToString(_Mgr);
    //       // clang::SourceRange srcRange = FD->getBody()->getSourceRange();
    //       // clang::CharSourceRange charRange = clang::CharSourceRange::getTokenRange(srcRange);
    //       // std::string text = clang::Lexer::getSourceText(charRange, _Mgr, _C->getLangOpts()).str();
    //       // clang::Lexer::getSourceText(charRange, _Mgr, _C->getLangOpts()).find();
    //       // _Output << text;
    //       // FD->getBody()->printPretty(_Output, 0, _C->getPrintingPolicy());
    //     // }
    //     _Output << "\n";
    //   // }
    // } else if (D->getKind() == clang::RecordDecl::Field) {
    // } else if (clang::NamedDecl *ND = clang::dyn_cast<clang::NamedDecl>(D)) {
    //   ND->print(_Output);
    //   _Output << ";\n";
    // }
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitDecl(D);
  // return true;
}

bool srcCodeGeneratorVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
  if (!D) return false;
  clang::RawComment thing;
  thing.parse(*_C, _PP, D);
  std::string myComment = thing.getFormattedText(_Mgr, _Diag);
  llvm::outs() << myComment;
  _Output << myComment;
  if (auto comment = _C->getRawCommentForAnyRedecl(D)) {
    llvm::outs() << comment->getFormattedText(_Mgr, _Diag) << "\n";
    _Output << "//"<< comment->getFormattedText(_Mgr, _Diag) << "\n";
  // auto anotherCom = _C->Comments.getCommentsInFile(FileID File)
  }
  if (_Mgr.isInMainFile(D->getLocation())) {
    D->print(_Output);
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitFunctionDecl(D);
}

bool srcCodeGeneratorVisitor::VisitVarDecl(clang::VarDecl *D) {
  if (!D) return false;
  clang::RawComment thing;
  thing.parse(*_C, _PP, D);
  std::string myComment = thing.getFormattedText(_Mgr, _Diag);
  _Output << myComment;
  llvm::outs() << myComment;
  if (auto comment = _C->getRawCommentForAnyRedecl(D)) {
    llvm::outs() << comment->getFormattedText(_Mgr, _Diag) << "\n";
    _Output << "//"<< comment->getFormattedText(_Mgr, _Diag) << "\n";
  // auto anotherCom = _C->Comments.getCommentsInFile(FileID File)
  }
  if (_Mgr.isInMainFile(D->getLocation()) &&
      D->isDefinedOutsideFunctionOrMethod()) {
      // !D->getDeclContext()->getParent()) {
    D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitVarDecl(D);
}

bool srcCodeGeneratorVisitor::VisitRecordDecl(clang::RecordDecl *D) {
  if (!D) return false;
  clang::RawComment thing;
  // auto wrong = thing.parse(*_C, _PP, D);
  // clang::RawCommentList *rawList;
  // auto fullCom = _C->getCommentForDecl(D, _PP);
  if (auto comment = _C->getRawCommentForAnyRedecl(D)) {
    llvm::outs() << comment->getFormattedText(_Mgr, _Diag) << "\n";
    _Output << "//" << comment->getFormattedText(_Mgr, _Diag) << "\n";
  // auto anotherCom = _C->Comments.getCommentsInFile(FileID File)
  }
  std::string myComment = thing.getFormattedText(_Mgr, _Diag);
  _Output << myComment;
  llvm::outs() << myComment;
  if (_Mgr.isInMainFile(D->getLocation())) {
      D->print(_Output);
    _Output << ";\n";
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitRecordDecl(D);
}

bool srcCodeGeneratorVisitor::VisitStmt(clang::Stmt *S) {
  if (!S) return false;
  if (_Mgr.isInMainFile(S->getBeginLoc())) {
    // S->printPretty(_Output, 0, _C->getPrintingPolicy());
    // _Output << "\n";
    /*S->printPretty(raw_ostream &OS, PrinterHelper *Helper, const PrintingPolicy &Policy)*/
    /*S->printPretty(llvm::outs(), PrinterHelper *Helper, const PrintingPolicy &Policy)*/
    /*S->getSourceRange().print(llvm::outs(), mgr);*/
    /*clang::PrinterHelper *helpME = clang::PrinterHelper(llvm::outs(), mgr);*/
    /*clang::PrintingPolicy policy = _C->getPrintingPolicy();*/
    /*if (S->getStmtClass() == clang::comments::FullComment) {*/
    /*if (S->getStmtClass() == clang::LineComment) {*/
    /*S->dumpColor();*/
    /*}*/
    /*clang::PrinterHelper *helpMe;*/
    /*helpMe->handledStmt(S, llvm::outs());*/
    /*S->printPretty(llvm::outs(), 0, policy, 0, "\n", _C);*/
    /*llvm::outs() << "\n";*/
  }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitStmt(S);
  // return true;
}

bool srcCodeGeneratorVisitor::VisitCallExpr(clang::CallExpr *E) {
  if (!_Mgr.isInMainFile(E->getExprLoc())) return true;
  // if (clang::FunctionDecl *func = E->getCalleeDecl()->getAsFunction()) {
  //   if (const clang::RawComment *comment = _C->getRawCommentForAnyRedecl(func)) {
  //     _Output << "//" << comment->getFormattedText(_Mgr, _Diag) << "\n";
  //   }
  //   // func->print(_Output);
  // }
  return clang::RecursiveASTVisitor<srcCodeGeneratorVisitor>::VisitCallExpr(E);
}
