#ifndef _ABSAT_LIB_H_
#define _ABSAT_LIB_H_
/*
    Alberta Sat light-weight library
*/

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

_CODE_ACCESS int32_t StrApFmtV( char *buf, size_t bufsize, 
    const char * __restrict format, va_list ap );

_CODE_ACCESS int32_t StrApFmt( char *buf, size_t bufsize,
    const char * __restrict format, ... );

_CODE_ACCESS int32_t StrInBuf( char *buf, size_t bufsize, char *inbuf, size_t inbufsize );

_CODE_ACCESS int32_t StrApBuf( char *buf, size_t bufsize, char *inbuf, size_t inbufsize );

_CODE_ACCESS int32_t StrApStr( char *buf, size_t bufsize, char *str );

_CODE_ACCESS int32_t StrApChar( char *buf, size_t bufsize, char c );

_CODE_ACCESS int32_t StrApHex( char *buf, size_t bufsize, int32_t val );

_CODE_ACCESS int32_t StrApHex32( char *buf, size_t bufsize, int32_t val );

_CODE_ACCESS int32_t StrApDec( char *buf, size_t bufsize, int32_t val );
#endif
