#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

struct Point {
  int x;
  int y;
};

struct Point make_point(void) {
  struct Point p = {1, 2};
  return p;
}



int untouched(void) {
  return 42;
}

int main(void) {
  make_point();
  untouched();
  return 0;
}
