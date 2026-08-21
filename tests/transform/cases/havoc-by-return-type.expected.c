#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  char __havoc_buf0[__HAVOC_BLOCK_MAX];
  unsigned char __havoc_buf1[__HAVOC_BLOCK_MAX];
  int a = __VERIFIER_nondet_int();
  ;
  char *s = (char *)__havoc_cstring_fill(__havoc_buf0, __HAVOC_BLOCK_MAX);
  int *b = (int *)(__VERIFIER_nondet_memory(__havoc_buf1, __HAVOC_BLOCK_MAX), __havoc_buf1);
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
