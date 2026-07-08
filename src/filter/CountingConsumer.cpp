// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "CountingConsumer.hpp"

#include <llvm/Support/raw_ostream.h>

CountingConsumer::CountingConsumer(
    std::shared_ptr<std::unordered_map<std::string, CountingVisitor::attributes>> toFilter)
    : _ToFilter(toFilter) {}

void CountingConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  CountingVisitor Visitor(&Context, _ToFilter);
  Visitor.TraverseTranslationUnitDecl(Context.getTranslationUnitDecl());
}
