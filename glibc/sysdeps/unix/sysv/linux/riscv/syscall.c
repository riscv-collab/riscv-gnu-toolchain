/* Copyright (C) 2001, 2002, 2003, 2005 Free Software Foundation, Inc.
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

#include <sysdep.h>

long syscall (long syscall_number, long arg1, long arg2, long arg3,
	      long arg4, long arg5, long arg6, long arg7)
{
  long ret;
  INTERNAL_SYSCALL_DECL (err);

  ret = INTERNAL_SYSCALL_NCS(syscall_number, err, 7, arg1, arg2, arg3, arg4,
			     arg5, arg6, arg7);

  if (INTERNAL_SYSCALL_ERROR_P (ret, err))
    {
      extern long __syscall_error (void) attribute_hidden;
      return __syscall_error();
    }

  return ret;
}
