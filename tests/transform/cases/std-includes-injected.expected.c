#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"



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
