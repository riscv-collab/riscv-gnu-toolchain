/* Inline math functions for RISC-V.
   Copyright (C) 2011
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _MATH_H
# error "Never use <bits/mathinline.h> directly; include <math.h> instead."
#endif

#include <bits/wordsize.h>

#ifdef __GNUC__

#if defined __USE_ISOC99
# undef isgreater
# undef isgreaterequal
# undef isless
# undef islessequal
# undef islessgreater
# undef isunordered

# define isgreater(x, y) ((x) > (y))
# define isgreaterequal(x, y) ((x) >= (y))
# define isless(x, y) ((x) < (y))
# define islessequal(x, y) ((x) <= (y))
# define islessgreater(x, y) (!!(isless(x, y) + isgreater(x, y)))
# define isunordered(x, y) (((x) == (x)) + ((y) == (y)) < 2)

# ifndef __extern_inline
#  define __MATH_INLINE __inline
# else
#  define __MATH_INLINE __extern_inline
# endif  /* __cplusplus */

__MATH_INLINE int __attribute_used__ __signbit (double __x)
{
  union { double __d; long __i[sizeof(double)/sizeof(long)]; } __u;
  __u.__d = __x;
  return __u.__i[sizeof(double)/sizeof(long)-1] < 0;
}

__MATH_INLINE int __attribute_used__ __signbitf (float __x)
{
  union { float __d; int __i; } __u;
  __u.__d = __x;
  return __u.__i < 0;
}

#endif /* __USE_ISOC99 */

#if (!defined __NO_MATH_INLINES || defined __LIBC_INTERNAL_MATH_INLINES) && defined __OPTIMIZE__

/* Nothing yet. */

#endif /* !__NO_MATH_INLINES && __OPTIMIZE__ */
#endif /* __GNUC__ */
