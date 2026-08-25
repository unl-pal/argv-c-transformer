#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

int takesPtr(int *p) {
  return *p;
}

int takesVarargs(int a, ...) {
  return a;
}

int plain(int a) {
  return a;
}

int main(void) {
  int __h0[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__h0, sizeof(__h0));
  takesPtr(__h0);
  plain(__VERIFIER_nondet_int());
  return 0;
}
