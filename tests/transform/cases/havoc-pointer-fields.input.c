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

struct Node *make_node(void);

int via_return(void) {
  struct Node *n = make_node();
  return n->value;
}
