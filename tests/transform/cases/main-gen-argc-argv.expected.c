#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

int helper(int x) {
  return x + 1;
}

int original_main(int argc, char *argv[]) {
  return __VERIFIER_nondet_int();
}

int main(void) {
  helper(__VERIFIER_nondet_int());
  int argc = __HAVOC_ARGC();
  original_main(argc, __havoc_argv_fill(argc));
  return 0;
}
