// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "RemoveVisitor.hpp"
#include "RemoveConsumer.hpp"

RemoveConsumer::RemoveConsumer(clang::Rewriter &rewriter,
                               std::shared_ptr<std::vector<std::string>> toRemove)
    : _Rewriter(rewriter), _ToRemove(toRemove) {}

void RemoveConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  if (!_ToRemove->empty()) {
    RemoveVisitor visitor(_Rewriter, _ToRemove);
    visitor.TraverseDecl(context.getTranslationUnitDecl());
  }
}
