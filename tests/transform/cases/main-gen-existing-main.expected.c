extern int __VERIFIER_nondet_int(void);

int helper(void) {
  return 7;
}

/* keep me */
int original_main() {
  int x = __VERIFIER_nondet_int();
  return x;
}

int main(void) {
  helper();
  original_main();
  return 0;
}
