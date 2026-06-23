extern int __VERIFIER_nondet_int(void);
extern void __VERIFIER_nondet_memory(void *, unsigned long);
extern void *malloc(unsigned long);
static void *__havoc_block(unsigned long size) {
  void *block = malloc(size);
  __VERIFIER_nondet_memory(block, size);
  return block;
}
static char *__havoc_cstring(unsigned long size) {
  char *s = __havoc_block(size);
  s[size - 1] = '\0';
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
  extern void abort(void);
  int argc = __VERIFIER_nondet_int();
  if (argc < 0 || argc > 7) abort();
  char *argv[argc + 1];
  for (int i = 0; i < argc; i++)
    argv[i] = __havoc_cstring(16);
  argv[argc] = 0;
  original_main(argc, argv);
  return 0;
}
