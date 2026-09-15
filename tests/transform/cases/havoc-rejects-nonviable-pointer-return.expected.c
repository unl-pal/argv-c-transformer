#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

typedef int (*op_t)(int);

int add_one(int x) { return x + 1; }

op_t get_op(void) {
  return add_one;
}



int untouched(void) {
  return 42;
}

int main(void) {
  add_one(__VERIFIER_nondet_int());
  get_op();
  untouched();
  return 0;
}
