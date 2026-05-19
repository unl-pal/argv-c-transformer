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

static int SIZE = 15;

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

void selection(int *array) {
  for (int i = 0; i < SIZE - 1; i++) {
    int min = i;
    for (int j = i + 1; j < SIZE; j++)
      if (array[j] < array[min])
        min = j;
    swap(&array[i], &array[min]);
  }
}

// Arg-C verification harness
int main() {
  int selection_list[SIZE];

  for (int i = 0; i < SIZE; i++) {
    int num = __VERIFIER_nondet_int();
    selection_list[i] = num;
  }

  selection(selection_list);

  for (int i = 0; i < SIZE - 1; i++) {
    __VERIFIER_assert(selection_list[i] <= selection_list[i + 1]);
  }

  return 0;
}
