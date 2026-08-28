#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

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
