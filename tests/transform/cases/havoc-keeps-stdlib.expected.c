#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

#include <string.h>

int uses_stdlib(const char *s) {
  return (int)strlen(s);
}

int main(void) {
  return 0;
}
