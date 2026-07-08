// SPDX-FileCopyrightText: Copyright (C) 2004, 2003, 2002 University of Utah
// SPDX-License-Identifier: Zlib
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
** enhex: simple stand-alone hex encoder
**
** Compile with:
**    cc -o enhex enhex.c
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

#define IN_SIZE 2

static char in_buffer[IN_SIZE];
static int in_pos;

int mock_fgetc(FILE *stream) {
  if (in_pos >= IN_SIZE) return EOF;
  return (unsigned char)in_buffer[in_pos++];
}

#define fgetc mock_fgetc

#define OUT_SIZE 4
static char out_buffer[OUT_SIZE];
static int out_pos;

int mock_fprintf(FILE *stream, const char *fmt, int c1, int c2) {
  (void)stream; (void)fmt;
  if (out_pos < OUT_SIZE) out_buffer[out_pos++] = (char)c1;
  if (out_pos < OUT_SIZE) out_buffer[out_pos++] = (char)c2;
  return 2;
}

#define fprintf mock_fprintf

int enhexColumns = 70; /* number of characters per line */

int enhexUsage(char *me) {
  /*                       0   1     2   (2/3) */
  // fprintf(stderr, "usage: %s <in> [<out>]\n", me);
  // fprintf(stderr, " <in>: file to read raw data from\n");
  // fprintf(stderr, "<out>: file to write hex data to; "
  //                 "uses stdout by default\n");
  // fprintf(stderr, " \"-\" can be used to refer to stdin/stdout\n");
  // exit(1);
  return 1;
}

void enhexFclose(FILE *file) {

  if (!(stdin == file || stdout == file)) {
    fclose(file);
  }
}

int enhexTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int original_main(int argc, char *argv[]) {
  char *me, *inS, *outS;
  FILE *fin, *fout;
  int car = 0, col;

  me = argv[0];
  if (!(2 == argc || 3 == argc))
    enhexUsage(me);

  inS = argv[1];
  if (!strcmp("-", inS)) {
    fin = stdin;
// #ifdef _WIN32
//     _setmode(_fileno(fin), _O_BINARY);
// #endif
  } else {
    fin = fopen(inS, "rb");
    if (!fin) {
      // fprintf(stderr, "\n%s: couldn't fopen(\"%s\",\"rb\"): %s\n\n", me, inS,
      //         strerror(errno));
      enhexUsage(me);
    }
  }
  if (2 == argc) {
    fout = stdout;
  } else {
    outS = argv[2];
    if (!strcmp("-", outS)) {
      fout = stdout;
    } else {
      fout = fopen(outS, "w");
      if (!fout) {
        // fprintf(stderr, "\n%s: couldn't fopen(\"%s\",\"w\"): %s\n\n", me, outS,
        //         strerror(errno));
        enhexUsage(me);
      }
    }
  }

  col = 0;
  car = fgetc(fin);
  while (EOF != car) {
    int high_nibble = (car >> 4) & 15;
    int low_nibble = car & 15;
    if (col > enhexColumns) {
      // fprintf(fout, "\n");
      col = 0;
    }
    fprintf(fout, "%c%c", enhexTable[high_nibble], enhexTable[low_nibble]);
    col += 2;
    car = fgetc(fin);
  }
  if (2 != col) {
    // fprintf(fout, "\n");
  }

  enhexFclose(fin);
  enhexFclose(fout);
  // exit(0);
  return 0;
}

// Arg-C verification harness
int main() {

  in_buffer[0] = 'H';
  in_buffer[1] = 'i';

  int argc = 2;
  char argv0[] = "enhex";
  char argv1[] = "-";
  char *argv[] = {argv0, argv1};

  original_main(argc, argv);

  __VERIFIER_assert(out_buffer[0] == '4');
  __VERIFIER_assert(out_buffer[1] == '8');
  __VERIFIER_assert(out_buffer[2] == '6');
  __VERIFIER_assert(out_buffer[3] == '9');

  return 0;
}
