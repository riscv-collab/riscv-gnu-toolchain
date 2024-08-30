#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "defs.h"

char getchar(void) {
  return com_getc();
}

void putchar(char c) {
  com_putc(c);
}

void puts(const char * s) {
  if (s != NULL) {
    while (*s != '\0')
      putchar(*s++);
  }

  putchar('\n');
}

char * getsn(char * buf, size_t n) {
  char * p = buf;
  char c;

  for (;;) {
    c = getchar();

    switch (c) {
      case '\n':
      case '\r':
        putchar('\n');
        *p = '\0';
        return buf;
      case '\b':
      case '\177':
        if (p != buf) {
          putchar('\b');
          putchar(' ');
          putchar('\b');
          p -= 1;
          n += 1;
        }
        break;
      default:
        if (n > 1) {
          putchar(c);
          *p++ = c;
          n -= 1;
        } else
          putchar('\a'); // bell
        break;
    }
  }
}

// pritnf from xv6, copyright (c) 2006-2019 Frans Kaashoek, Robert Morris,
// Russ Cox, Massachusetts Institute of Technology. See LICENSE.

static const char digits[] = "0123456789abcdef";

static void printint(int x, int base, int sign) {
  char buf[16];
  unsigned int xx;
  int i;

  if(sign && (sign = x < 0))
    xx = -x;
  else
    xx = x;

  i = 0;
  do {
    buf[i++] = digits[xx % base];
  } while((xx /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    putchar(buf[i]);
}

static void printptr(uint64_t x) {
  int i;
  putchar('0');
  putchar('x');
  for (i = 0; i < (sizeof(uint64_t) * 2); i++, x <<= 4)
    putchar(digits[x >> (sizeof(uint64_t) * 8 - 4)]);
}

// Print to the console. only understands %c, %d, %x, %p, %s.

void printf(const char *fmt, ... ) {
  va_list ap;
  int i, c;
  char *s;

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      putchar(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      putchar((char)va_arg(ap, int));
      break;
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64_t));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        putchar(*s);
      break;
    case '%':
      putchar('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      putchar('%');
      putchar(c);
      break;
    }
  }
  va_end(ap);
}