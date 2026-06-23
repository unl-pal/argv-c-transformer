#include <stdbool.h>

extern int __VERIFIER_nondet_int(void);

bool flag(int n) {
  return n > 0;
}

int main(void) {
  flag(__VERIFIER_nondet_int());
  return 0;
}
