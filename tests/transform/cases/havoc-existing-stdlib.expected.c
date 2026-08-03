#include <stdlib.h>

#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
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

int uses_stdlib(const char *s) {
  return (int)atoi(s);
}

int main(void) {
  uses_stdlib((const char *)__havoc_cstring(__HAVOC_STR_MAX));
  return 0;
}
