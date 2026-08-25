#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

int *acquire(void);

// Pointer-returning call whose value is discarded: no handle survives on the
// block, so it needs no storage and is dropped like a void call. The rest of
// the body keeps the function harnessable.
int discard(int n) {
  
  return n + 1;
}

int main(void) {
  discard(__VERIFIER_nondet_int());
  return 0;
}
