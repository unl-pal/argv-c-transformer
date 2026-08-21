#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

#include <stdbool.h>

bool flag(int n) {
  return n > 0;
}

int main(void) {
  flag(__VERIFIER_nondet_int());
  return 0;
}
