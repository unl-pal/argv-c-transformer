// SPDX-FileCopyrightText: Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

/* Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/*
 * May 8, 2026
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void abort();
void reach_error();

extern int __VERIFIER_nondet_int(void);
extern char __VERIFIER_nondet_char(void);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

#undef errno
int mock_errno = 0;
#define errno mock_errno

#define ferror(stream) (0)

char *mock_realloc(char *ptr, size_t size) {
  if (size == 0) {
    if (ptr) free(ptr);
    return NULL;
  }
  void *new_ptr = malloc(size);
  if (ptr) free(ptr);
  return new_ptr;
}

#define realloc mock_realloc

// Limit reads so the initial MAX_CANON buffer is never exhausted and realloc
// is never triggered, keeping the path set small enough for verification.
#define MAX_READS 4
static int reads_done = 0;

int mock_getc(FILE *stream) {
    if (reads_done >= MAX_READS) return EOF;
    reads_done++;
    int c = __VERIFIER_nondet_int();
    if (!((c >= 0 && c < 256) || c == EOF)) { abort(); }
    return c;
}

#define getc mock_getc

/* Read up to (and including) a TERMINATOR from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

ssize_t getdelim(char **lineptr, size_t *n, int terminator, FILE *stream) {
  char *line, *p;
  size_t size, copy;

  if (stream == NULL || lineptr == NULL || n == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (ferror(stream))
    return -1;

  /* Make sure we have a line buffer to start with.  */
  if (*lineptr == NULL || *n < 2) /* !seen and no buf yet need 2 chars.  */
  {
#ifndef MAX_CANON
#define MAX_CANON 256
#endif
    line = realloc(*lineptr, MAX_CANON);
    if (line == NULL)
      return -1;
    *lineptr = line;
    *n = MAX_CANON;
  }

  line = *lineptr;
  size = *n;

  copy = size;
  p = line;

  while (1) {
    size_t len;

    while (--copy > 0) {
      register int c = getc(stream);
      if (c == EOF)
        goto lose;
      else if ((*p++ = c) == terminator)
        goto win;
    }

    /* Need to enlarge the line buffer.  */
    len = p - line;
    size *= 2;
    line = realloc(line, size);
    if (line == NULL)
      goto lose;
    *lineptr = line;
    *n = size;
    p = line + len;
    copy = size - len;
  }

lose:
  if (p == *lineptr)
    return -1;
  /* Return a partial line since we got an error in the middle.  */
win:
  *p = '\0';
  return p - *lineptr;
}

int main(void) {
  char *line = NULL;
  size_t n = 0;

  FILE dummy_stream;

  int terminatorChar = __VERIFIER_nondet_int();
  if (!(terminatorChar >= 0 && terminatorChar < 256)) { abort(); }

  ssize_t lenOfDelimStr = getdelim(&line, &n, terminatorChar, &dummy_stream);

  if (lenOfDelimStr >= 0 && line) {
    __VERIFIER_assert(lenOfDelimStr < n);
    __VERIFIER_assert(line[lenOfDelimStr] == '\0');
    __VERIFIER_assert(n % MAX_CANON == 0);
    __VERIFIER_assert(lenOfDelimStr == 0 || line[lenOfDelimStr - 1] == terminatorChar);
  }

  if (line) free(line);

  return 0;
}
