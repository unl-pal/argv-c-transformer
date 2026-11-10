#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#include <stdlib.h>



// Never freed: stack storage.
int unfreed(int key) {
  char __hret0[__HAVOC_STR_MAX];
  char *s = __havoc_cstring_fill(__hret0, __HAVOC_STR_MAX);
  return s[0];
}

void release(char *p) { free(p); }

// Freed only inside another in-file function, whose call is havocked away.
int freed_elsewhere(int key) {
  char __hret1[__HAVOC_STR_MAX];
  char *s = __havoc_cstring_fill(__hret1, __HAVOC_STR_MAX);
  int c = s[0];
  
  return c;
}

// A different variable is freed.
int other_freed(int key) {
  char __hret2[__HAVOC_STR_MAX];
  char *s = __havoc_cstring_fill(__hret2, __HAVOC_STR_MAX);
  char *t = malloc(4);
  int c = s[0];
  free(t);
  return c;
}

int main(void) {
  {
    unfreed(__VERIFIER_nondet_int());
  }
  {
    char __h0[__HAVOC_STR_MAX];
    release(__havoc_cstring_fill(__h0, __HAVOC_STR_MAX));
  }
  {
    freed_elsewhere(__VERIFIER_nondet_int());
  }
  {
    other_freed(__VERIFIER_nondet_int());
  }
  if (0) reach_error();
  return 0;
}
