#include "RemoveNodeConsumer.hpp"
#include "RemoveNodeHandler.hpp"

#include <clang/ASTMatchers/ASTMatchers.h>

// Remove Nodes with specidied name
// check that function
// see if name matches and remove

// Replace Nodes for function calls that are not defined matcher
// Need to dump the ast and check what that looks like for those nodes
// attribute isDefined
  // attribute isUsed? of any use here?
// noun is CallExpr?
// do I need to check that it is nested in something?
// do I need to check that the node has parameters
// going to need to get the return type to figure out the replacement protocol
//


// Extra Questions along the way
// what is spellilng in the context of matchers and locations?

using namespace clang::ast_matchers;
void RemoveNodeConsumer::HandleTranslationUnit(
  clang::ASTContext &Context) {
  MatchFinder MatchFinder;
  RemoveNodeHandler Handler;
  MatchFinder.addMatcher(functionDecl(hasName(_Name)), &Handler);
  MatchFinder.matchAST(Context);
  Handler.onEndOfTranslationUnit();
}
