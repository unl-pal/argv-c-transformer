#include <stdlib.h>

struct Pair {
  int a;
  int b;
};

struct Pair *make_pair(void);

// Bulk-filled heap storage: the fill size comes from the element type, not sizeof(__hret0).
int bulk(void) {
  struct Pair *p = make_pair();
  int s = p->a + p->b;
  free(p);
  return s;
}
