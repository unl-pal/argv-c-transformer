#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#include <assert.h>
int helper(int x) { return x * 2; }
int check(int v) {
  if (!(__VERIFIER_nondet_int() > 0)) reach_error();
  return v;
}

int main(void) {
  helper(__VERIFIER_nondet_int());
  check(__VERIFIER_nondet_int());
  return 0;
}
