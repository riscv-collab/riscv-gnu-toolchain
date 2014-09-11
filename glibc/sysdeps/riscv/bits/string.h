/* This file should provide inline versions of string functions.

   Surround GCC-specific parts with #ifdef __GNUC__, and use `__extern_inline'.

   This file should define __STRING_INLINES if functions are actually defined
   as inlines.  */

#ifndef _BITS_STRING_H
#define _BITS_STRING_H	1

#define _STRING_ARCH_unaligned   0

#if defined(__GNUC__) && !defined(__cplusplus)

static inline unsigned long __libc_detect_null(unsigned long w)
{
  unsigned long mask = 0x7f7f7f7f;
  if (sizeof(long) == 8)
    mask = ((mask << 16) << 16) | mask;
  return ~(((w & mask) + mask) | w | mask);
}

#define _HAVE_STRING_ARCH_memcpy 1
#define __use_memcpy_align(k, d, s, n) \
  (__builtin_constant_p(n) && (n) % (k) == 0 && (n) <= 64 && \
   __alignof__(*(d)) >= (k) && __alignof__(*(s)) >= (k))
#define memcpy(d, s, n) \
  (__use_memcpy_align(8, d, s, n) ? __memcpy_align8(d, s, n) : \
   __use_memcpy_align(4, d, s, n) ? __memcpy_align4(d, s, n) : \
   __memcpy_g(d, s, n))

#define __declare_memcpy_align(size, type) \
  static inline void *__memcpy_align ## size(void *__restrict __dest, \
                        __const void *__restrict __src, size_t __n) { \
    type *__d = (type*)__dest; \
    const type *__s = (const type*)__src, *__e = (const type*)(__src + __n); \
    while (__s < __e) { \
      type __t = *__s; \
      __d++, __s++; \
      *(__d-1) = __t; \
    } \
    return __dest; \
  }
__declare_memcpy_align(8, long long)
__declare_memcpy_align(4, int)

#ifdef _LIBC
extern void *__memcpy_g (void *__restrict __dest,
                         __const void *__restrict __src, size_t __n);
libc_hidden_proto (__memcpy_g)
#else
# define __memcpy_g memcpy
#endif

#endif /* __GNUC__ && !__cplusplus */

#endif /* bits/string.h */
