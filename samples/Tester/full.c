#include <string.h>
#include <stdbool.h>

// comments for the var decl
int a0 = 0;
int a1 = 1;
int a2 = 2;

int ara[3] = {0, 1, 2};
int b0;

bool anotherBool = true;

struct thing {
  float f;
  char s[10];
  bool b;
};

int doesThing(int input) {
  int var = 1;
  if (input) {
    return var + input;
  }
  return 0;
}

// comments for the function to remove
// badFunc3 comment
int empty() {
  return 1;
}

// removeMe comment
int hasInput(int input) {
  return input;
}

// comments for main
int main() {

  struct thing myThing;
  struct thing *myThing2 = &myThing;
  myThing.b = true;
  char *s1 = "";
  char *s2 = "";
  char *s3 = "thing";

  int foo = a0 + a1 - a2;

  if (a0 > a1) {
    a1++;
    if (a1 < a2) {
      a1++;
    }
  } else if (a0 == a1 || a1 <= a2) {
    return a0 + a1 + a2;
  } else if (myThing.b) {
    foo += a0;
  } else {
    ++a0;
  }

  int y = empty();

  int z = hasInput(a2)+ empty();

  // comments by the removed function call
  int x = hasInput(z) * y;

  doesThing(x);

  if (*s1 == *s2) {
    myThing.f++;
  }
  if (myThing.b) myThing.f++;
  if (strcmp(myThing.s, s3)) myThing.f++;
  return y - x;
}
