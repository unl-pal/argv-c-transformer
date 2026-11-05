#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

struct Point {
  int x;
  int y;
};

// Bare pointer plus a length: the length is clamped to the block's element
// count so any index derived from it stays in bounds.
int sum(int *data, int len) {
  int total = 0;
  for (int i = 0; i < len; i++)
    total += data[i];
  return total;
}

// Declared bound survives in getOriginalType(), so the block is sized exactly.
int third(int fixed[3]) { return fixed[2]; }

// char* is a string, not a byte block.
int first_char(const char *s) { return s[0]; }

// No sizeof available: opaque byte block.
int opaque(void *p) { return p != 0; }

// Pointer to a record defined in this file, with no pointer fields.
int point_x(struct Point *p) { return p->x; }

// No pointer in the list, so the integer stays unconstrained.
int unbounded(int n) { return n + 1; }

int main(void) {
  {
    int __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0] = __VERIFIER_nondet_int();
    int __h1 = __VERIFIER_nondet_int();
    if (__h1 < 0 || __h1 > __HAVOC_ARRAY_ELEMS) abort();
    sum(__h0, __h1);
  }
  {
    int __h2[3];
    for (int __i0 = 0; __i0 < 3; ++__i0)
      __h2[__i0] = __VERIFIER_nondet_int();
    third(__h2);
  }
  {
    char __h3[__HAVOC_STR_MAX];
    first_char(__havoc_cstring_fill(__h3, __HAVOC_STR_MAX));
  }
  {
    unsigned char __h4[__HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h4, sizeof(__h4));
    opaque((void *)__h4);
  }
  {
    struct Point __h5[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __h5[__i0].x = __VERIFIER_nondet_int();
      __h5[__i0].y = __VERIFIER_nondet_int();
    }
    point_x(__h5);
  }
  {
    unbounded(__VERIFIER_nondet_int());
  }
  return 0;
}
