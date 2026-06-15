// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
// SPDX-License-Identifier: Apache-2.0

/*
 * May 14, 2026
 * Written by Nathanael Steven and the ARG-V development team for use as
 * a benchmark for Static Verification tools
 */

#include <assert.h>

extern void abort();
void reach_error() { assert(0); }

extern unsigned int __VERIFIER_nondet_uint(void);

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

void unsort(int *array) {
  for (int i = 0; i < SIZE; i++) {
    int j = (int)(__VERIFIER_nondet_uint() % SIZE);
    swap(&array[i], &array[j]);
  }
}

// Arg-C verification harness
int main() {
  int my_list[SIZE];
  for (int i = 0; i < SIZE; i++) {
    my_list[i] = i;
  }
  unsort(my_list);

  int not_sorted = 0;
  for (int i = 0; i < SIZE - 1; i++) {
    if (my_list[i] > my_list[i + 1]) {
      not_sorted = 1;
    }
  }
  __VERIFIER_assert(not_sorted);

  return 0;
}
