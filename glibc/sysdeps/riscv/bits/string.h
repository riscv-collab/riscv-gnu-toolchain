/* This file should provide inline versions of string functions.

   Surround GCC-specific parts with #ifdef __GNUC__, and use `__extern_inline'.

   This file should define __STRING_INLINES if functions are actually defined
   as inlines.  */

#ifndef _BITS_STRING_H
#define _BITS_STRING_H	1

#define _STRING_INLINE_unaligned   0

#if defined(__GNUC__) && !defined(__cplusplus)

static __inline__ unsigned long __libc_detect_null(unsigned long w)
{
  unsigned long mask = 0x7f7f7f7f;
  if (sizeof(long) == 8)
    mask = ((mask << 16) << 16) | mask;
  return ~(((w & mask) + mask) | w | mask);
}

#endif /* __GNUC__ && !__cplusplus */

#endif /* bits/string.h */
