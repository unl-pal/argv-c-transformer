void log_step(void);
void log_other(void);

/* if/else, while, and for whose bodies are only dropped void calls, with
 * side-effect-free conditions: pruned entirely. The for loop's `i++`
 * mutates a variable declared in its own init clause, which dies with the
 * loop, so it does not count as an observable side effect. */
int busy(int n) {
  if (n > 0) {
    log_step();
  } else {
    log_other();
  }
  while (n > 100)
    log_step();
  for (int i = 0; i < n; i++)
    log_step();
  return n;
}

/* The increment targets a variable declared outside the loop, observed
 * after it: a real side effect, so the loop is kept. */
int outer_counter(int n) {
  int i = 0;
  for (; i < n; i++)
    log_step();
  return i;
}

/* Body collapses entirely to no-ops: stripped to `;` and not harnessed. */
void all_logging(int n) {
  if (n)
    log_step();
}
