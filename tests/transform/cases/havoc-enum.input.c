// An enum is havocked as its underlying integer type, as a parameter and as a
// call's return, and is not clamped alongside a pointer.
enum color { RED, GREEN, BLUE };

enum color pick(int seed) { return seed > 0 ? GREEN : RED; }

int is_red(enum color c) { return c == RED; }

int count(const int *cells, enum color c, int n) {
  enum color other = pick(n);
  return cells[n] + (c == other);
}
