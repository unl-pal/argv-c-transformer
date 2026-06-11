extern int __VERIFIER_nondet_int(void);

int pick(int n) {
  return n + __VERIFIER_nondet_int();
}

int main(void) {
  pick(__VERIFIER_nondet_int());
  return 0;
}
