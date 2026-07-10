

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

int run(int n) {
  char *s = (char *)__havoc_cstring(128);
  return __VERIFIER_nondet_int() + s[0];
}

int main(void) {
  run(__VERIFIER_nondet_int());
  return 0;
}
