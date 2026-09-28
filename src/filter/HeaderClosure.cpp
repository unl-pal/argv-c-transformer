// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#include "HeaderClosure.hpp"
#include "DebugLog.hpp"
#include "StdHeaders.hpp"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroInfo.h>
#include <cctype>
#include <deque>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Path.h>
#include <optional>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Declaration collecting and helpers
// ---------------------------------------------------------------------------
namespace {

/** @brief True if `loc` sits in a real file that is neither the main file nor a system header. */
bool isLocalHeaderLoc(const clang::SourceManager &mgr, clang::SourceLocation loc) {
  if (loc.isInvalid())
    return false;
  clang::SourceLocation fileLoc = mgr.getFileLoc(loc);
  if (fileLoc.isInvalid() || mgr.isInMainFile(fileLoc) || mgr.isInSystemHeader(fileLoc))
    return false;
  clang::FileID id = mgr.getFileID(fileLoc);
  return id.isValid() && mgr.getFileEntryRefForID(id).has_value();
}

/**
 * @brief Walks the AST roots, collecting declarations that live in project-local
 * headers (to inline) and ones that live in system headers (to #include).
 */
class ClosureCollector : public clang::RecursiveASTVisitor<ClosureCollector> {
public:
  explicit ClosureCollector(clang::ASTContext &context) : _Mgr(context.getSourceManager()) {}

  bool VisitTypeLoc(clang::TypeLoc typeLoc) {
    addType(typeLoc.getType());
    return true;
  }
  bool VisitExpr(clang::Expr *expr) {
    addType(expr->getType());
    return true;
  }
  bool VisitDeclRefExpr(clang::DeclRefExpr *expr) {
    addDecl(expr->getDecl());
    return true;
  }
  bool VisitMemberExpr(clang::MemberExpr *expr) {
    addDecl(expr->getMemberDecl());
    return true;
  }
  bool VisitCallExpr(clang::CallExpr *expr) {
    addDecl(expr->getCalleeDecl());
    return true;
  }
  bool VisitDeclStmt(clang::DeclStmt *stmt) {
    for (clang::Decl *decl : stmt->decls())
      addDecl(decl);
    return true;
  }

  /// declarations to inline
  const std::vector<const clang::Decl *> &needed() const { return _Needed; }
  /// system header declarations, needing an #include.
  const std::vector<const clang::Decl *> &fromSystem() const { return _FromSystem; }

  void addDecl(const clang::Decl *decl) {
    if (!decl)
      return;

    // get a record's actual definition
    if (const auto *record = llvm::dyn_cast<clang::RecordDecl>(decl))
      if (const clang::RecordDecl *def = record->getDefinition())
        decl = def;
    if (const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl))
      decl = func->getCanonicalDecl();

    if (!_SeenDecls.insert(decl).second)
      return;

    if (isLocalHeaderLoc(_Mgr, decl->getLocation())) {
      _Needed.push_back(decl);
    } else if (_Mgr.isInSystemHeader(_Mgr.getFileLoc(decl->getLocation()))) {
      _FromSystem.push_back(decl);
      return;
    }

    recurse(decl);
  }

private:
  void recurse(const clang::Decl *decl) {
    if (const auto *typedefDecl = llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
      addType(typedefDecl->getUnderlyingType());
    } else if (const auto *record = llvm::dyn_cast<clang::RecordDecl>(decl)) {
      for (const clang::FieldDecl *field : record->fields())
        addType(field->getType());
    } else if (const auto *enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl)) {
      for (const clang::EnumConstantDecl *constant : enumDecl->enumerators())
        if (const clang::Expr *init = constant->getInitExpr())
          TraverseStmt(const_cast<clang::Expr *>(init));
    } else if (const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
      // prototype only because we don't re-emit the body
      addType(func->getReturnType());
      for (const clang::ParmVarDecl *parm : func->parameters())
        addType(parm->getOriginalType());
    } else if (const auto *var = llvm::dyn_cast<clang::VarDecl>(decl)) {
      addType(var->getType());
      if (const clang::Expr *init = var->getInit())
        TraverseStmt(const_cast<clang::Expr *>(init));
    } else if (const auto *field = llvm::dyn_cast<clang::FieldDecl>(decl)) {
      addDecl(field->getParent());
      addType(field->getType());
    } else if (const auto *constant = llvm::dyn_cast<clang::EnumConstantDecl>(decl)) {
      addDecl(llvm::dyn_cast<clang::Decl>(constant->getDeclContext()));
    }
  }

  void addType(clang::QualType type) {
    if (type.isNull())
      return;
    const clang::Type *ptr = type.getTypePtrOrNull();
    if (!ptr || !_SeenTypes.insert(ptr).second)
      return;

    // Typedefs need to be checked before records because either is gettable but a typedef
    // can recurse into a record, and otherwise we'd lose the typedef identifier
    if (const auto *typedefType = ptr->getAs<clang::TypedefType>()) {
      addDecl(typedefType->getDecl());
      return;
    }
    if (const auto *recordType = ptr->getAs<clang::RecordType>()) {
      addDecl(recordType->getDecl());
      return;
    }
    if (const auto *enumType = ptr->getAs<clang::EnumType>()) {
      addDecl(enumType->getDecl());
      return;
    }
    if (ptr->isAnyPointerType()) {
      addType(ptr->getPointeeType());
      return;
    }
    if (const clang::ArrayType *arrayType = ptr->getAsArrayTypeUnsafe()) {
      addType(arrayType->getElementType());
      return;
    }
    if (const auto *protoType = ptr->getAs<clang::FunctionProtoType>()) {
      addType(protoType->getReturnType());
      for (clang::QualType param : protoType->getParamTypes())
        addType(param);
      return;
    }
    if (const auto *funcType = ptr->getAs<clang::FunctionType>())
      addType(funcType->getReturnType());
  }

  clang::SourceManager &_Mgr;
  std::unordered_set<const clang::Decl *> _SeenDecls;
  std::unordered_set<const clang::Type *> _SeenTypes;
  std::vector<const clang::Decl *> _Needed;
  std::vector<const clang::Decl *> _FromSystem;
}; // ClosureCollector

/** @brief One declaration rendered to text, keyed by its span for ordering and overlap checks. */
struct EmittedDecl {
  unsigned begin = 0;
  unsigned end = 0;
  std::string text;
};

/**
 * @brief Renders one declaration as EmittedDecl
 */
std::optional<EmittedDecl> renderDecl(const clang::Decl *decl, const clang::SourceManager &mgr,
                                      const clang::LangOptions &langOpts) {
  clang::SourceRange range = decl->getSourceRange();
  const auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
  bool isDefinition = func && func->doesThisDeclarationHaveABody() && func->getBody();
  if (isDefinition)
    // remove body
    range.setEnd(func->getBody()->getBeginLoc().getLocWithOffset(-1));

  // makeFileCharRange resolves macro locations, so a declaration produced by a macro is emitted
  // as the invocation that produced it
  clang::CharSourceRange chars =
      clang::Lexer::makeFileCharRange(isDefinition ? clang::CharSourceRange::getCharRange(range)
                                                   : clang::CharSourceRange::getTokenRange(range),
                                      mgr, langOpts);
  if (chars.isInvalid())
    return std::nullopt;

  bool invalid = false;
  llvm::StringRef text = clang::Lexer::getSourceText(chars, mgr, langOpts, &invalid);
  if (invalid || text.trim().empty())
    return std::nullopt;

  std::string body = text.str();

  size_t firstChar = body.find_first_not_of(" \t\r\n");
  // remove whitespace
  if (firstChar != std::string::npos)
    body.erase(0, firstChar);
  // Trailing ';' is outside every one of these declarations' source ranges.
  size_t lastChar = body.find_last_not_of(" \t\r\n");
  if (lastChar != std::string::npos)
    body.erase(lastChar + 1);
  if (body.empty())
    return std::nullopt;
  if (body.back() != ';')
    body += ";";

  EmittedDecl emitted;
  emitted.begin = chars.getBegin().getRawEncoding();
  emitted.end = chars.getEnd().getRawEncoding();
  emitted.text = body;
  return emitted;
}

/**
 * @brief Gets the `#include <...>` that supplies a declaration reached in a system header.
 */
std::optional<std::string> systemHeaderFor(const clang::Decl *decl,
                                           const clang::SourceManager &mgr) {
  llvm::StringRef path = mgr.getFilename(mgr.getFileLoc(decl->getLocation()));
  if (!path.empty()) {
    // by convention most std/sys headers live under some include/ subdir
    size_t marker = path.rfind("include/");
    llvm::StringRef spelling = marker == llvm::StringRef::npos ? path : path.drop_front(marker + 8);
    // exceptions for sys, arpa, and netinet because e.g sys/types.h is a valid/portable include
    bool topLevel = !spelling.contains('/') || spelling.starts_with("sys/") ||
                    spelling.starts_with("arpa/") || spelling.starts_with("netinet/");
    // A leading "__" marks a compiler-internal fragment so fallback to the StdHeaders map
    if (topLevel && !spelling.empty() &&
        !llvm::StringRef(llvm::sys::path::filename(spelling)).starts_with("__"))
      return spelling.str();
  }

  // fallback to the hardcoded map of known std headers
  const auto *named = llvm::dyn_cast<clang::NamedDecl>(decl);
  if (named) {
    auto it = StdHeaders.find(named->getNameAsString());
    if (it != StdHeaders.end())
      return it->second;
  }
  return std::nullopt;
}

/** @brief Appends every identifier appearing in `text` to `out`.
 *
 *  This helper supports macro closure in local headers by re-lexing declarations that may have
 *  nested macros.
 */
void collectIdentifiers(llvm::StringRef text, std::vector<std::string> &out) {
  size_t i = 0;
  while (i < text.size()) {
    char c = text[i];
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      size_t start = i;
      while (i < text.size() &&
             (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_'))
        ++i;
      out.push_back(text.substr(start, i - start).str());
    } else {
      ++i;
    }
  }
}

} // namespace

// ---------------------------------------------------------------------------
// LocalHeaderPP
// ---------------------------------------------------------------------------

LocalHeaderPP::LocalHeaderPP(clang::SourceManager &SM, const clang::LangOptions &langOpts,
                             clang::Rewriter &rewriter, std::shared_ptr<HeaderClosureState> state)
    : _Mgr(SM), _LangOpts(langOpts), _Rewriter(rewriter), _State(state) {}

void LocalHeaderPP::InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &,
                                       llvm::StringRef FileName, bool IsAngled,
                                       clang::CharSourceRange FilenameRange,
                                       clang::OptionalFileEntryRef, llvm::StringRef,
                                       llvm::StringRef, const clang::Module *, bool,
                                       clang::SrcMgr::CharacteristicKind FileType) {
  // A quoted include is project-local by convention regardless of FileType.
  bool localTarget = !IsAngled || FileType == clang::SrcMgr::C_User;

  if (_Mgr.isInMainFile(HashLoc)) {
    if (!localTarget)
      return; // system include, kept by reference
    debugLog(3, "[filter] inlining project-local include: " + FileName.str());
    _State->strippedLocalInclude = true;
    _Rewriter.RemoveText(clang::CharSourceRange::getCharRange(HashLoc, FilenameRange.getEnd()));
    return;
  }

  // A system include written inside a project-local header. Re-emitting it is
  // over-inclusive but never wrong
  if (!localTarget && isLocalHeaderLoc(_Mgr, HashLoc))
    _State->systemIncludes.insert(FileName.str());
}

void LocalHeaderPP::MacroDefined(const clang::Token &MacroNameTok,
                                 const clang::MacroDirective *MD) {
  const clang::MacroInfo *info = MD ? MD->getMacroInfo() : nullptr;
  if (!info || info->isBuiltinMacro())
    return;
  clang::SourceLocation defLoc = info->getDefinitionLoc();
  if (!isLocalHeaderLoc(_Mgr, defLoc))
    return;

  clang::CharSourceRange range =
      clang::CharSourceRange::getTokenRange(defLoc, info->getDefinitionEndLoc());
  bool invalid = false;
  llvm::StringRef text = clang::Lexer::getSourceText(range, _Mgr, _LangOpts, &invalid);
  if (invalid || text.empty())
    return;

  const clang::IdentifierInfo *id = MacroNameTok.getIdentifierInfo();
  if (!id)
    return;

  MacroRecord record;
  record.text = "#define " + text.str();
  record.order = defLoc.getRawEncoding();
  _State->localMacros[id->getName().str()] = std::move(record);
}

void LocalHeaderPP::MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &MD,
                                 clang::SourceRange Range, const clang::MacroArgs *) {
  const clang::IdentifierInfo *id = MacroNameTok.getIdentifierInfo();
  const clang::MacroInfo *info = MD.getMacroInfo();
  if (!id || !info || !isLocalHeaderLoc(_Mgr, info->getDefinitionLoc()))
    return;

  clang::SourceLocation expansion = _Mgr.getExpansionLoc(Range.getBegin());
  if (!expansion.isValid() || !_Mgr.isInMainFile(expansion))
    return;
  _State->macroUses.emplace_back(id->getName().str(), expansion);
}

// ---------------------------------------------------------------------------
// HeaderClosureConsumer
// ---------------------------------------------------------------------------

HeaderClosureConsumer::HeaderClosureConsumer(clang::Rewriter &rewriter,
                                             std::shared_ptr<std::vector<std::string>> toRemove,
                                             std::shared_ptr<HeaderClosureState> state)
    : _Rewriter(rewriter), _ToRemove(toRemove), _State(state) {}

void HeaderClosureConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  // No local include means nothing was deleted, so there is nothing to inline.
  if (!_State->strippedLocalInclude)
    return;

  clang::SourceManager &mgr = context.getSourceManager();
  const clang::LangOptions &langOpts = context.getLangOpts();
  std::set<std::string> rejected(_ToRemove->begin(), _ToRemove->end());

  // --- roots --------------------------------------------------------------
  // Traverse roots and collect decls: Surviving function bodies, signatures, and
  // main-file declarations
  ClosureCollector collector(context);
  std::vector<std::pair<unsigned, unsigned>> rejectedBodies;
  for (clang::Decl *decl : context.getTranslationUnitDecl()->decls()) {
    if (!mgr.isInMainFile(mgr.getFileLoc(decl->getLocation())))
      continue;
    auto *func = llvm::dyn_cast<clang::FunctionDecl>(decl);
    if (!func) {
      collector.TraverseDecl(decl);
      continue;
    }
    collector.addDecl(func);
    if (!func->doesThisDeclarationHaveABody() || !func->getBody())
      continue;
    if (!rejected.count(func->getNameAsString())) {
      collector.TraverseStmt(func->getBody());
      continue;
    }
    // otherwise, dont traverse and remember its file range
    clang::SourceRange body = func->getBody()->getSourceRange();
    if (mgr.isInMainFile(body.getBegin()) && mgr.isInMainFile(body.getEnd()))
      rejectedBodies.emplace_back(mgr.getFileOffset(body.getBegin()),
                                  mgr.getFileOffset(body.getEnd()));
  }

  // --- declaration closure ------------------------------------------------
  std::vector<EmittedDecl> emitted;
  std::set<std::string> forwardDecls;
  for (const clang::Decl *decl : collector.needed()) {
    std::optional<EmittedDecl> rendered = renderDecl(decl, mgr, langOpts);
    if (!rendered) {
      const auto *named = llvm::dyn_cast<clang::NamedDecl>(decl);
      debugLog(1, "[filter] closure could not render declaration: " +
                      (named ? named->getNameAsString() : std::string("<unnamed>")));
      continue;
    }
    emitted.push_back(std::move(*rendered));

    // Emit forward decls for any record to protect against cyclic/mutual references
    if (const auto *record = llvm::dyn_cast<clang::RecordDecl>(decl))
      if (!record->getName().empty())
        forwardDecls.insert(std::string(record->getKindName()) + " " + record->getName().str() +
                            ";");
  }

  // order declarations by their source location (tie break for overlapping decl types when same code range has multiple)
  std::sort(emitted.begin(), emitted.end(), [](const EmittedDecl &a, const EmittedDecl &b) {
    return a.begin != b.begin ? a.begin < b.begin : a.end > b.end;
  });

  std::string declText;
  unsigned lastRange = 0;
  for (const EmittedDecl &decl : emitted) {
    // A "typedef struct { ... } X;" yields both a RecordDecl and a TypedefDecl
    // skip a typedef it its record was already emitted
    if (decl.begin < lastRange)
      continue;
    lastRange = decl.end;
    declText += decl.text + "\n";
  }

  // --- macro closure ------------------------------------------------------
  // we need to follow transitive macro references so we use a worklist and visited set
  std::deque<std::string> pending;
  std::set<std::string> neededMacros;
  auto require = [&](const std::string &name) {
    // if not local header Macro or already in needed skip
    if (!_State->localMacros.count(name) || !neededMacros.insert(name).second)
      return;
    pending.push_back(name);
  };

  // validate macro uses aren't in bodies that are being removed
  for (const std::pair<std::string, clang::SourceLocation> &use : _State->macroUses) {
    if (!mgr.isInMainFile(use.second))
      continue;
    unsigned offset = mgr.getFileOffset(use.second);
    bool inRejected = false;
    for (const std::pair<unsigned, unsigned> &body : rejectedBodies)
      if (offset >= body.first && offset <= body.second)
        inRejected = true;
    if (!inRejected)
      require(use.first); // add to pending
  }

  // check emitted declaration for macro references
  std::vector<std::string> identifiers;
  collectIdentifiers(declText, identifiers); // put macros in identifiers
  for (const std::string &name : identifiers)
    require(name);
  while (!pending.empty()) { // follow nested references
    std::string name = pending.front();
    pending.pop_front();
    identifiers.clear();
    collectIdentifiers(_State->localMacros.at(name).text, identifiers);
    for (const std::string &nested : identifiers)
      require(nested);
  }

  std::vector<const MacroRecord *> macros;
  for (const std::string &name : neededMacros)
    macros.push_back(&_State->localMacros.at(name));
  std::sort(macros.begin(), macros.end(),
            [](const MacroRecord *a, const MacroRecord *b) { return a->order < b->order; });

  // --- system headers the closure still needs -----------------------------
  std::set<std::string> includes = _State->systemIncludes;
  for (const clang::Decl *decl : collector.fromSystem()) {
    std::optional<std::string> header = systemHeaderFor(decl, mgr);
    if (header) {
      includes.insert(*header);
      continue;
    }
    const auto *named = llvm::dyn_cast<clang::NamedDecl>(decl);
    debugLog(1, "[filter] closure could not map to an includable header: " +
                    (named ? named->getNameAsString() : std::string("<unnamed>")));
  }

  // --- emit ---------------------------------------------------------------
  std::string block;
  for (const std::string &header : includes)
    block += "#include <" + header + ">\n";
  for (const MacroRecord *macro : macros)
    block += macro->text + "\n";
  for (const std::string &fwd : forwardDecls)
    block += fwd + "\n";
  block += declText;

  if (block.empty())
    return;

  debugLog(2, "[filter] header closure: " + std::to_string(includes.size()) + " include(s), " +
                  std::to_string(macros.size()) + " macro(s), " + std::to_string(emitted.size()) +
                  " declaration(s)");
  // put it on line 1
  _Rewriter.InsertTextBefore(mgr.translateLineCol(mgr.getMainFileID(), 1, 1), block + "\n");
}
