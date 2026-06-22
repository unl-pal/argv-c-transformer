#include "local_with_std.h"

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
