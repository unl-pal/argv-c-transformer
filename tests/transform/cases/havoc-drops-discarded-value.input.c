int helper(void);
void voidhelper(void);

int ticks;
int tick(void);

void run(void) {
  helper();
  int x = helper();
  (void)helper();
  if (x)
    helper();
  else
    voidhelper();
  for (int i = 0; i < 10; helper())
    voidhelper();
  /* This loop terminates in the input: `tick` advances the global the
   * condition reads. Havocking the increment destroys that, so erasing the
   * increment but keeping the loop would leave `for (; ticks < 10; ) ;` -
   * nontermination manufactured by the transform. Pruning the whole loop is
   * what keeps that from happening. */
  for (; ticks < 10; tick())
    voidhelper();
  int y = (helper() + helper());
  /* Dead, but the author wrote it and we never rewrote any part of it: it
   * survives untouched. */
  x;
}
