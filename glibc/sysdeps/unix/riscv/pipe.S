/* Copyright (C) 1992, 1995, 1997, 2000, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Brendan Kehoe (brendan@zen.org).

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

SYSCALL__ (pipe, 1)
	/* Plop in the two descriptors.  */
/*	sw v0, 0(a0)
	sw v1, 4(a0)

*/
	/* Go out with a clean status.  */
	move v0, zero
	ret
PSEUDO_END(__pipe)

libc_hidden_def (__pipe)
weak_alias (__pipe, pipe)
