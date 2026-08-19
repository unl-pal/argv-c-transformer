#include <stdlib.h>
#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
extern int __VERIFIER_nondet_int(void);
#define __HAVOC_BLOCK_MAX 128
extern void __VERIFIER_nondet_memory(void *, size_t);
extern size_t __VERIFIER_nondet_size_t(void);

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
  char __argv_buf[__HAVOC_ARGC_MAX][__HAVOC_STR_MAX];
  char *argv[__HAVOC_ARGC_MAX + 1];
  for (int i = 0; i < argc; i++) {
    __VERIFIER_nondet_memory(__argv_buf[i], __HAVOC_STR_MAX);
    size_t __argv_len = __VERIFIER_nondet_size_t();
    if (__argv_len >= __HAVOC_STR_MAX) abort();
    __argv_buf[i][__argv_len] = 0;
    argv[i] = __argv_buf[i];
  }
  argv[argc] = 0;
  original_main(argc, argv);
  return 0;
}
