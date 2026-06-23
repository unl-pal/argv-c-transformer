extern int external_thing(int x);
void quiet(void);
char *make_name(void);
int *make_buf(void);
float fval(void);

int compute(int n) {
  int a = external_thing(n);
  quiet();
  char *s = make_name();
  int *b = make_buf();
  float f = fval();
  return a + s[0] + b[0] + (int)f;
}
