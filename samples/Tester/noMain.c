/* 
* LICENSE INFO
*
* COPYRIGHT INFO
*
*/

/// How to tutorial thingy

#include <string.h>
#include <stdbool.h>

bool anotherBool = true;

// comment by local external function
extern int elsewhere(int x);
// comments for the var decl
int a0 = 0;
int a1 = 1;
int a2 = 2;

int ara[3] = {0, 1, 2};
int b0;

struct thing {
  float f;
  char s[10];
  bool b;
};

int doesThing(int input, int input2) {
  int var = 1;
  if (input) {
    return var + input + input2;
  }
  return 0;
}

int doesThing2(int input) {
  int var = 1;
  if (input) {
    return var + input;
  }
  return 0;
}

int doesThing3(int input) {
  int var = 1;
  if (input) {
    return var + input;
  }
  return 0;
}
// comments for the function to remove
// badFunc3 comment
bool empty() {
  return false;
}

char* returnsString() {
  return "string";
}

// removeMe comment
int hasInput(int input) {
  return input;
}

