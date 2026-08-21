#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

extern int __VERIFIER_nondet_int(void);

int pick(int n) {
  return n + __VERIFIER_nondet_int();
}

int main(void) {
  pick(__VERIFIER_nondet_int());
  return 0;
}
