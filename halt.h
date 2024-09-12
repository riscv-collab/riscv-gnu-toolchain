// halt.h - halt and panic
//

#ifndef _HALT_H_
#define _HALT_H_

#include "console.h"

extern void halt_success(void) __attribute__ ((noreturn));
extern void halt_failure(void) __attribute__ ((noreturn));

extern void panic(const char * msg) __attribute__ ((noreturn));

#define assert(c) do { \
    if (!(c)) { \
        kprintf("ASSERTION FAILED (%s:%d)\n", __FILE__, __LINE__); \
        panic(0); \
    } \
} while (0)

#endif // _HALT_H_