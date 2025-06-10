#pragma once

#include <clang/AST/ASTConsumer.h>
#include <ostream>

class ConsumerFindInclude : public clang::ASTConsumer {
public:
	ConsumerFindInclude(std::ostream &output);
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
	std::ostream &_Output;
};
