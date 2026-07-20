void quiet(void);

/* The loop body is a dropped void call, but the condition mutates x, which
 * is observed after the loop — the loop must survive pruning. */
int drain(int x) {
  while (x-- > 0)
    quiet();
  return x;
}
