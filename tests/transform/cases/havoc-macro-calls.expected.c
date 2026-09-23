#define __HAVOC_ARGC_MIN 1
#define __HAVOC_ARGC_MAX 4
#define __HAVOC_STR_MAX 16
#define __HAVOC_BLOCK_MAX 128
#define __HAVOC_ARRAY_ELEMS 8
#include "argv_c_harness.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CHECK(x) validate(x)
#define LOG(x) log_it(x)
#define CALLER(name) int name(int v) { return __VERIFIER_nondet_int() + 1; }
#define WRAP(f) (f(1) + 1)
#define TRACE(x) do {  } while (0)

int helper(int x) { return x * 2; }
int validate(int x) { return x > 0; }
void log_it(int x) { (void)x; }

// A call in a macro argument is rewritten there; MAX expands it twice.
int in_arg(int v) { return MAX(__VERIFIER_nondet_int(), 3); }

// A call that is a whole macro use is rewritten at that use; a void one is dropped with its `;`
// as a statement, and becomes ((void)0) inside an expression.
int whole_use(int v) {
  
  int w = (((void)0), 1);
  TRACE(v);
  return __VERIFIER_nondet_int() + w;
}

// A call inside a #define body is rewritten in the #define.
CALLER(in_body)

// Spelled half in the argument, half in the #define: no single range to rewrite.


int main(void) {
  helper(__VERIFIER_nondet_int());
  validate(__VERIFIER_nondet_int());
  log_it(__VERIFIER_nondet_int());
  in_arg(__VERIFIER_nondet_int());
  whole_use(__VERIFIER_nondet_int());
  in_body(__VERIFIER_nondet_int());
  return 0;
}
