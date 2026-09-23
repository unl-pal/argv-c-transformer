#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#define EXPECT(x) __builtin_expect((x), 1)
int helper(int x) { return x * 2; }

// Compiler builtins have no declaring header but are not in-file calls: left alone,
// while a call in their arguments is still havocked.
int direct(int v) {
  if (__builtin_expect(__VERIFIER_nondet_int(), 1)) return 1;
  return 0;
}

int via_macro(int v) {
  if (EXPECT(v > 0)) return 1;
  return 0;
}

int main(void) {
  helper(__VERIFIER_nondet_int());
  direct(__VERIFIER_nondet_int());
  via_macro(__VERIFIER_nondet_int());
  return 0;
}
