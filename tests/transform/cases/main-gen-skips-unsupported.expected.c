extern int __VERIFIER_nondet_int(void);

int takesPtr(int *p) {
  return *p;
}

int takesVarargs(int a, ...) {
  return a;
}

int plain(int a) {
  return a;
}

int main(void) {
  plain(__VERIFIER_nondet_int());
  return 0;
}
