#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#include <stdlib.h>

int uses_stdlib(const char *s) {
  return (int)atoi(s);
}

int main(void) {
  char __h0[__HAVOC_STR_MAX];
  uses_stdlib(__havoc_cstring_fill(__h0, __HAVOC_STR_MAX));
  return 0;
}
