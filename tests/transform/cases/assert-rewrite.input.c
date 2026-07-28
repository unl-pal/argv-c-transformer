#include <assert.h>
int add(int a, int b) {
  int r = a + b;
  assert(r >= a);
  return r;
}
