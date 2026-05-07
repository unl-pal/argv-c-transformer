// SPDX-FileCopyrightText: Copyright (C) 2004, 2003, 2002 University of Utah
// SPDX-License-Identifier: Custom
// SPDX-FileCopyrightText: Copyright (C) 2025 The ARG-V Project

/*
 * Aug 27, 2025
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
 */

/*
  Copyright (C) 2004, 2003, 2002 University of Utah

  This software,  is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*
** dehex: simple stand-alone hex decoder
**
** Compile with:
**    cc -o dehex dehex.c
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void abort();
void reach_error();

extern int __VERIFIER_nondet_int(void);
extern void *__VERIFIER_nondet_pointer(void);
extern char __VERIFIER_nondet_char(void);
extern void __VERIFIER_assume(int expression);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

char* strerror(int errnum) {
  return "mocked_strerror";
}

void dehexUsage(char *me) {
  /*                       0   1     2   (2/3) */
  fprintf(stderr, "usage: %s <in> [<out>]\n", me);
  fprintf(stderr, " <in>: file to read hex data from\n");
  fprintf(stderr, "<out>: file to write raw data to; "
                  "uses stdout by default\n");
  fprintf(stderr, " \"-\" can be used to refer to stdin/stdout\n");
  exit(1);
}

void dehexFclose(FILE *file) {

  if (!(stdin == file || stdout == file)) {
    fclose(file);
  }
}

int dehexTable[128] = {
    /* 0   1   2   3   4   5   6   7   8   9 */
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, /*   0 */
    -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, /*  10 */
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, /*  20 */
    -2, -2, -1, -2, -2, -2, -2, -2, -2, -2, /*  30 */
    -2, -2, -2, -2, -2, -2, -2, -2, 0,  1,  /*  40 */
    2,  3,  4,  5,  6,  7,  8,  9,  -2, -2, /*  50 */
    -2, -2, -2, -2, -2, 10, 11, 12, 13, 14, /*  60 */
    15, -2, -2, -2, -2, -2, -2, -2, -2, -2, /*  70 */
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, /*  80 */
    -2, -2, -2, -2, -2, -2, -2, 10, 11, 12, /*  90 */
    13, 14, 15, -2, -2, -2, -2, -2, -2, -2, /* 100 */
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, /* 110 */
    -2, -2, -2, -2, -2, -2, -2, -2          /* 120 */
};

// original main from source
int
original_main(int argc, char *argv[]) {
  char *me, *inS, *outS;
  FILE *fin, *fout;
  int car=0, byte, nibble, even;

  me = argv[0];
  if (!( 2 == argc || 3 == argc )){
    dehexUsage(me);
  }
  inS = argv[1];
  if (!strcmp("-", inS)) {
    fin = stdin;
  } else {
    fin = fopen(inS, "r");
    if (!fin) {
      fprintf(stderr, "\n%s: couldn't fopen(\"%s\",\"rb\"): %s\n\n",
              me, inS, strerror(errno));
      dehexUsage(me);
    }
  }
  if (2 == argc) {
    fout = stdout;
  } else {
    outS = argv[2];
    if (!strcmp("-", outS)) {
      fout = stdout;
// #ifdef _WIN32
//       _setmode(_fileno(fout), _O_BINARY);
// #endif
    } else {
      fout = fopen(outS, "w");
      if (!fout) {
        fprintf(stderr, "\n%s: couldn't fopen(\"%s\",\"w\"): %s\n\n",
                me, outS, strerror(errno));
        dehexUsage(me);
      }
    }
  }

  byte = 0;
  even = 1;
  for (car=fgetc(fin); EOF != car; car=fgetc(fin)) {
    __VERIFIER_assert(car >= 0 && car <= 255);
    nibble = dehexTable[car & 127];
    __VERIFIER_assert(nibble >= -2 && nibble <= 15);
    if (-2 == nibble) {
      /* its an invalid character */
      break;
    }
    if (-1 == nibble) {
      /* its white space */
      continue;
    }
    __VERIFIER_assert(nibble >= 0 && nibble <= 15);
    if (even) {
      byte = nibble << 4;
    } else {
      byte += nibble;
      __VERIFIER_assert(byte >= 0 && byte <= 255);
      if (EOF == fputc(byte, fout)) {
        fprintf(stderr, "%s: error writing!!!\n", me);
        exit(1);
      }
    }
    even = 1 - even;
  }
  if (EOF != car) {
    fprintf(stderr, "\n%s: got invalid character '%c'\n\n", me, car);
    dehexUsage(me);
  }

  dehexFclose(fin);
  dehexFclose(fout);
  exit(0);
}

// Arg-C verification harness
int main() {
  int argc = __VERIFIER_nondet_int();
  __VERIFIER_assume(argc >= 0 && argc <= 4);

  int BOUND = 8;
  char argv_data[4][BOUND];
  char *argv[4];
  for (int i = 0; i < 4; i++) {
    argv[i] = argv_data[i];
  }
  for (int i = 0; i < argc; i++) {
    for (int j = 0; j < BOUND - 1; j++) {
      argv[i][j] = __VERIFIER_nondet_char();
    }
    argv[i][BOUND - 1] = '\0';
  }

  original_main(argc, argv);
  return 0;
}
