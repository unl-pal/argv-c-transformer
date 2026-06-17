#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <memory>
#include <set>
#include <string>

/**
 * @brief ASTConsumer that generates the benchmark entry point.
 *
 * Any pre-existing {@code main} (including its forward declarations) is renamed
 * to {@code original_main}, then a fresh {@code int main(void)} is appended that
 * calls every function defined in the file with {@code __VERIFIER_nondet_*}
 * arguments. Functions with a parameter type that has no nondet equivalent
 * (pointers, structs, ...) and variadic functions are skipped. For
 * {@code original_main(int, char**)}, a synthesized {@code argc}/{@code argv}
 * harness is generated instead of skipping.
 */
class MainGenConsumer : public clang::ASTConsumer {
public:
  /**
   * @brief Constructs the consumer with the shared pipeline state.
   *
   * @param neededSuffixes Verifier suffixes used by the harness, shared with
   *        {@code AddVerifiersConsumer} which emits the extern declarations.
   * @param rewriter       Shared rewriter for modifying the source buffer.
   */
  MainGenConsumer(std::shared_ptr<std::set<std::string>> neededSuffixes, clang::Rewriter &rewriter);

  /**
   * @brief Renames an existing {@code main} and appends the generated harness main.
   *
   * Iterates all declarations in the translation unit, collecting functions with
   * definitions. Each is harnessed according to its signature: primitive-param
   * functions get {@code __VERIFIER_nondet_*} arguments, {@code main} is
   * delegated to {@code genMainHarness}, and unsupported/variadic functions are
   * skipped.
   *
   * @param Context The AST context for the translation unit being transformed.
   */
  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  /**
   * @brief Builds the harness body that invokes the renamed {@code original_main}.
   *
   * Unlike an arbitrary function, {@code main}'s pointer params have a known
   * contract, so instead of skipping it we synthesize a realistic call: a nondet
   * {@code argc} bounded to [0, 7] via {@code abort()}, and an {@code argv}
   * array of havocked, null-terminated C strings (via {@code __havoc_cstring}).
   * Registers any verifier helpers it uses in {@code _NeededSuffixes}.
   *
   * @param mainFn The original {@code main} FunctionDecl (already renamed in
   *               the rewriter output).
   * @return C statements (indented, newline-terminated) to splice into the
   *         generated {@code main} body.
   */
  std::string genMainHarness(const clang::FunctionDecl *mainFn);

  std::shared_ptr<std::set<std::string>> _NeededSuffixes;
  clang::Rewriter &_Rewriter;
};
