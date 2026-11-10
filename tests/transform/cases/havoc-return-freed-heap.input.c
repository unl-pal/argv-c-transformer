#include <stdlib.h>

struct Buf {
  int len;
  int cap;
};

struct Holder {
  struct Buf *buf;
};

struct Buf *make_buf(void);
char *make_str(void);
void *make_block(void);

// Freed through the variable it initializes.
int plain(void) {
  struct Buf *b = make_buf();
  int n = b->len;
  free(b);
  return n;
}

// Assigned inside a condition, freed later.
int in_condition(void) {
  char *s;
  if (!(s = make_str())) return -1;
  int c = s[0];
  free(s);
  return c;
}

// Stored in a struct field and freed through it.
int field(void) {
  struct Holder h;
  h.buf = make_buf();
  int n = h.buf->len;
  free(h.buf);
  return n;
}

// The same through a pointer to the struct.
int field_arrow(struct Holder *h) {
  h->buf = make_buf();
  int n = h->buf->len;
  free(h->buf);
  return n;
}

// Freed through a local copy.
int alias(void) {
  void *p = make_block();
  unsigned char *q = p;
  int c = q[0];
  free(q);
  return c;
}

// Grown with realloc.
int grow(int n) {
  char *s = make_str();
  s = realloc(s, n + 1);
  int c = s[0];
  free(s);
  return c;
}
