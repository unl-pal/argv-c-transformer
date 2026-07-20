extern int __VERIFIER_nondet_int(void);

void quiet(void);

/* The loop body is a dropped void call, but the condition mutates x, which
 * is observed after the loop — the loop must survive pruning. */
int drain(int x) {
  while (x-- > 0)
    ;
  return x;
}

int main(void) {
  drain(__VERIFIER_nondet_int());
  return 0;
}
