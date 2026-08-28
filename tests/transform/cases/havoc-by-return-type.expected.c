#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  
  char __hret0[__HAVOC_STR_MAX];
  char *s = __havoc_cstring_fill(__hret0, __HAVOC_STR_MAX);
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
