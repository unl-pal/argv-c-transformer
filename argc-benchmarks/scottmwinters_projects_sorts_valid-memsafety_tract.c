// SPDX-FileCopyrightText: Copyright (C) 2025 The ARG-V Project

/*
 * Aug 27, 2025
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
 */

#include <stdio.h>
#include <stdlib.h>

static int SIZE = 3;

// Arg-C: Verification functions
// ----------------------------------
extern void abort();
void reach_error();

extern int __VERIFIER_nondet_int(void);
extern void __VERIFIER_assume(int expression);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}
// ----------------------------------

void unsort(int *p);
void bubble(int *p);

void unsort(int *p) {
  int i, j;
  for (i = SIZE; i > 0; i--) {  // i starts at SIZE → p[SIZE] is out-of-bounds
    j = i % SIZE;
    p[j] = p[i];
  }
}

void bubble(int *p) {
  int i, j;

  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      if (p[i] < p[j]) {
        p[i] = p[i] ^ p[j];
        p[j] = p[i] ^ p[j];
        p[i] = p[i] ^ p[j];
      }
    }
  }
}

// Arg-C verification harness
int main() {
  int bubble_list[SIZE];

  for (int i = 0; i < SIZE; i++) {
    bubble_list[i] = __VERIFIER_nondet_int();
    __VERIFIER_assume(bubble_list[i] >= -8 && bubble_list[i] <= 8);
  }

  bubble(bubble_list);
  unsort(bubble_list);  // out-of-bounds: p[SIZE] when i == SIZE

  return 0;
}
