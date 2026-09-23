#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CHECK(x) validate(x)
#define LOG(x) log_it(x)
#define CALLER(name) int name(int v) { return helper(v) + 1; }
#define WRAP(f) (f(1) + 1)

int helper(int x) { return x * 2; }
int validate(int x) { return x > 0; }
void log_it(int x) { (void)x; }

// A call in a macro argument is rewritten there; MAX expands it twice.
int in_arg(int v) { return MAX(helper(v), 3); }

// A call that is a whole macro use is rewritten at that use.
int whole_use(int v) {
  LOG(v);
  return CHECK(v);
}

// A call inside a #define body is rewritten in the #define.
CALLER(in_body)

// Spelled half in the argument, half in the #define: no single range to rewrite.
int split(int v) { return WRAP(helper) + v; }
