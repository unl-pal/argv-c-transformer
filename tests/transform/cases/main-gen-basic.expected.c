#include "argv_c_runtime.h"

int add(int a, int b) {
  return a + b;
}

int main(void) {
  add(__VERIFIER_nondet_int(), __VERIFIER_nondet_int());
  return 0;
}
