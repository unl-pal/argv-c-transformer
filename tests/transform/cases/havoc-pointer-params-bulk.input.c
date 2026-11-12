struct Point {
  int x;
  int y;
};

// Bare pointer plus a length: the length is clamped to the block's element
// count so any index derived from it stays in bounds.
int sum(int *data, int len) {
  int total = 0;
  for (int i = 0; i < len; i++)
    total += data[i];
  return total;
}

// Declared bound survives in getOriginalType(), so the block is sized exactly.
int third(int fixed[3]) { return fixed[2]; }

// char* is a string, not a byte block.
int first_char(const char *s) { return s[0]; }

// No sizeof available: opaque byte block.
int opaque(void *p) { return p != 0; }

// Pointer to a record defined in this file, with no pointer fields.
int point_x(struct Point *p) { return p->x; }

// No pointer in the list, so the integer stays unconstrained.
int unbounded(int n) { return n + 1; }
