#include "AddStdIncludesConsumer.hpp"
#include "StdHeaders.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <set>

namespace {

class TypeCollector : public clang::RecursiveASTVisitor<TypeCollector> {
public:
  TypeCollector(clang::SourceManager &SM) : _SM(SM) {}

  bool VisitTypeLoc(clang::TypeLoc TL) {
    if (!_SM.isInMainFile(TL.getBeginLoc()))
      return true;
    recordType(TL.getType());
    return true;
  }

  const std::set<std::string> &neededHeaders() const { return _NeededHeaders; }

private:
  void recordType(clang::QualType QT) {
    if (QT.isNull())
      return;

    const clang::Type *T = QT.getTypePtrOrNull();
    if (!T)
      return;

    if (const auto *TDT = T->getAs<clang::TypedefType>()) {
      std::string name = TDT->getDecl()->getNameAsString();
      if (auto info = stdHeaderForType(name))
        _NeededHeaders.insert(info->header);
    } else if (const auto *RT = T->getAs<clang::RecordType>()) {
      std::string name = RT->getDecl()->getNameAsString();
      if (auto info = stdHeaderForType(name))
        _NeededHeaders.insert(info->header);
    } else if (const auto *BT = T->getAs<clang::BuiltinType>()) {
      if (BT->getKind() == clang::BuiltinType::Bool) {
        if (auto info = stdHeaderForType("bool"))
          _NeededHeaders.insert(info->header);
      }
    }
  }

  clang::SourceManager &_SM;
  std::set<std::string> _NeededHeaders;
};

} // namespace

AddStdIncludesConsumer::AddStdIncludesConsumer(
    std::shared_ptr<std::set<std::string>> existingIncludes, clang::Rewriter &rewriter)
    : _ExistingIncludes(existingIncludes), _Rewriter(rewriter) {}

void AddStdIncludesConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::SourceManager &SM = Context.getSourceManager();

  TypeCollector collector(SM);
  collector.TraverseDecl(Context.getTranslationUnitDecl());

  std::string includes;
  for (const std::string &header : collector.neededHeaders()) {
    if (_ExistingIncludes->count(header))
      continue;
    includes += "#include <" + header + ">\n";
    _ExistingIncludes->insert(header);
  }

  if (includes.empty())
    return;

  clang::SourceLocation loc = SM.translateLineCol(SM.getMainFileID(), 1, 1);
  _Rewriter.InsertTextBefore(loc, includes);
}
