#include <stdlib.h>

char *lookup(int key);

// Never freed: stack storage.
int unfreed(int key) {
  char *s = lookup(key);
  return s[0];
}

void release(char *p) { free(p); }

// Freed only inside another in-file function, whose call is havocked away.
int freed_elsewhere(int key) {
  char *s = lookup(key);
  int c = s[0];
  release(s);
  return c;
}

// A different variable is freed.
int other_freed(int key) {
  char *s = lookup(key);
  char *t = malloc(4);
  int c = s[0];
  free(t);
  return c;
}
