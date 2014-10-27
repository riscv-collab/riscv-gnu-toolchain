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

/* vfork() is just a special case of clone().  */

#include <sys/asm.h>
#include <sysdep.h>
#define __ASSEMBLY__
#include <linux/sched.h>
#include <asm/signal.h>

#ifndef SAVE_PID
#define SAVE_PID
#endif

#ifndef RESTORE_PID
#define RESTORE_PID
#endif


/* int vfork() */

	.text
LEAF(__vfork)

	SAVE_PID

#if (CLONE_VFORK | CLONE_VM | SIGCHLD) != 0x4111
# error
#endif
	li		a0, (CLONE_VFORK | CLONE_VM | SIGCHLD)
	move		a1, sp
	li		a2, 0
	li		a3, 0
	li		a4, 0

	/* Do the system call */
	li		a7,__NR_clone
	scall

	bltz		a0,L(error)

	RESTORE_PID

	ret

	/* Something bad happened -- no child created.  */
L(error):
	j		__syscall_error

	END(__vfork)

libc_hidden_def(__vfork)
weak_alias (__vfork, vfork)
