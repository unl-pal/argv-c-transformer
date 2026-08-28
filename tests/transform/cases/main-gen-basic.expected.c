#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

int add(int a, int b) {
  return a + b;
}

int main(void) {
  add(__VERIFIER_nondet_int(), __VERIFIER_nondet_int());
  return 0;
}
