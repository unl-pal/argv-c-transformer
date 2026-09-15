#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

void quiet(void);

/* The loop body is a dropped void call, but the condition mutates x, which
 * is observed after the loop - the loop must survive pruning. */
int drain(int x) {
  while (x-- > 0)
    ;
  return x;
}

int main(void) {
  drain(__VERIFIER_nondet_int());
  return 0;
}
