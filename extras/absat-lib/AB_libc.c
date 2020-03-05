/*
    Alberta Sat minimal libc
*/

/*
    Basic string conversion and other utils.

    longtostrn - convert long int into a decimal, octal, or hex string.

    doubletostrn - convert long double into a floating point string

    strnlen - string len, dafe form.

    All input and output buffers are in safe form, i.e. the length of the 
    buffer is specified.

    Return values are the count of characters resulting from the conversion,
    terminating \0 is not counted.
*/

#include <string.h>

#define BUFLEN 20

_CODE_ACCESS int longtostrn(long val, int base, char *buf, size_t buflen)
{
    char           tempc[BUFLEN];
    register char *bufptr;
    register int   neg = val < 0;
    register long  uval = val;

    /* bases are hard coded for speed */

    *(bufptr = &tempc[BUFLEN - 1]) = 0;

    do {*--bufptr = abs(uval % 10) + '0';}  while(uval /= 10);
    if (neg) *--bufptr = '-';

    memcpy(buf,bufptr, uval = (tempc + BUFLEN) - bufptr);
    return uval - 1;    /* DON'T COUNT NULL TERMINATION */

    Influenced by original versions of ltoa and fcvt in TI ccs libc.

}

#include <ctype.h>
#include <math.h>

int longtostrn(long val, char *buf, size_t buflen);

int doubletostrn(long double value, int ndigit, int *decpt, int *sign, char *buf, size_t buflen)

{
    static char out[400]; 

    int   digits = 0;         /* NUMBER OF DIGITS BEFORE .       */
    char *pos    = out + 1;     
    int   temp;

    out[0] = '0';                      /* JUST IN CASE WE ROUND.          */
    ndigit++;                          /* DO ONE EXTRA DIGIT FOR ROUNDING */

    /*--------------------------------------------------------------------*/
    /* IF TRYING TO CONVERT INFINITY, RETURN HUGE_VALL OF PROPER SIGN     */
    /*--------------------------------------------------------------------*/
    if (((_hi(value) >> 20) & 0x7ff) == 0x7ff) 
	value = (_hi(value) & 0x80000000) ? -HUGE_VALL : HUGE_VALL;

    /*--------------------------------------------------------------------*/
    /* PERFORM PRESCALING - MAKE SURE NUMBER HAS INTEGRAL WHOLE PART      */
    /*--------------------------------------------------------------------*/
    if (*sign = (value < 0)) value = -value;
    while (value > 0x7FFFFFFF) { value /= 10; digits++; }
    while (value && value < 1) { value *= 10; digits--; }

    /*--------------------------------------------------------------------*/
    /* WRITE OUT INTEGRAL PART OF NUMBER.                                 */
    /*--------------------------------------------------------------------*/
    pos   += temp    = ltoa((long)value, pos);
    *decpt = digits + temp; 

    /*--------------------------------------------------------------------*/
    /* ndigit MAY BE AFFECTED BY PRE-SCALING ...                          */
    /*--------------------------------------------------------------------*/
    ndigit += digits;

    /*--------------------------------------------------------------------*/
    /* WRITE OUT FRACTIONAL PART OF NUMBER                                */
    /*--------------------------------------------------------------------*/
    if (ndigit > 0) do
    {
         value -= (int)value;
         *pos++ = (int)(value *= 10.0) + '0';
    }
    while (--ndigit);

    /*--------------------------------------------------------------------*/
    /* PERFORM ROUNDING.  NOTE THAT pos IS CURRENTLY POINTING AT AN EXTRA */
    /* DIGIT WHICH WAS CONVERTED FOR THIS PURPOSE.                        */
    /*--------------------------------------------------------------------*/
    if (*--pos >= '5')
    {
       char *ptr = pos;
       while ((*--ptr += 1) > '9') *ptr = '0';
       
       if (ptr == out) 
       {
          if (pos > out) *pos   = 0;
          else           out[1] = 0;
          
          *decpt += 1;
          return out;
       }
    }
	
    *pos = 0;
    return out + 1;
}

