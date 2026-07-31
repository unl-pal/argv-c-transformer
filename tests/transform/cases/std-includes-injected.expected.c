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
static void *__havoc_block(size_t size) {
  void *block = malloc(size);
  __VERIFIER_nondet_memory(block, size);
  return block;
}
static char *__havoc_cstring(size_t size) {
  char *s = __havoc_block(size);
  size_t len = __VERIFIER_nondet_size_t();
  if (len >= size) abort();
  s[len] = '\0';
  return s;
}

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
  measure((const char *)__havoc_cstring(__HAVOC_STR_MAX));
  check(__VERIFIER_nondet_int());
  mask(__VERIFIER_nondet_uint());
  return 0;
}
