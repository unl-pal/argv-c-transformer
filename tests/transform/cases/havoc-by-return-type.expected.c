#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern float __VERIFIER_nondet_float(void);
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

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  
  char *s = (char *)__havoc_cstring(__HAVOC_STR_MAX);
  int *b = (int *)__havoc_block(sizeof(int) * __HAVOC_ARRAY_ELEMS);
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
