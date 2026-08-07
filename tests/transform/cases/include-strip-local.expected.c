

#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern int __VERIFIER_nondet_int(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

int run(int n) {
  char __hret0[__HAVOC_STR_MAX];
  __VERIFIER_nondet_memory(__hret0, sizeof(__hret0));
  size_t __hret0_len = __VERIFIER_nondet_size_t();
  if (__hret0_len >= __HAVOC_STR_MAX) abort();
  __hret0[__hret0_len] = '\0';
  char *s = __hret0;
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
