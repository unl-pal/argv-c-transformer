#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"
struct Named;
struct Node;

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
  unsigned char __hret0[sizeof(struct Node) > __HAVOC_BLOCK_MAX ? sizeof(struct Node) : __HAVOC_BLOCK_MAX];
  __VERIFIER_nondet_memory(__hret0, sizeof(__hret0));
  struct Node *n = (struct Node *)__hret0;
  return n->value;
}

// Array declarators nest, so the sizeof floor is spelled by clang.
int grid(int g[3][4]) { return g[2][3]; }
int any_op(int (*fs[2])(int)) { return fs[0] != 0; }

int main(void) {
  {
    unsigned char __h0[sizeof(struct Node) > __HAVOC_BLOCK_MAX ? sizeof(struct Node) : __HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h0, sizeof(__h0));
    second_value((struct Node *)__h0);
  }
  {
    unsigned char __h1[sizeof(struct Named) > __HAVOC_BLOCK_MAX ? sizeof(struct Named) : __HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h1, sizeof(__h1));
    name_len((struct Named *)__h1);
  }
  {
    unsigned char __h2[sizeof(char *) > __HAVOC_BLOCK_MAX ? sizeof(char *) : __HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h2, sizeof(__h2));
    first_arg((char **)__h2);
  }
  {
    via_return();
  }
  {
    unsigned char __h3[sizeof(int[3][4]) > __HAVOC_BLOCK_MAX ? sizeof(int[3][4]) : __HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h3, sizeof(__h3));
    grid((int (*)[4])__h3);
  }
  {
    unsigned char __h4[sizeof(int (*[2])(int)) > __HAVOC_BLOCK_MAX ? sizeof(int (*[2])(int)) : __HAVOC_BLOCK_MAX];
    __VERIFIER_nondet_memory(__h4, sizeof(__h4));
    any_op((int (**)(int))__h4);
  }
  return 0;
}
