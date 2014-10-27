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

	li	a7, SYS_ify (rt_sigprocmask)
	scall

	bltz	a0, 99f

#ifdef __riscv_hard_float
	lw	t1, MCONTEXT_FSR(t0)

	fld	fs0, ( 8 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs1, ( 9 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs2, (18 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs3, (19 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs4, (20 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs5, (21 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs6, (22 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs7, (23 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs8, (24 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs9, (25 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs10,(26 * 8 + MCONTEXT_FPREGS)(t0)
	fld	fs11,(27 * 8 + MCONTEXT_FPREGS)(t0)

	fssr	t1
#endif /* __riscv_hard_float */

	/* Note the contents of argument registers will be random
	   unless makecontext() has been called.  */
	REG_L	t1, MCONTEXT_PC(t0)
	REG_L	ra, ( 1 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	sp, ( 2 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	tp, ( 4 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s0, ( 8 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s1, ( 9 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a0, (10 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a1, (11 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a2, (12 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a3, (13 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a4, (14 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a5, (15 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a6, (16 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	a7, (17 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s2, (18 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s3, (19 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s4, (20 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s5, (21 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s6, (22 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s7, (23 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s8, (24 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s9, (25 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s10,(26 * SZREG + MCONTEXT_GREGS)(t0)
	REG_L	s11,(27 * SZREG + MCONTEXT_GREGS)(t0)

	jr	t1

99:	j	__syscall_error

PSEUDO_END (__setcontext)

weak_alias (__setcontext, setcontext)
