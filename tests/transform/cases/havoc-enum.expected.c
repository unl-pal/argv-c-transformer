#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

// An enum is havocked as its underlying integer type, as a parameter and as a
// call's return, and is not clamped alongside a pointer.
enum color { RED, GREEN, BLUE };

enum color pick(int seed) { return seed > 0 ? GREEN : RED; }

int is_red(enum color c) { return c == RED; }

int count(const int *cells, enum color c, int n) {
  enum color other = __VERIFIER_nondet_uint();
  return cells[n] + (c == other);
}

int main(void) {
  {
    pick(__VERIFIER_nondet_int());
  }
  {
    is_red(__VERIFIER_nondet_uint());
  }
  {
    int __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0] = __VERIFIER_nondet_int();
    int __h1 = __VERIFIER_nondet_int();
    if (__h1 < 0 || __h1 >= __HAVOC_ARRAY_ELEMS) abort();
    count(__h0, __VERIFIER_nondet_uint(), __h1);
  }
  if (0) reach_error();
  return 0;
}
