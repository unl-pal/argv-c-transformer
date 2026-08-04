/* Regression guard for the quoted-include leak (see docs/... for the real
 * incident: a vendored zlib.h, dropped by the filter step because it only
 * ever copies .c files, silently got picked back up from the build host's
 * /usr/include/zlib.h instead of the include directive being stripped).
 *
 * "stddef.h" here has no companion file anywhere near this source; it is
 * deliberately quoted, not angled, to confirm it gets stripped purely
 * because it's a quoted include, regardless of whether it happens to
 * resolve. On every platform clang will still happily resolve it via its
 * own bundled resource-dir headers (the same class of accidental fallback
 * resolution that caused the zlib incident), which is exactly what makes
 * this a faithful, portable reproduction: a naive "only strip if Clang
 * couldn't resolve it" check passes this file too, since it does resolve -
 * just to the wrong thing. The surviving code deliberately does not depend
 * on anything stddef.h would provide, so the benchmark should still compile
 * either way; the assertion in run_smoke_test.sh is purely textual,
 * decoupled from whether this file happens to compile. */
#include <stdio.h>
#include "stddef.h"

int identity(int x) {
  return x;
}
