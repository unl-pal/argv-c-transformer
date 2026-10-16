#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

struct Node {
  int value;
  struct Node *next;
};

struct Named {
  int id;
  char *name;
  char *aliases[2];
};

// A recursive type reaches a pointer past any depth bound: never harnessed.
int second_value(struct Node *head) {
  if (head->next == 0) return head->value;
  return head->next->value;
}

int name_len(struct Named *n) {
  int len = 0;
  while (n->name[len] != '\0') len++;
  return len + n->aliases[1][0];
}

int first_arg(char **argv) { return argv[0][0]; }





int main(void) {
  {
    struct Named __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0].id = __VERIFIER_nondet_int();
    char __h0_0[__HAVOC_ARRAY_ELEMS][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0].name = __havoc_cstring_fill(__h0_0[__i0], __HAVOC_STR_MAX);
    char __h0_1[__HAVOC_ARRAY_ELEMS][2][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      for (int __i1 = 0; __i1 < 2; ++__i1)
        __h0[__i0].aliases[__i1] = __havoc_cstring_fill(__h0_1[__i0][__i1], __HAVOC_STR_MAX);
    name_len(__h0);
  }
  {
    char *__h1[__HAVOC_ARRAY_ELEMS];
    char __h1_0[__HAVOC_ARRAY_ELEMS][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h1[__i0] = __havoc_cstring_fill(__h1_0[__i0], __HAVOC_STR_MAX);
    first_arg(__h1);
  }
  return 0;
}
