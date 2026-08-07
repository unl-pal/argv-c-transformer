#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern void __VERIFIER_nondet_memory(void *, size_t);

int *acquire(void);

// Pointer-returning call inside an if condition: the storage must hoist above
// the whole `if`, and the call is replaced in place.
int cond_if(void) {
  int __hret0[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__hret0, sizeof(__hret0));
  if (__hret0[0])
    return 1;
  return 0;
}

// Same for a while condition, a distinct anchor.
int cond_while(void) {
  int __hret1[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__hret1, sizeof(__hret1));
  while (__hret1[0])
    return 2;
  return 0;
}

int main(void) {
  cond_if();
  cond_while();
  return 0;
}
