#include <cstdio>
#include <stdbool.h>

int a0;
int a1 = 1;
int a2 = 2;
int a3 = 3;
int a4 = 4;
int a5 = 5;

struct thing {
  float f;
  char s[10];
  bool b;
};

int main() {
  for (a0=0; a0<a5; a0++) {
    if (a0) a1++;
    if (a0) a2++;
    if (a0) a3++;
    if (a0) a4++;
    else a0++;
  }

  while (a5 > 0) {
    a5--;
  }

  struct thing myThing = {3.4, "thing", true};
  struct thing* ptr = &myThing;

  printf("%d\n", ptr->f == myThing.f);

  return a0;
}
