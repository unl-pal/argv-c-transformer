#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#include <stdlib.h>

struct Buf {
  int len;
  int cap;
};

struct Holder {
  struct Buf *buf;
};





// Freed through the variable it initializes.
int plain(void) {
  struct Buf *__hret0 = malloc(sizeof(struct Buf) * __HAVOC_ARRAY_ELEMS);
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
    __hret0[__i0].len = __VERIFIER_nondet_int();
    __hret0[__i0].cap = __VERIFIER_nondet_int();
  }
  struct Buf *b = __hret0;
  int n = b->len;
  free(b);
  return n;
}

// Assigned inside a condition, freed later.
int in_condition(void) {
  char *s;
  char *__hret1 = malloc(sizeof(char) * __HAVOC_STR_MAX);
  if (!(s = __havoc_cstring_fill(__hret1, __HAVOC_STR_MAX))) return -1;
  int c = s[0];
  free(s);
  return c;
}

// Stored in a struct field and freed through it.
int field(void) {
  struct Holder h;
  struct Buf *__hret2 = malloc(sizeof(struct Buf) * __HAVOC_ARRAY_ELEMS);
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
    __hret2[__i0].len = __VERIFIER_nondet_int();
    __hret2[__i0].cap = __VERIFIER_nondet_int();
  }
  h.buf = __hret2;
  int n = h.buf->len;
  free(h.buf);
  return n;
}

// The same through a pointer to the struct.
int field_arrow(struct Holder *h) {
  struct Buf *__hret3 = malloc(sizeof(struct Buf) * __HAVOC_ARRAY_ELEMS);
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
    __hret3[__i0].len = __VERIFIER_nondet_int();
    __hret3[__i0].cap = __VERIFIER_nondet_int();
  }
  h->buf = __hret3;
  int n = h->buf->len;
  free(h->buf);
  return n;
}

// Freed through a local copy.
int alias(void) {
  unsigned char *__hret4 = malloc(sizeof(unsigned char) * __HAVOC_BLOCK_MAX);
  __VERIFIER_nondet_memory(__hret4, sizeof(unsigned char) * __HAVOC_BLOCK_MAX);
  void *p = (void *)__hret4;
  unsigned char *q = p;
  int c = q[0];
  free(q);
  return c;
}

// Grown with realloc.
int grow(int n) {
  char *__hret5 = malloc(sizeof(char) * __HAVOC_STR_MAX);
  char *s = __havoc_cstring_fill(__hret5, __HAVOC_STR_MAX);
  s = realloc(s, n + 1);
  int c = s[0];
  free(s);
  return c;
}

int main(void) {
  {
    plain();
  }
  {
    in_condition();
  }
  {
    field();
  }
  {
    alias();
  }
  {
    grow(__VERIFIER_nondet_int());
  }
  if (0) reach_error();
  return 0;
}
