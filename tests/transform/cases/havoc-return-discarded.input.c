int *acquire(void);

// Pointer-returning call whose value is discarded: no handle survives on the
// block, so it needs no storage and is dropped like a void call. The rest of
// the body keeps the function harnessable.
int discard(int n) {
  acquire();
  return n + 1;
}
