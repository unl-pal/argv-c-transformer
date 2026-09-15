int *acquire(void);

// Pointer-returning call inside an if condition: the storage must hoist above
// the whole `if`, and the call is replaced in place.
int cond_if(void) {
  if (acquire()[0])
    return 1;
  return 0;
}

// Same for a while condition, a distinct anchor.
int cond_while(void) {
  while (acquire()[0])
    return 2;
  return 0;
}
