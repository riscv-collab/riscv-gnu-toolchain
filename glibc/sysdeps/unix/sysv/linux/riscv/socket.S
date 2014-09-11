/* Copyright (C) 1997, 1998, 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Miguel de Icaza <miguel@gnu.ai.mit.edu>, 1997.

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

#include <sysdep-cancel.h>
#include <socketcall.h>

#define P(a, b) P2(a, b)
#define P2(a, b) a##b

#ifndef NARGS
#ifdef socket
#error NARGS not defined
#endif
#define NARGS 3
#endif

	.text
/* The socket-oriented system calls are handled unusually in Linux.
   They are all gated through the single `socketcall' system call number.
   `socketcall' takes two arguments: the first is the subcode, specifying
   which socket function is being called; and the second is a pointer to
   the arguments to the specific function.

   The .S files for the other calls just #define socket and #include this.  */

#ifndef __socket
# ifndef NO_WEAK_ALIAS
#  define __socket P(__,socket)
# else
#  define __socket socket
# endif
#endif

LEAF (__socket)

	addi  sp, sp, -SZREG*NARGS

	REG_S a0, 0*SZREG(sp)
	REG_S a1, 1*SZREG(sp)
#if NARGS > 2
	REG_S a2, 2*SZREG(sp)
#if NARGS > 3
	REG_S a3, 3*SZREG(sp)
#if NARGS > 4
	REG_S a4, 4*SZREG(sp)
#if NARGS > 5
	REG_S a5, 5*SZREG(sp)
#endif
#endif
#endif
#endif

#if defined NEED_CANCELLATION && defined CENABLE
	SINGLE_THREAD_P(a0)
	bnez  a0, 1f
#endif
	li    a0, P(SOCKOP_,socket)   /* arg 1: socket subfunction */
	move  a1, sp			/* arg 2: parameter block */
	li    v0, SYS_ify (rt_sigprocmask)
	scall

	addi  sp, sp, SZREG*NARGS
	bltz	v0, 99f

#if defined NEED_CANCELLATION && defined CENABLE
1:sbreak
#endif

99: 	j	__syscall_error

END (__socket)

#ifndef NO_WEAK_ALIAS
weak_alias (__socket, socket)
#endif
