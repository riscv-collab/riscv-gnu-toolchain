/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <errno.h>

int pthread_spin_lock(pthread_spinlock_t* lock)
{
#ifdef __riscv_atomic
  int tmp1, tmp2;

  asm volatile ("\n\
  1:lw           %0, 0(%2)\n\
    li           %1, %3\n\
    bnez         %0, 1b\n\
    amoswap.w.aq %0, %1, 0(%2)\n\
    bnez         %0, 1b"
    : "=&r"(tmp1), "=&r"(tmp2) : "r"(lock), "i"(EBUSY)
  );

  return tmp1;
#else
  return pthread_mutex_lock(lock);
#endif
}
