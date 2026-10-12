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

// Pointer fields are set to 0 past the depth bound.
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



int via_return(void) {
  struct Node __hret0[__HAVOC_ARRAY_ELEMS];
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
    __hret0[__i0].value = __VERIFIER_nondet_int();
  struct Node __hret0_0[__HAVOC_ARRAY_ELEMS][__HAVOC_ARRAY_ELEMS];
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
    for (int __i1 = 0; __i1 < __HAVOC_ARRAY_ELEMS; ++__i1)
      __hret0_0[__i0][__i1].value = __VERIFIER_nondet_int();
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
    for (int __i1 = 0; __i1 < __HAVOC_ARRAY_ELEMS; ++__i1)
      __hret0_0[__i0][__i1].next = 0;
  for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
    __hret0[__i0].next = __hret0_0[__i0];
  struct Node *n = __hret0;
  return n->value;
}

int main(void) {
  {
    struct Node __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0].value = __VERIFIER_nondet_int();
    struct Node __h0_0[__HAVOC_ARRAY_ELEMS][__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      for (int __i1 = 0; __i1 < __HAVOC_ARRAY_ELEMS; ++__i1)
        __h0_0[__i0][__i1].value = __VERIFIER_nondet_int();
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      for (int __i1 = 0; __i1 < __HAVOC_ARRAY_ELEMS; ++__i1)
        __h0_0[__i0][__i1].next = 0;
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h0[__i0].next = __h0_0[__i0];
    second_value(__h0);
  }
  {
    struct Named __h1[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h1[__i0].id = __VERIFIER_nondet_int();
    char __h1_0[__HAVOC_ARRAY_ELEMS][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h1[__i0].name = __havoc_cstring_fill(__h1_0[__i0], __HAVOC_STR_MAX);
    char __h1_1[__HAVOC_ARRAY_ELEMS][2][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      for (int __i1 = 0; __i1 < 2; ++__i1)
        __h1[__i0].aliases[__i1] = __havoc_cstring_fill(__h1_1[__i0][__i1], __HAVOC_STR_MAX);
    name_len(__h1);
  }
  {
    char *__h2[__HAVOC_ARRAY_ELEMS];
    char __h2_0[__HAVOC_ARRAY_ELEMS][__HAVOC_STR_MAX];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h2[__i0] = __havoc_cstring_fill(__h2_0[__i0], __HAVOC_STR_MAX);
    first_arg(__h2);
  }
  {
    via_return();
  }
  return 0;
}
