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

// comments for the function to remove
int doesThing() {
  int var = 1;
  if (var) {
    return var;
  }
  return 0;
}


// comments for main
int main() {
  int foo = 3;
  if (a0 < 10) a1++;
  else a0++;

  if (a0 > a1) {
    a1++;
    if (a1 < a2) {
      a1++;
    }
  } else if (a0 == a1 || a1 <= a2) {
    return a0 + a1 + a2;
  } else {
    a0++;
  }

  // comments by the removed function call
  doesThing();

  struct thing myThing;
  struct thing *myThing2 = &myThing;
  myThing.b = true;
  char *s1 = "";
  char *s2 = "";
  char *s3 = "thing";

  if (*s1 == *s2) {
    myThing.f++;
  }
  if (myThing.b) myThing.f++;
  if (strcmp(myThing.s, s3)) myThing.f++;
  return a0;
}
