#define EXPECT(x) __builtin_expect((x), 1)
int helper(int x) { return x * 2; }

// Compiler builtins have no declaring header but are not in-file calls: left alone,
// while a call in their arguments is still havocked.
int direct(int v) {
  if (__builtin_expect(helper(v), 1)) return 1;
  return 0;
}

int via_macro(int v) {
  if (EXPECT(v > 0)) return 1;
  return 0;
}
