#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
struct Point;
extern int __VERIFIER_nondet_int(void);
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);
static void *__havoc_block(size_t size) {
  void *block = malloc(size);
  __VERIFIER_nondet_memory(block, size);
  return block;
}
static char *__havoc_cstring(size_t size) {
  char *s = __havoc_block(size);
  size_t len = __VERIFIER_nondet_size_t();
  if (len >= size) abort();
  s[len] = '\0';
  return s;
}

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
  int __h0 = __VERIFIER_nondet_int();
  if (__h0 < 0 || __h0 > __HAVOC_ARRAY_ELEMS) abort();
  sum((int *)__havoc_block(sizeof(int) * __HAVOC_ARRAY_ELEMS), __h0);
  third((int *)__havoc_block(sizeof(int) * 3));
  first_char((const char *)__havoc_cstring(__HAVOC_STR_MAX));
  opaque((void *)__havoc_block(__HAVOC_OPAQUE_BYTES));
  point_x((struct Point *)__havoc_block(sizeof(struct Point) * __HAVOC_ARRAY_ELEMS));
  unbounded(__VERIFIER_nondet_int());
  return 0;
}
