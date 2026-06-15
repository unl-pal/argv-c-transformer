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

static int SIZE = 2;

void merge_combine(int *array, int l, int m, int r) {
  int n1 = m - l + 1;
  int n2 = r - m;

  int L[n1], R[n2];

  for (int i = 0; i < n1; i++) L[i] = array[l + i];
  for (int j = 0; j < n2; j++) R[j] = array[m + 1 + j];

  int i = 0, j = 0, k = l;
  while (i < n1 && j < n2) {
    if (L[i] <= R[j]) { array[k] = L[i]; i++; }
    else { array[k] = R[j]; j++; }
    k++;
  }
  while (i < n1) { array[k] = L[i]; i++; k++; }
  while (j < n2) { array[k] = R[j]; j++; k++; }
}

void merge_sort_recursive(int *array, int l, int r) {
  if (l < r) {
    int m = l + (r - l) / 2;
    merge_sort_recursive(array, l, m);
    merge_sort_recursive(array, m + 1, r);
    merge_combine(array, l, m, r);
  }
}

void merge(int *array, int n) {
  merge_sort_recursive(array, 0, n - 1);
}

// Arg-C verification harness
int main() {
  int merge_list[SIZE];

  merge_list[0] = 2;
  merge_list[1] = 1;

  merge(merge_list, SIZE);

  for (int i = 0; i < SIZE - 1; i++) {
    __VERIFIER_assert(merge_list[i] <= merge_list[i + 1]);
  }

  return 0;
}
