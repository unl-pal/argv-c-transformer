#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern float __VERIFIER_nondet_float(void);
extern int __VERIFIER_nondet_int(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  
  char __hret0[__HAVOC_STR_MAX];
  __VERIFIER_nondet_memory(__hret0, sizeof(__hret0));
  size_t __hret0_len = __VERIFIER_nondet_size_t();
  if (__hret0_len >= __HAVOC_STR_MAX) abort();
  __hret0[__hret0_len] = '\0';
  char *s = __hret0;
  int __hret1[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__hret1, sizeof(__hret1));
  int *b = __hret1;
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
