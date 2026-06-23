extern float __VERIFIER_nondet_float(void);
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

extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = __VERIFIER_nondet_int();
  ;
  char *s = __havoc_cstring(128);
  int *b = __havoc_block(128);
  float f = __VERIFIER_nondet_float();
  return a + s[0] + b[0] + (int)f;
}

int main(void) {
  compute(__VERIFIER_nondet_int());
  return 0;
}
