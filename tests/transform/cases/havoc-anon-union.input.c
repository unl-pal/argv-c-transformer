// An anonymous union has no name to fill through: its record is filled whole
// first, and the typed fills of the named fields follow.
struct tagged {
  int kind;
  double weight;
  union {
    int i;
    float f;
  };
  short tail;
};

int kind_of(const struct tagged *t) { return t->kind + t->tail; }
