#include "argv_c_runtime.h"

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
