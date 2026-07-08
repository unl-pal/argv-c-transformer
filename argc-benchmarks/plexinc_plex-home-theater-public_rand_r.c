// SPDX-FileCopyrightText: Copyright (C) 1996, 1999 Free Software Foundation, Inc.
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

/*
 * May 8, 2026
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
*/

/* Reentrant random function frm POSIX.1c.
   Copyright (C) 1996, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com <mailto:drepper@cygnus.com>>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <stdlib.h>

extern void abort();
void reach_error();

extern unsigned int __VERIFIER_nondet_uint(void);

void __VERIFIER_assert(int cond) {if (!cond) {reach_error(); abort(); } }

/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits.  */
int rand_r (unsigned int *seed)
{
    unsigned int next = *seed;
    int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    *seed = next;

    return result;
}

int main(void)
{
    unsigned int seed = __VERIFIER_nondet_uint();
    unsigned int replay_seed = seed;

    int result = rand_r(&seed);
    int replay_result = rand_r(&replay_seed);

    __VERIFIER_assert(result >= 0 && result <= 0x7FFFFFFF);
    __VERIFIER_assert(replay_result == result);
    __VERIFIER_assert(replay_seed == seed);

    return 0;
}
