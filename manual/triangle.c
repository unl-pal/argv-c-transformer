/*
 * Practicing adding asserts to code
 * Goal is to attempt to create asserts fitting the ReachSafety meta category
 * and ControlFlow set unless otherwise stated
 * This file has the correct gates in place to prevent overflow
 */

#include <stdio.h>
#include <string.h>

// System defined abort (I think)
extern void abort();

// SV-Comp compatible reach_error function with no args
void reach_error();
// void reach_error(){};


// Transformer added VERIFIER function defined and used by the verifier tools
extern int __VERIFIER_nondet_int(void);

/// Verifier assert defined by this->source code and taking advantage of the 
/// SV-Comp reach_error and system abort
/// @returns nothing as void is required for the signature
/// @param cond an integer that is treated as a boolean to check a condition in
/// the code to see if not 1 then calls the reach_error and abort
void __VERIFIER_assert(int cond) { if(!cond) { reach_error(); abort(); } }

// An enumeration of types of triangles utilized for code readability
enum triangleTypes {
    NOT_TRIANGLE = 0,
    EQUALATERAL = 1,
    ISOSCELES = 2,
    SCALENE = 3,
    RIGHT = 4,
    OBTUSE = 5,
    ACUTE = 6
};

int triangleType;

/// isTriangle uses basic logic to determine if 3 sides of a, b, c length can be
/// a triangle mathmatically speaking
/// @returns boolean 1 if 3 sides can be a triangle
/// @param a side of int length a
/// @param b side of int length b
/// @param c side of int length c
int isTriangle(int a, int b, int c) {
    int result = 1;

    if (a > 9999 || b > 9999 || c > 9999) {
        result = 0;
    } else if (a <= 0 || b <= 0 || c <= 0) {
        result = 0;
    } else if (a + b <= c || a + c <= b || b + c <= a) {
        result = 0;
    }
    return result;
}

/// Classifies the triangles by side definitions equalateral, isosceles, scalene
/// @returns result which is one of the triangleTypes in the enum
/// @param a is int side length of a
/// @param b is int side length of b
/// @param c is int side length of c
/// @param character array reference set to a value corresponding to the type
int classifyTriangleSides(int a, int b, int c, char** sideType) {
    int result;
    if (!isTriangle(a, b, c)) {
        sideType = (char**)("Not a Triangle");
        result = NOT_TRIANGLE;
    } else {
        /// An assert that verifies that a group of 3 sides that are said to be a
        /// triangle follow basic definition of all sides being positive otherwise
        /// throwing a reach_error
        __VERIFIER_assert(a > 0 && b > 0 && c > 0);
        if (a == b && b == c) {
        result   = EQUALATERAL;
        sideType = (char**)("Equalateral");
        } else if (a == b || a == c || b == c) {
            result = ISOSCELES;
        sideType = (char**)("Isosceles");
        } else {
            result = SCALENE;
        sideType = (char**)("Scalene");
        }
    }

        triangleType = result;
    return result;
    /// Asserts that the function ends after the return is made
    __VERIFIER_assert(1); // True on CPA
}

/// Classifies triangles by its angles
/// @returns int value equal to one of the enums available in triangleTypes
/// @param a is int side length of a
/// @param b is int side length of b
/// @param c is int side length of c
/// @param character array reference set to a value corresponding to the type
int classifyTriangleAngles(int a, int b, int c, char** angleType) {
    int result;
    if (isTriangle(a, b, c)) {

        int a_sq = a * a;
        int b_sq = b * b;
        int c_sq = c * c;

        if (a_sq + b_sq == c_sq || a_sq + c_sq == b_sq || b_sq + c_sq == a_sq) {
            result = RIGHT;
            angleType = (char**)("Right");
        } else if (a_sq + b_sq < c_sq || a_sq + c_sq < b_sq || b_sq + c_sq < a_sq) {
            result = OBTUSE;
            printf("a: %d b: %d c: %d", a, b, c);
            angleType = (char**)("Obtuse");
        } else {
            result = ACUTE;
            angleType = (char**)("Acute");
        }
    } else {
        result = NOT_TRIANGLE;
        angleType = (char**)("Not a Triangle");
    }

    /// Assert that angleType is instanciated
    __VERIFIER_assert(angleType != NULL); // True on CPA
    return result;
}

int main() {
    int a = __VERIFIER_nondet_int();
    int b = __VERIFIER_nondet_int();
    int c = __VERIFIER_nondet_int();

    char** sidesString = (char**)("");
    // char** sidesString;
    int sideType = classifyTriangleSides(a, b, c, sidesString);
    char** anglesString = (char**)("");
    // char** anglesString;
    int angleType = classifyTriangleAngles(a, b, c, anglesString);

    /// Assert that the triangle side type is within allowed bounds of the enum
    /// and is one of the side types
    __VERIFIER_assert(sideType >= 0 && sideType <= 3);
    /// Assert that the triangle angle type is within allowed bounds of the enum
    /// and is one of the angle types
    __VERIFIER_assert((angleType >= 4 && angleType < 7) || angleType == 0);

    /// Assert that the given Triangle not both a Right and Equalateral triangle
    /// as defined by geometry
    __VERIFIER_assert(!(angleType == RIGHT && sideType == EQUALATERAL));
    /// Assert that the given Triangle is not both an Obtuse and Equalateral
    /// triangle as defined by geometry
    __VERIFIER_assert(!(angleType == OBTUSE && sideType == EQUALATERAL));

    if (angleType && sideType) {
        /// Assert that sideType exists before accessed in function
        __VERIFIER_assert(anglesString != NULL);
        /// Assert that the triangle is a triangle by both side and angle defs
        __VERIFIER_assert(angleType != NOT_TRIANGLE && sideType != NOT_TRIANGLE);
        printf("Triangle is a  %s %s Triangle\n", *anglesString, *sidesString);
    } else {
        /// Assert that both types are not triangle and not one or the other
        __VERIFIER_assert(!angleType && !sideType);
        /// Assert that sideType exists before accessed in function
        __VERIFIER_assert(anglesString != NULL);
        if (anglesString && sidesString) {
            printf("%s I Repeat %s\n", *anglesString, *sidesString);
        } else {
            printf("Error: Null Strings Given to Function\n");
            /// Assert that at least one string is null
            __VERIFIER_assert(!anglesString || !sidesString);
        }
    }


    return 0;
    /// Assert that the program ends after main
    __VERIFIER_assert(1);
}
