/* Save current context.
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

/* int getcontext (ucontext_t *ucp) */

	.text
LEAF (__getcontext)
	REG_S	ra, MCONTEXT_PC(a0)
	REG_S	ra, ( 1 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	sp, ( 2 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s0, ( 8 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s1, ( 9 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	x0, (10 * SZREG + MCONTEXT_GREGS)(a0)	/* return 0 */
	REG_S	s2, (18 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s3, (19 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s4, (20 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s5, (21 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s6, (22 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s7, (23 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s8, (24 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s9, (25 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s10,(26 * SZREG + MCONTEXT_GREGS)(a0)
	REG_S	s11,(27 * SZREG + MCONTEXT_GREGS)(a0)

#ifdef __riscv_hard_float
	frsr	a1

	fsd	fs0, ( 8 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs1, ( 9 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs2, (18 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs3, (19 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs4, (20 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs5, (21 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs6, (22 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs7, (23 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs8, (24 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs9, (25 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs10,(26 * 8 + MCONTEXT_FPREGS)(a0)
	fsd	fs11,(27 * 8 + MCONTEXT_FPREGS)(a0)

	sw	a1, MCONTEXT_FSR(a0)
#endif /* __mips_hard_float */

/* rt_sigprocmask (SIG_BLOCK, NULL, &ucp->uc_sigmask, _NSIG8) */
	li	a3, _NSIG8
	add     a2, a0, UCONTEXT_SIGMASK
	move	a1, zero
	li	a0, SIG_BLOCK

	li	a7, SYS_ify (rt_sigprocmask)
	scall
	bltz	a0, 99f

	ret

99:	j	__syscall_error

PSEUDO_END (__getcontext)

weak_alias (__getcontext, getcontext)
