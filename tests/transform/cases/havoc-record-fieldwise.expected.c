#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

enum Color { RED, GREEN };

struct Rect {
  int w;
  int h;
};

struct Shape {
  enum Color color;
  unsigned flags : 3;
  struct Rect box;
  short samples[2];
  char *label;
};

struct Tagged {
  int kind;
  union {
    int i;
    float f;
  } as;
};

struct Fixed {
  const int id;
  int value;
};

struct Wide {
  long double x;
};

int area(struct Rect *r) { return r->w * r->h; }

int pick(struct Rect rs[3]) { return rs[2].w; }

int shape_w(struct Shape *s) { return s->box.w + s->samples[1] + s->label[0]; }

// Not every field has a nondet suffix, so these are bulk-filled.
int tagged(struct Tagged *t) { return t->kind; }

int fixed(struct Fixed *f) { return f->id + f->value; }

int wide(struct Wide *w) { return w->x > 0; }

int main(void) {
  {
    struct Rect __h0[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __h0[__i0].w = __VERIFIER_nondet_int();
      __h0[__i0].h = __VERIFIER_nondet_int();
    }
    area(__h0);
  }
  {
    struct Rect __h1[3];
    for (int __i0 = 0; __i0 < 3; ++__i0) {
      __h1[__i0].w = __VERIFIER_nondet_int();
      __h1[__i0].h = __VERIFIER_nondet_int();
    }
    pick(__h1);
  }
  {
    struct Tagged __h2[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __h2[__i0].kind = __VERIFIER_nondet_int();
      __VERIFIER_nondet_memory((void *)&__h2[__i0].as, sizeof(__h2[__i0].as));
    }
    tagged(__h2);
  }
  {
    struct Fixed __h3[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __VERIFIER_nondet_memory((void *)&__h3[__i0].id, sizeof(__h3[__i0].id));
      __h3[__i0].value = __VERIFIER_nondet_int();
    }
    fixed(__h3);
  }
  {
    struct Wide __h4[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h4, sizeof(__h4));
    wide(__h4);
  }
  if (0) reach_error();
  return 0;
}
