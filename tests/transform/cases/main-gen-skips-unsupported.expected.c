#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern int __VERIFIER_nondet_int(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

int takesPtr(int *p) {
  return *p;
}

int takesVarargs(int a, ...) {
  return a;
}

int plain(int a) {
  return a;
}

int main(void) {
  int __h0[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__h0, sizeof(__h0));
  takesPtr(__h0);
  plain(__VERIFIER_nondet_int());
  return 0;
}
