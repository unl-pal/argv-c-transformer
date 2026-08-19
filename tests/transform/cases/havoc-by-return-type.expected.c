#include <stdlib.h>
extern float __VERIFIER_nondet_float(void);
extern int __VERIFIER_nondet_int(void);
#define __HAVOC_BLOCK_MAX 128
extern void __VERIFIER_nondet_memory(void *, size_t);
extern size_t __VERIFIER_nondet_size_t(void);

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  ;
  char *s = (char *)({ char __havoc_str[__HAVOC_BLOCK_MAX]; __VERIFIER_nondet_memory(__havoc_str, __HAVOC_BLOCK_MAX); size_t __havoc_len = __VERIFIER_nondet_size_t(); if (__havoc_len >= __HAVOC_BLOCK_MAX) abort(); __havoc_str[__havoc_len] = 0; __havoc_str; });
  int *b = (int *)({ unsigned char __havoc_blk[__HAVOC_BLOCK_MAX]; __VERIFIER_nondet_memory(__havoc_blk, __HAVOC_BLOCK_MAX); __havoc_blk; });
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
