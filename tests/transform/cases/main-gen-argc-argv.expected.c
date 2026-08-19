#include "argv_c_runtime.h"

int helper(int x) {
  return x + 1;
}

int original_main(int argc, char *argv[]) {
  return __VERIFIER_nondet_int();
}

int main(void) {
  helper(__VERIFIER_nondet_int());
  int argc = __HAVOC_ARGC();
  original_main(argc, __HAVOC_ARGV(argc));
  return 0;
}
