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
