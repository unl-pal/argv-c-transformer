#include <sys/types.h>
#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#include "argv_c_harness.h"

void set_mode(void) {
  mode_t m = 0;
  int x = 1;
}

int main(void) {
  set_mode();
  return 0;
}
