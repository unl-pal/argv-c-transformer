

#include <stdlib.h>
extern int __VERIFIER_nondet_int(void);
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

int run(int n) {
  char *s = (char *)__havoc_cstring(128);
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
