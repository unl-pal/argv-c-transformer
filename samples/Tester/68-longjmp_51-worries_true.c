// This file is part of the SV-Benchmarks collection of verification tasks:
// https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks
//
// SPDX-FileCopyrightText: 2022-2023 University of Tartu & Technische Universität München
//
// SPDX-License-Identifier: MIT
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>

extern void abort(void);
void reach_error() { assert(0); }
void __VERIFIER_assert(int cond) { if(!(cond)) { ERROR: {reach_error();abort();} } }


jmp_buf env_buffer;
int global = 0;

int bar() {
   longjmp(env_buffer, 2);
   return 8;
}

void foo() {
   global = bar();
}

int main() {
   if(setjmp( env_buffer )) {
      __VERIFIER_assert(global == 0);
      return 0;
   }

   foo();
}
