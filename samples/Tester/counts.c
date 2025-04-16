#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int a0;
int a1 = 1;
int a2 = 2;
int a3 = 3;
int a4 = 4;
int a5 = 5;

float f0 = 1.0;

struct thing {
  float f;
  char s[10];
  bool b;
};

char myString[] = "Hello";
char myCharAr[] = "World";

int main() {
  if (strcmp(myString, myCharAr)) {
    return 0;
  }
  for (a0=0; a0<a5; a0++) {
    if (a0) a1++;
    if (a1) a2++;
    if (a2) a3++;
    if (a3) a4++;
    else a0++;

    if (a0 < a1) {
      a0++;
    }
    else if (a0 < a2) {
      a0++;
    }
    else if (a0 < a3) {
      a0++;
    }
    else if (a0 < a4) {
      a0++;
    }
    else {
      a0++;
    }
  }

  while (a5 > 0) {
    a5--;
  }

  struct thing myThing = {3.4, "thing", true};
  struct thing* ptr = &myThing;

  printf("%d\n", ptr->f == myThing.f);
  printf("%d\n", a0);

  return a0;
}
