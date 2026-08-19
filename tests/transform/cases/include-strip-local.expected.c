

#include <stdlib.h>
extern int __VERIFIER_nondet_int(void);
#define __HAVOC_BLOCK_MAX 128
extern void __VERIFIER_nondet_memory(void *, size_t);
extern size_t __VERIFIER_nondet_size_t(void);

int run(int n) {
  char *s = (char *)({ char __havoc_str[__HAVOC_BLOCK_MAX]; __VERIFIER_nondet_memory(__havoc_str, __HAVOC_BLOCK_MAX); size_t __havoc_len = __VERIFIER_nondet_size_t(); if (__havoc_len >= __HAVOC_BLOCK_MAX) abort(); __havoc_str[__havoc_len] = 0; __havoc_str; });
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
