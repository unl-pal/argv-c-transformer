#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

// An anonymous union has no name to fill through: its record is filled whole
// first, and the typed fills of the named fields follow.
struct tagged {
  int kind;
  double weight;
  union {
    int i;
    float f;
  };
  short tail;
};

int kind_of(const struct tagged *t) { return t->kind + t->tail; }

int main(void) {
  {
    struct tagged __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __VERIFIER_nondet_memory((void *)&__h0[__i0], sizeof(__h0[__i0]));
      __h0[__i0].kind = __VERIFIER_nondet_int();
      __h0[__i0].weight = __VERIFIER_nondet_double();
      __h0[__i0].tail = __VERIFIER_nondet_short();
    }
    kind_of(__h0);
  }
  if (0) reach_error();
  return 0;
}
