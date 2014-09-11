/* Set current context.
   Copyright (C) 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Maciej W. Rozycki <macro@codesourcery.com>.

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
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <sysdep.h>
#include <sys/asm.h>

#include "ucontext_i.h"

/* int setcontext (const ucontext_t *ucp) */

	.text
LEAF (__setcontext)

	mv	t0, a0					/* t0 <- ucp */

/* rt_sigprocmask (SIG_SETMASK, &ucp->uc_sigmask, NULL, _NSIG8) */
	li	a3, _NSIG8
	move	a2, zero
	add     a1, a0, UCONTEXT_SIGMASK
	li	a0, SIG_SETMASK

	li	v0, SYS_ify (rt_sigprocmask)
	scall

	bltz	v0, 99f

#ifdef __riscv_hard_float
	lw	v1, MCONTEXT_FSR(t0)

	fld	fs0, ( 0 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs1, ( 1 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs2, ( 2 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs3, ( 3 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs4, ( 4 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs5, ( 5 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs6, ( 6 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs7, ( 7 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs8, ( 8 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs9, ( 9 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs10,(10 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs11,(11 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs12,(12 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs13,(13 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs14,(14 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs15,(15 * 8 + MCONTEXT_FPREGS)(t0)

	fssr	v1
#endif /* __mips_hard_float */

	/* Note the contents of argument registers will be random
	   unless makecontext() has been called.  */
	REG_L	v1, MCONTEXT_PC(t0)
	REG_L	ra, ( 1 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s0, ( 2 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s1, ( 3 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s2, ( 4 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s3, ( 5 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s4, ( 6 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s5, ( 7 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s6, ( 8 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s7, ( 9 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s8, (10 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s9, (11 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s10,(12 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s11,(13 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	sp, (14 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	tp, (15 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a0, (18 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a1, (19 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a2, (20 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a3, (21 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a4, (22 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a5, (23 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a6, (24 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a7, (25 * SZREG + MCONTEXT_GREGS)(t0)

	jr	v1

99:	j	__syscall_error

PSEUDO_END (__setcontext)

weak_alias (__setcontext, setcontext)
