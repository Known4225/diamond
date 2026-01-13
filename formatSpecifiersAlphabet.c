/* format specifiers alphabet */

#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("%a\n", 97); // a is for... 0x0.0000000000061p-1022
    printf("%b\n", 98); // b is not for bool or byte
    printf("%c\n", 99); // c is for character
    printf("%d\n", 100); // d is for integer
    printf("%e\n", 101); // e is for... 4.990063e-322
    printf("%f\n", 102); // f is for float
    printf("%g\n", 103); // g is for... 5.08888e-322
    printf("%hd\n", 104); // h is for short
    printf("%i\n", 105); // i is for integer
    printf("%jd\n", 106); // j is for 
    printf("%k\n", 107);
    printf("%ld\n", 108); // l is for longer
    printf("%m\n", 109);
    int n = 110;
    printf("%n\n", &n); // n is for nothing
    printf("%o\n", 111); // o is for octal
    printf("%p\n", 112); // p is for pointer
    printf("%qd\n", 113); // q is for %qd
    printf("%r\n", 114);
    printf("%s\n", "115"); // s is for string
    printf("%td\n", 116); // t is for 
    printf("%u\n", 117); // u is for integer
    printf("%vd\n", 118); // v is for %vd
    printf("%w\n", 119);
    printf("%x\n", 120); // x is for hex
    printf("%y\n", 121);
    printf("%zd\n", 122); // z is for
}