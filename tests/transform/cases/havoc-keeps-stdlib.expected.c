#include <string.h>

#include <stdlib.h>
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
extern size_t __VERIFIER_nondet_size_t(void);
extern void __VERIFIER_nondet_memory(void *, size_t);

int uses_stdlib(const char *s) {
  return (int)strlen(s);
}

int main(void) {
  char __h0[__HAVOC_STR_MAX];
  __VERIFIER_nondet_memory(__h0, sizeof(__h0));
  size_t __h0_len = __VERIFIER_nondet_size_t();
  if (__h0_len >= __HAVOC_STR_MAX) abort();
  __h0[__h0_len] = '\0';
  uses_stdlib(__h0);
  return 0;
}
