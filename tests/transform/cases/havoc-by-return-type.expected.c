#include "argv_c_runtime.h"

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  ;
  char *s = (char *)__HAVOC_CSTRING();
  int *b = (int *)__HAVOC_BLOCK();
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
