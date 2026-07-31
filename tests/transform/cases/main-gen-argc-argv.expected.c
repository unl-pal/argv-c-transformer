#include <stdlib.h>
#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_ARRAY_ELEMS 8
#define __HAVOC_OPAQUE_BYTES 128
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

int helper(int x) {
  return x + 1;
}

int original_main(int argc, char *argv[]) {
  return __VERIFIER_nondet_int();
}

int main(void) {
  helper(__VERIFIER_nondet_int());
  int argc = __VERIFIER_nondet_int();
  if (argc < __HAVOC_ARGC_MIN || argc > __HAVOC_ARGC_MAX) abort();
  char *argv[__HAVOC_ARGC_MAX + 1];
  for (int i = 0; i < argc; i++)
    argv[i] = __havoc_cstring(__HAVOC_STR_MAX);
  argv[argc] = 0;
  original_main(argc, argv);
  return 0;
}
