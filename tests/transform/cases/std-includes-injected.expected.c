#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "argv_c_runtime.h"



size_t measure(const char *s) {
  size_t i = 0;
  while (s[i]) i++;
  return i;
}

bool check(int n) {
  return n > 0;
}

uint32_t mask(uint32_t x) {
  return x & 0xFF;
}

int main(void) {
  check(__VERIFIER_nondet_int());
  mask(__VERIFIER_nondet_uint());
  return 0;
}
