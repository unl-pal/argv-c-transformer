#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern int __VERIFIER_nondet_int(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

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
  int __h0[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__h0, sizeof(__h0));
  int __h1 = __VERIFIER_nondet_int();
  if (__h1 < 0 || __h1 > __HAVOC_ARRAY_ELEMS) abort();
  sum(__h0, __h1);
  int __h2[3];
  __VERIFIER_nondet_memory(__h2, sizeof(__h2));
  third(__h2);
  char __h3[__HAVOC_STR_MAX];
  __VERIFIER_nondet_memory(__h3, sizeof(__h3));
  size_t __h3_len = __VERIFIER_nondet_size_t();
  if (__h3_len >= __HAVOC_STR_MAX) abort();
  __h3[__h3_len] = '\0';
  first_char(__h3);
  _Alignas(16) unsigned char __h4[__HAVOC_OPAQUE_BYTES];
  __VERIFIER_nondet_memory(__h4, sizeof(__h4));
  opaque((void *)__h4);
  struct Point __h5[__HAVOC_ARRAY_ELEMS];
  __VERIFIER_nondet_memory(__h5, sizeof(__h5));
  point_x(__h5);
  unbounded(__VERIFIER_nondet_int());
  return 0;
}
