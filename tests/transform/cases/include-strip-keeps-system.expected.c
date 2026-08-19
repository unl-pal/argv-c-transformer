#include "argv_c_runtime.h"

#include <stdbool.h>

bool flag(int n) {
  return n > 0;
}

int main(void) {
  flag(__VERIFIER_nondet_int());
  return 0;
}
