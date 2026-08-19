#include "argv_c_runtime.h"



int run(int n) {
  char *s = (char *)__HAVOC_CSTRING();
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
