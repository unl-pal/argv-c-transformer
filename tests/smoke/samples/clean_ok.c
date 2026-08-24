/* Baseline sanity file: no quirky constructs, should always survive the
 * pipeline. If this ever stops producing a benchmark, the pipeline itself is
 * broken (wrong clang, broken build, etc.) rather than anything quirk-specific. */
#include <stdio.h>

int compute(int a, int b) {
  int total = 0;
  for (int i = 0; i < b; i++) {
    total += a;
  }
  printf("%d\n", total);
  return total;
}
