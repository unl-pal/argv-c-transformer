extern int __VERIFIER_nondet_int(void);

int helper(void);
void voidhelper(void);

int ticks;
int tick(void);

void run(void) {
  
  int x = __VERIFIER_nondet_int();
  
  
  
  /* This loop terminates in the input: `tick` advances the global the
   * condition reads. Havocking the increment destroys that, so erasing the
   * increment but keeping the loop would leave `for (; ticks < 10; ) ;` -
   * nontermination manufactured by the transform. Pruning the whole loop is
   * what keeps that from happening. */
  
  int y = (__VERIFIER_nondet_int() + __VERIFIER_nondet_int());
  /* Dead, but the author wrote it and we never rewrote any part of it: it
   * survives untouched. */
  x;
}

int main(void) {
  run();
  return 0;
}
