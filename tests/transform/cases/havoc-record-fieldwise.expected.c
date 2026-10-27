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
    struct Shape __h2[__HAVOC_ARRAY_ELEMS];
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0) {
      __h2[__i0].color = __VERIFIER_nondet_uint();
      __h2[__i0].flags = __VERIFIER_nondet_uint();
      __h2[__i0].box.w = __VERIFIER_nondet_int();
      __h2[__i0].box.h = __VERIFIER_nondet_int();
      for (int __i1 = 0; __i1 < 2; ++__i1)
        __h2[__i0].samples[__i1] = __VERIFIER_nondet_short();
    }
    for (int __i0 = 0; __i0 < __HAVOC_ARRAY_ELEMS; ++__i0)
      __h2[__i0].label = 0;
    shape_w(__h2);
  }
  {
    struct Tagged __h3[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h3, sizeof(__h3));
    tagged(__h3);
  }
  {
    struct Fixed __h4[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h4, sizeof(__h4));
    fixed(__h4);
  }
  {
    struct Wide __h5[__HAVOC_ARRAY_ELEMS];
    __VERIFIER_nondet_memory(__h5, sizeof(__h5));
    wide(__h5);
  }
  return 0;
}
