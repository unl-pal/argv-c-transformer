// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

/*
 * May 14, 2026
 * Written by Nathanael Steven and the ARG-V development team for use as
 * a benchmark for Static Verification tools
 */

#include <stdlib.h>

extern void abort();
void reach_error();

extern int __VERIFIER_nondet_int(void);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

int SIZE = 4;

void insertion(int *array) {
  for (int i = 0; i < SIZE; i++) {
    int val = array[i];
    int j = i - 1;
    while (j >= 0 && array[j] > val) {
      array[j + 1] = array[j];
      j--;
    }
    array[j + 1] = val;
  }
}

// Arg-C verification harness
int main() {
  int insertion_list[SIZE];

  for (int i = 0; i < SIZE; i++) {
    int num = __VERIFIER_nondet_int();
    insertion_list[i] = num;
  }

  insertion(insertion_list);

  for (int i = 0; i < SIZE - 1; i++) {
    __VERIFIER_assert(insertion_list[i] <= insertion_list[i + 1]);
  }

  return 0;
}
