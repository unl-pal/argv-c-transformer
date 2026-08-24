#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"



int run(int n) {
  char __havoc_buf0[__HAVOC_BLOCK_MAX];
  char *s = (char *)__havoc_cstring_fill(__havoc_buf0, __HAVOC_BLOCK_MAX);
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
