// SPDX-FileCopyrightText: Copyright (C) 2004, 2003, 2002 University of Utah
// SPDX-License-Identifier: Custom
// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

/*
 * May 8, 2026
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

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

char* mock_strerror(int errnum) {
  return "mocked_strerror";
}

#define strerror mock_strerror

#define IN_SIZE 4
#define OUT_SIZE 2

static char in_buffer[IN_SIZE];
static char out_buffer[OUT_SIZE];
static int in_pos, out_pos;

int mock_fgetc(FILE *stream) {
  if (in_pos >= IN_SIZE) return EOF;
  return (unsigned char)in_buffer[in_pos++];
}

#define fgetc mock_fgetc

int mock_fputc(int c, FILE *stream) {
  if (out_pos >= OUT_SIZE) return EOF;
  out_buffer[out_pos++] = (char)c;
  return c;
}

#define fputc mock_fputc

int dehexUsage(char *me) {
  /*                       0   1     2   (2/3) */
  fprintf(stderr, "usage: %s <in> [<out>]\n", me);
  fprintf(stderr, " <in>: file to read hex data from\n");
  fprintf(stderr, "<out>: file to write raw data to; "
                  "uses stdout by default\n");
  fprintf(stderr, " \"-\" can be used to refer to stdin/stdout\n");
  // exit(1);
  return 1;
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
    nibble = dehexTable[car & 127];
    if (-2 == nibble) {
      /* its an invalid character */
      break;
    }
    if (-1 == nibble) {
      /* its white space */
      continue;
    }
    if (even) {
      byte = nibble << 4;
    } else {
      byte += nibble;
      if (EOF == fputc(byte, fout)) {
        fprintf(stderr, "%s: error writing!!!\n", me);
        // exit(1);
        return 1;
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
  // exit(0);
  return 0;
}

// Arg-C verification harness
int main() {
  in_buffer[0] = '4';
  in_buffer[1] = '8';
  in_buffer[2] = '6';
  in_buffer[3] = '9';

  int argc = 2;
  char argv0[] = "dehex";
  char argv1[] = "-";
  char *argv[] = {argv0, argv1};

  original_main(argc, argv);

  __VERIFIER_assert(out_buffer[0] == 'H');
  __VERIFIER_assert(out_buffer[1] == 'i');

  return 0;
}
