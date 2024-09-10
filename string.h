// string.h - String functions
// 

#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>
#include <stdarg.h>

extern int strcmp(const char * s1, const char * s2);
extern size_t strlen(const char * s);
extern int strncmp(const char * s1, const char * s2, size_t n);

extern void * memset(void * s, int c, size_t n);
extern void * memcpy(void * restrict dst, const void * restrict src, size_t n);
extern int memcmp(const void * p1, const void * p2, size_t n);

extern size_t snprintf(char * buf, size_t bufsz, const char * fmt, ...);
extern size_t vsnprintf(char * buf, size_t bufsz, const char * fmt, va_list ap);

extern size_t vgprintf (
    void (*putcfn)(char, void*), void * aux,
    const char * fmt, va_list ap);

#endif // _STRING_H_