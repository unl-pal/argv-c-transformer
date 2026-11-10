#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#include <stdlib.h>

struct Pair {
  int a;
  int b;
};



// Bulk-filled heap storage: the fill size comes from the element type, not sizeof(__hret0).
int bulk(void) {
  struct Pair *__hret0 = malloc(sizeof(struct Pair) * __HAVOC_ARRAY_ELEMS);
  __VERIFIER_nondet_memory(__hret0, sizeof(struct Pair) * __HAVOC_ARRAY_ELEMS);
  struct Pair *p = __hret0;
  int s = p->a + p->b;
  free(p);
  return s;
}

int main(void) {
  {
    bulk();
  }
  if (0) reach_error();
  return 0;
}
