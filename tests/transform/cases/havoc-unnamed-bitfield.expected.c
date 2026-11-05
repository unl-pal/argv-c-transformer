#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

// Unnamed bitfields are padding: the fieldwise fill skips them.
struct box {
  unsigned int opaque : 1;
  unsigned int : 4;
  unsigned int color : 3;
};

int is_opaque(const struct box *b) { return b->opaque; }

int main(void) {
  {
    struct box __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __h0[__i0].opaque = __VERIFIER_nondet_uint();
      __h0[__i0].color = __VERIFIER_nondet_uint();
    }
    is_opaque(__h0);
  }
  return 0;
}
