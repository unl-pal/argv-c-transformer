struct Point {
  int x;
  int y;
};

struct Point make_point(void) {
  struct Point p = {1, 2};
  return p;
}

int caller(void) {
  struct Point p = make_point();
  return p.x;
}

int untouched(void) {
  return 42;
}
