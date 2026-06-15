// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

/*
 * May 14, 2026
 * Written by Nathanael Steven and the ARG-V development team for use as
 * a benchmark for Static Verification tools
 */

#include <stdlib.h>
#include <assert.h>

extern void abort();
void reach_error() { assert(0); }

extern int __VERIFIER_nondet_int(void);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

static int SIZE = 5;

void xor_swap(int *a, int *b) {
  *a ^= *b;
  *b ^= *a;
  *a ^= *b;
}

void bubble(int *array) {
  int swapped;
  for (int i = 0; i < SIZE - 1; i++) {
    swapped = 0;
    for (int j = 0; j < SIZE - i - 1; j++) {
      if (array[j] > array[j + 1]) {
        xor_swap(&array[j], &array[j + 1]);
        swapped = 1;
      }
    }
    if (!swapped) break;
  }
}

// Arg-C verification harness
int main() {
  int bubble_list[SIZE];

  for (int i = 0; i < SIZE; i++) {
    int num = __VERIFIER_nondet_int();
    bubble_list[i] = num;
  }

  // bubble sort is ascending
  bubble(bubble_list);
  for (int i = 0; i < SIZE - 1; i++) {
    __VERIFIER_assert(bubble_list[i] <= bubble_list[i + 1]);
  }

  return 0;
}
