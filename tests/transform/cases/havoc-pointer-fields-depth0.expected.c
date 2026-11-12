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

// At depth 0 storage is typed but pointers inside it are left as nondet bytes,
// so recursive types are harnessed.
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
  __VERIFIER_nondet_memory(__hret0, sizeof(__hret0));
  struct Node *n = __hret0;
  return n->value;
}

// Array params keep their declared bound; declarators nest inside it.
int grid(int g[3][4]) { return g[2][3]; }
int any_op(int (*fs[2])(int)) { return fs[0] != 0; }

// The clamp bound matches the element count of the typed storage.
int sum_values(struct Node *nodes, int n) {
  int s = 0;
  for (int i = 0; i < n; i++) s += nodes[i].value;
  return s;
}

int main(void) {
  {
    struct Node __h0[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h0, sizeof(__h0));
    second_value(__h0);
  }
  {
    struct Named __h1[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h1, sizeof(__h1));
    name_len(__h1);
  }
  {
    char *__h2[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h2, sizeof(__h2));
    first_arg(__h2);
  }
  {
    via_return();
  }
  {
    int __h3[3][4];
    __VERIFIER_nondet_memory(__h3, sizeof(__h3));
    grid(__h3);
  }
  {
    int (*__h4[2])(int);
    __VERIFIER_nondet_memory(__h4, sizeof(__h4));
    any_op(__h4);
  }
  {
    struct Node __h5[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h5, sizeof(__h5));
    int __h6 = __VERIFIER_nondet_int();
    if (__h6 < 0 || __h6 >= __HAVOC_ARRAY_ELEMS) abort();
    sum_values(__h5, __h6);
  }
  if (0) reach_error();
  return 0;
}
