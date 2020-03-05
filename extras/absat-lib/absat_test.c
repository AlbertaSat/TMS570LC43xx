/*
    Test harness for absat_lib string functions
*/

#include "absat_lib.c"
#include <stdio.h>
int main( int argc, char *argv[] )
{
    size_t bufsize = 80;
    char buf[bufsize];
    int n = 0;

    printf("Testing begins ...\n");

    printf("Simple append calls ...\n");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize-n, buf);

    n += StrApStr(buf, bufsize, "test");
    n += StrApChar(buf, bufsize, '%');
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApBuf(buf, bufsize, "none", 0);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApBuf(buf, bufsize, "A-long-string", 7);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    printf("Hex\n");

    n += StrApHex(buf, bufsize, 0x12345678);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApHex(buf, bufsize, 0xf2345678);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApHex(buf, bufsize, 0);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    printf("Dec\n");

    n += StrApDec(buf, bufsize, 0);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, 123);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, -123);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, 1234567890);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, -1234567890);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, -34567890);
    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf, bufsize, -1234567890);
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApStr(buf, bufsize, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    /* second batch of tests begins */

    n = 0;
    buf[0] = '\0';
    printf("Moving buffer append calls ...\n");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApStr(buf+n, bufsize-n, "test");
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApBuf(buf+n, bufsize-n, "none", 0);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApBuf(buf+n, bufsize-n, "A-long-string", 7);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    printf("Hex\n");

    n += StrApHex(buf+n, bufsize-n, 0x12345678);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApHex(buf+n, bufsize-n, 0xf2345678);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApHex(buf+n, bufsize-n, 0);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    printf("Dec\n");

    n += StrApDec(buf+n, bufsize-n, 0);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf+n, bufsize-n, 123);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf+n, bufsize-n, -123);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf+n, bufsize-n, 1234567890);
    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApDec(buf+n, bufsize-n, -1234567890);
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);

    n += StrApStr(buf+n, bufsize-n, "|");
    printf("n=%4d, size=%4lu, buf='%s'\n", n, bufsize, buf);


    printf("... testing ends.\n");
    }
