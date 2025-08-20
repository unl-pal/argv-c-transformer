#include <stdio.h>

#ifdef _WIN32
typedef unsigned __int64 nio_ullong;
#else
typedef unsigned long long nio_ullong;
#endif

extern void abort();
void reach_error();

void __VERIFIER_assert(int cond) { if(!cond) { reach_error(); abort(); } }

typedef union {
  double d;
  struct {
    nio_ullong bogus0 : 60;
    unsigned int bogus1 : 2;
    unsigned int bogus2 : 2;
  } c;
} nio_test;

int main() {
  nio_test verifyUnion;
  verifyUnion.d = 0.0;
  __VERIFIER_assert(!verifyUnion.c.bogus0 && !verifyUnion.c.bogus1 && !verifyUnion.c.bogus2);
  verifyUnion.d = -1.0;
  // __VERIFIER_assert(verifyUnion.c.bogus2 == (unsigned int)1 || verifyUnion.c.bogus1 == (unsigned int)1);
  printf("hello, world.\n");
  return 0;
  __VERIFIER_assert(0);
}

