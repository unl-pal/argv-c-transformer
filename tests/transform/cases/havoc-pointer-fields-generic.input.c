// Generic: every pointer, strings and argv included, is an opaque block.
struct Node {
  int value;
  struct Node *next;
};

struct Named {
  int id;
  char *name;
  char *aliases[2];
};

// Pointer fields stay nondet bytes.
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

// Array params lose their bound.
int grid(int g[3][4]) { return g[2][3]; }
int any_op(int (*fs[2])(int)) { return fs[0] != 0; }

char *get_name(void);

int name_head(void) {
  char *s = get_name();
  return s[0];
}

int main(int argc, char **argv) { return argc > 1 ? argv[1][0] : 0; }

// Integers beside a pointer are not clamped.
int sum(int *a, int n) {
  int s = 0;
  for (int i = 0; i < n; i++) s += a[i];
  return s;
}
