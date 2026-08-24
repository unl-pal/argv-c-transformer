#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

#include <assert.h>
int add(int a, int b) {
  int r = a + b;
  if (!(r >= a)) reach_error();
  return r;
}

int main(void) {
  add(__VERIFIER_nondet_int(), __VERIFIER_nondet_int());
  return 0;
}
