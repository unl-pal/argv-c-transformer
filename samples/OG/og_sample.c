#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int MkX(int* x) {
  int y = rand();
  *x = y;
  return 1;
}

char* hello(bool what) {
  char* str;
  if (what) {
    if (rand() > 4) {
      str = (char*)"In Random Territory";
    } else {
      str = (char*)"Hello World";
    }
  } else if (!what) {
    str = (char*)"a pain that I am used to";
  } else {
    str = (char*)"Ouch!";
  }
  return str;
}

int main(){
  printf("%s", hello(true));
  if (4 == 4) {
    return 0;
  }
  return 0;
}
