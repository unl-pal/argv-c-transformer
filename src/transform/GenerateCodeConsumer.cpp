// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "GenerateCodeConsumer.hpp"
#include "RegenCode.hpp"

GenerateCodeConsumer::GenerateCodeConsumer(llvm::raw_fd_ostream &output) : _Output(output) {}

void GenerateCodeConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  RegenCodeVisitor Visitor(&context, _Output);
  Visitor.TraverseTranslationUnitDecl(context.getTranslationUnitDecl());
  llvm::outs() << "Ran the RegenCodeVisitor";
}
