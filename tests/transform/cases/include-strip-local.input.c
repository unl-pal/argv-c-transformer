#include "local.h"

int run(int n) {
  char *s = name_fn();
  return helper_fn(n) + s[0];
}
