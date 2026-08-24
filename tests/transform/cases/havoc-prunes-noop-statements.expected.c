#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

void log_step(void);
void log_other(void);

/* if/else, while, and for whose bodies are only dropped void calls, with
 * side-effect-free conditions: pruned entirely. The for loop's `i++`
 * mutates a variable declared in its own init clause, which dies with the
 * loop, so it does not count as an observable side effect. */
int busy(int n) {
  
  ;
  ;
  return n;
}

/* The increment targets a variable declared outside the loop, observed
 * after it: a real side effect, so the loop is kept. */
int outer_counter(int n) {
  int i = 0;
  for (; i < n; i++)
    ;
  return i;
}

/* Body collapses entirely to no-ops: stripped to `;` and not harnessed. */
void all_logging(int n) ;

int main(void) {
  busy(__VERIFIER_nondet_int());
  outer_counter(__VERIFIER_nondet_int());
  return 0;
}
