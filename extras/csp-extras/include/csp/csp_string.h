#ifndef __CSP_STRING_H__
#define __CSP_STRING_H__

static inline size_t strnlen (const char *string, size_t length);

static inline size_t strnlen (const char *string, size_t length)
{
    char *ret = memchr (string, 0, length);
    return ret ? ret - string : length;
}
#endif
