#include <string.h>
#include "helper.h"

int compute(int n) {
  int a = local_helper(n);      /* repo-local header: havoc */
  char *name = make_name();     /* repo-local header, char*: havoc cstring */
  log_msg(a);                   /* repo-local header, void: drop */
  if (strlen(name) > 3) {       /* system header: keep */
    a += (int)strlen(name);
  }
  return a;
}
