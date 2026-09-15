typedef int (*op_t)(int);

int add_one(int x) { return x + 1; }

op_t get_op(void) {
  return add_one;
}

int use_op(int n) {
  op_t op = get_op();
  return op(n);
}

int untouched(void) {
  return 42;
}
