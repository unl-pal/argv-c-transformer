#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern int __VERIFIER_nondet_int(void);
extern unsigned int __VERIFIER_nondet_uint(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

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
  char __h0[__HAVOC_STR_MAX];
  __VERIFIER_nondet_memory(__h0, sizeof(__h0));
  size_t __h0_len = __VERIFIER_nondet_size_t();
  if (__h0_len >= __HAVOC_STR_MAX) abort();
  __h0[__h0_len] = '\0';
  measure(__h0);
  check(__VERIFIER_nondet_int());
  mask(__VERIFIER_nondet_uint());
  return 0;
}
