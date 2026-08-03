#include <assert.h>
extern int __VERIFIER_nondet_int(void);
void reach_error(void) { assert(0); }

int add(int a, int b) {
  int r = a + b;
  if (!(r >= a)) reach_error();
  return r;
}

int main(void) {
  add(__VERIFIER_nondet_int(), __VERIFIER_nondet_int());
  return 0;
}
