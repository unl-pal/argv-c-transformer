// SPDX-FileCopyrightText: Copyright (c) 1998, 2015 Todd C. Miller <millert@openbsd.org>
// SPDX-License-Identifier: Custom
// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

/*
 * May 8, 2026
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
 */

/*
 * Copyright (c) 1998, 2015 Todd C. Miller <millert@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// #include <pthread.h>
#include <string.h>

extern void abort();
void reach_error();

extern unsigned int __VERIFIER_nondet_uint(void);
extern char __VERIFIER_nondet_char(void);
extern void __VERIFIER_assume(int expression);

void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}

/*
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
size_t redis_strlcpy(char *dst, const char *src, size_t dsize) {
  const char *osrc = src;
  size_t nleft = dsize;

  /* Copy as many bytes as will fit. */
  if (nleft != 0) {
    while (--nleft != 0) {
      if ((*dst++ = (char)*src++) == '\0')
        break;
    }
  }

  /* Not enough room in dst, add NUL and traverse rest of src. */
  if (nleft == 0) {
    if (dsize != 0)
      *dst = '\0'; /* NUL-terminate dst */
    while (*src++)
      ;
  }

  return (src - osrc - 1); /* count does not include NUL */
}

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 */
size_t redis_strlcat(char *dst, const char *src, size_t dsize) {
  // printf("str1: %s\nstr2: %s\nsize: %zu\n", dst, src, dsize);
  const char *odst = dst;
  const char *osrc = src;
  size_t n = dsize;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end. */
  while (n-- != 0 && *dst != '\0')
    dst++;
  dlen = dst - odst;
  n = dsize - dlen;

  if (n-- == 0)
    return (dlen + strlen(src));
  while (*src != '\0') {
    if (n != 0) {
      *dst++ = *src;
      n--;
    }
    src++;
  }
  *dst = '\0';

  return (dlen + (src - osrc)); /* count does not include NUL */
}

// Arg-C verification harness
#define MAX_BOUND 16
int main(void) {
  char dst[MAX_BOUND];
  char src[MAX_BOUND];

  unsigned int src_len = __VERIFIER_nondet_uint();
  unsigned int dst_len = __VERIFIER_nondet_uint();
  __VERIFIER_assume(src_len < MAX_BOUND);
  __VERIFIER_assume(dst_len < MAX_BOUND);

  for (unsigned int i = 0; i < src_len; i++) {
    src[i] = __VERIFIER_nondet_char();
    __VERIFIER_assume(src[i] != '\0');
  }
  for (unsigned int i = 0; i < dst_len; i++) {
    dst[i] = __VERIFIER_nondet_char();
    __VERIFIER_assume(dst[i] != '\0');
  }
  src[src_len] = '\0';
  dst[dst_len] = '\0';

  unsigned int size = __VERIFIER_nondet_uint();
  __VERIFIER_assume(size <= MAX_BOUND);

  size_t cat_result = redis_strlcat(dst, src, size);

  __VERIFIER_assert(cat_result == src_len + (size < dst_len ? size : dst_len));

  size_t copy_result = redis_strlcpy(dst, src, size);

  __VERIFIER_assert(copy_result == src_len);
  if (size > 0) {
      size_t term_idx = src_len < size - 1 ? src_len : size - 1;
      __VERIFIER_assert(dst[term_idx] == '\0');
  }

  return 0;
}
