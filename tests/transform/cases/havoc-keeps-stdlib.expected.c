#include "argv_c_runtime.h"

#include <string.h>

int uses_stdlib(const char *s) {
  return (int)strlen(s);
}

int main(void) {
  return 0;
}
