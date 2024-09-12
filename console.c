// console.c - Console output
// 

#include "console.h"

#include <stdarg.h>
#include <stdint.h>

#include "serial.h" // provides com0 functions
#include "string.h"

// INTERNAL FUNCTION DECLARATIONS
// 

static void vprintf_putc(char c, void * aux);

// EXPORTED GLOBAL VARIABLES
//

int console_initialized = 0;

// EXPORTED FUNCTION DEFINITIONS
//

void console_init(void) {
    com0_init();
    
    console_initialized = 1;
}

void console_putchar(char c) {
    static char cprev = '\0';

    switch (c) {
    case '\r':
        com0_putc(c);
        com0_putc('\n');
        break;
    case '\n':
        if (cprev != '\r')
            com0_putc('\r');
        // nobreak
    default:
        com0_putc(c);
        break;
    }

    cprev = c;
}

char console_getchar(void) {
    static char cprev = '\0';
  char c;

  // Convert \r followed by any number of \n to just \n

  do {
    c = com0_getc();
  } while (c == '\n' && cprev == '\r');
  
  cprev = c;

  if (c == '\r')
    return '\n';
  else
    return c;
}

void console_puts(const char * str) {
    while (*str != '\0')
        console_putchar(*str++);
    console_putchar('\n');
}

char * console_getsn(char * buf, size_t n) {
    char * p = buf;
    char c;

    
    for (;;) {
        c = console_getchar();

        switch (c) {
        case '\r':
            break;		
        case '\n':
#ifdef CONSOLE_RAW
              console_putchar('\n');
#endif
            *p = '\0';
            return buf;
        case '\b':
        case '\177':
            if (p != buf) {
#ifdef CONSOLE_RAW
                console_putchar('\b');
                console_putchar(' ');
                console_putchar('\b');
#endif

                p -= 1;
                n += 1;
            }
            break;
        default:
            if (n > 1) {
#ifdef CONSOLE_RAW
                console_putchar(c);
#endif
                *p++ = c;
                n -= 1;
            } else {
#ifdef CONSOLE_RAW
                console_putchar('\a'); // bell
#endif
            }
      
            break;
        }
    }
}

size_t console_printf(const char * fmt, ...) {
    va_list ap;
    size_t n;

    va_start(ap, fmt);
    n = console_vprintf(fmt, ap);
    va_end(ap);
    return n;
}

size_t kprintf(const char * fmt, ...)
    __attribute__ ((alias("console_printf")));

size_t console_vprintf(const char * fmt, va_list ap) {
    return vgprintf(vprintf_putc, NULL, fmt, ap);
}

void console_labeled_printf (
    const char * label,
    const char * src_flname,
    int src_lineno,
    const char * fmt, ...)
{
    va_list ap;

    console_printf("%s: %s:%d: ", label, src_flname, src_lineno);

    va_start(ap, fmt);
    console_vprintf(fmt, ap);
    console_putchar('\n');
    va_end(ap);
}


// INTERNAL FUNCTION DEFINITIONS
//

void vprintf_putc(char c, void * __attribute__ ((unused)) aux) {
    console_putchar(c);
}