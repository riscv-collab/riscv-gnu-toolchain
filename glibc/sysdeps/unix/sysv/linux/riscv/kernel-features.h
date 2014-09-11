/* Set flags signalling availability of kernel features based on given
   kernel version number.
   Copyright (C) 1999-2003, 2004, 2005, 2006 Free Software Foundation, Inc.
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

#include <sgidefs.h>

#if _RISCV_SIM == _ABIN32
# define __ASSUME_FCNTL64		1
#endif

#define __ASSUME_EVENTFD2		1
#define __ASSUME_SIGNALFD4		1
#define __ASSUME_CLONE_THREAD_FLAGS	1
#define __ASSUME_TGKILL			1
#define __ASSUME_UTIMES			1
#define __ASSUME_O_CLOEXEC		1
#define __ASSUME_SOCK_CLOEXEC		1
#define __ASSUME_IN_NONBLOCK		1
#define __ASSUME_PIPE2			1
#define __ASSUME_EVENTFD2		1
#define __ASSUME_SIGNALFD4		1
#define __ASSUME_DUP3			1
#define __ASSUME_ACCEPT4		1

#include_next <kernel-features.h>
