// Unnamed bitfields are padding: the fieldwise fill skips them.
struct box {
  unsigned int opaque : 1;
  unsigned int : 4;
  unsigned int color : 3;
};

int is_opaque(const struct box *b) { return b->opaque; }
