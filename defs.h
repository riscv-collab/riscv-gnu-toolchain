// defs.h - Declarations and definitions

#ifndef _DEFS_H_
#define _DEFS_H_

#include <stddef.h>
#include <stdint.h>

#define EINVAL 1

// com.c
extern void com_init(void);
extern char com_getc(void);
extern void com_putc(char c);

// cons.c
extern char getchar(void);
extern void putchar(char c);
extern void puts(const char * s);
extern char * getsn(char * buf, size_t n);
extern void printf(const char *fmt, ... );

// halt.c
extern void halt(void) __attribute__ ((noreturn));
extern void panic(const char * msg) __attribute__ ((noreturn));

#if 0
// string.c
extern int memcmp(const void *v1, const void *v2, size_t n);
extern void* memmove(void *dst, const void *src, size_t n);
extern void memcpy(void *dst, const void *src, size_t n);
extern int strncmp(const char *p, const char *q, size_t n);
extern char* strncpy(char *s, const char *t, int n);
extern size_t strlen(const char *s);
extern void* memset(void *dst, int c, size_t n);
#endif

#endif // _DEFS_H_