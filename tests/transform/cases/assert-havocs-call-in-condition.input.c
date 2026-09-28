#include <assert.h>
int helper(int x) { return x * 2; }
int check(int v) {
  assert(helper(v) > 0);
  return v;
}
