/*
    Alberta Sat minimal libc

    Version 1.0 - 2020-02-27 
*/

/*
    Safe string operations.  That is, all strings we generate are \0
    terminated, and no string is fetched or assigned with an index 
    outside the range 0 .., len-1.  So, assuming that the sie limits
    are correct, we will never run off the end of a string.

    Return values are the count of characters generated by the operation,
    terminating \0 is not counted.

    Note, we use array indexing on the assumption that the compiler will
    convert to more efficient code.  Plus, we don't do a lot of string
    operations.

    Terminology 

    size - the maximum number of chars, including \0, that the buffer
        can store.

    maxlen - the maximum number of characters possible in a string 
        stored in the buffer, i.e. size-1.

    len - the actual number of characters in the string, not counting
        the terminating \0.

    insert - means to place the result at a specific position in the buffer

    append - means to place it at the end of the string in the buffer. I.e.
        in the position where the \0 resides.
    
    Functions

    StrAp - append a string to a buffer
    StrApDec - convert int32_t into a decimal and append to a buffer
    StrApHex - convert uin32_t into a 8 digit hex and append to a buffer

    Pending ...
    StrApDbl - convert long double into a floating point string
    strnlen - string len, safe form.

    Typical use:

        char buf[32];
        size_t bufsize = 32;

        StrApStr(buf, bufsize, "test", 5);
        StrApStr(buf, bufsize, "|", 1);
        StrApHex(buf, bufsize, 0x12345678);

    The above are slow, because you need to locate the \0 every
    time you call.  An alternative approach is to move the buffer 
    to near then end of the string as in:

        char buf[32];
        size_t bufsize = 32;
        int n=0;

        n += StrApStrbuf+n, bufsize-n, "|", 1);
        n += StrApStrbuf+n, bufsize-n, "none", 0);
        n += StrApHex(buf+n, bufsize-n, 0x12345678);

*/

#include <stddef.h>
#include <stdint.h>

int32_t StrInBuf( char *buf, size_t bufsize, char *inbuf, size_t inbufsize )
{
    if ( bufsize <= 0 ) { return 0; };

    int i = 0;
    while ( i < bufsize-1 && i < inbufsize && 
        (buf[i] = inbuf[i]) != '\0'  ) {
        i++;
        }
    buf[i] = '\0';

    /* number of characters appended, not counting \0 */
    return i;
    }

int32_t StrApBuf( char *buf, size_t bufsize, char *inbuf, size_t inbufsize )
{
    if ( bufsize <= 0 ) { return 0; };

    /* locate the end of the current string in buf */
    while ( bufsize > 0 && *buf != '\0' ) {
        bufsize--;
        buf++;
        }

    /* number of characters inserted, not counting \0 */
    return StrInBuf(buf, bufsize, inbuf, inbufsize);
    }


int32_t StrApStr( char *buf, size_t bufsize, char *str )
{
    if ( bufsize <= 0 ) { return 0; };

    /* locate the end of the current string in buf */
    while ( bufsize > 0 && *buf != '\0' ) {
        bufsize--;
        buf++;
        }

    if ( bufsize <= 0 ) { return 0; };

    int i = 0;
    while ( i < bufsize-1 && (buf[i] = str[i]) != '\0'  ) {
        i++;
        }
    buf[i] = '\0';

    /* number of characters appended, not counting \0 */
    return i;
    }

int32_t StrApChar( char *buf, size_t bufsize, char c )
{
    char cbuf[2];
    cbuf[0] = c;
    cbuf[1] = '\0';

    if ( bufsize <= 0 ) { return 0; };

    return StrApStr( buf, bufsize, cbuf );
    }


int32_t StrApHex( char *buf, size_t bufsize, uint32_t val )
{
    static char* digits = "0123456789abcdef";

    if ( bufsize <= 0 ) { return 0; };

    /* locate the end of the current string in buf */
    while ( bufsize > 0 && *buf != '\0' ) {
        bufsize--;
        buf++;
        }

    if ( bufsize <= 0 ) { return 0; };

    /* if insufficient length to hold all 8 hex digits of val, return
       as many right hand ones as possible.
    */

    uint32_t bits = (uint32_t) val;
    int pos = bufsize > 8 ? 8 : bufsize-1;
    
    /* fill in from rhs */

    buf[pos] = '\0';
    int i = 0;
    while ( pos > 0 ) {
        pos--;
        buf[pos] = digits[ (uint8_t) (bits & 0xf) ];
        i++;
        bits = bits >> 4;
        }

    /* number of characters appended, not counting \0 */
    return i;
    }



int32_t StrApDec( char *buf, size_t bufsize, int32_t val )
{
    if ( bufsize <= 0 ) { return 0; };

    /* locate the end of the current string in buf */
    while ( bufsize > 0 && *buf != '\0' ) {
        bufsize--;
        buf++;
        }

    /* if insufficient length to hold all 11 signed decimal digits of val, 
       return as many right hand ones as possible.
    */

    if ( bufsize <= 0 ) { return 0; };

    int isneg = val < 0;
    if ( isneg ) {
        val = -val;
        }

    uint32_t bits = (uint32_t) val;

    int pos = bufsize > 13 ? 13 : bufsize-1;

    int j;

    /* fill in from rhs */
    
    buf[pos] = '\0';
    int i = 0;
    while ( pos > 0 ) {
        pos--;
        buf[pos] = '0' + (uint8_t) ( bits % 10 );
        i++;
        bits = bits / 10;
        if ( bits == 0 ) { break; }
        }

    /* if bits isn't 0, we ran out of space */
    if ( bits ) {
        for ( j=0; j < i; j++ ) {
            buf[j] = '*';
            }
        return i;
        }

    /* negation if needed */
    if ( pos > 0 && isneg ) {
        pos--;
        buf[pos] = '-';
        i++;
        }

    /* pos now tells us how many leading blanks to insert or shift */

    if ( 0 ) {
        /* put in leading blanks */
        while ( pos > 0 ) {
            pos--;
            buf[pos] = ' ';
            i++;
            }
        }
    else {
        /* shift left so no leading blanks, including \0 */
        for ( j=0; j <= i ; j++ ) {
            buf[j] = buf[j+pos];
            }
        }

    /* number of characters appended, not counting \0 */
    return i;
    }

