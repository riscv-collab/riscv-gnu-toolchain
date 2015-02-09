/* Copyright (C) 1996, 1997, 2000, 2002, 2003, 2004
	Free Software Foundation, Inc.
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
#include <sys/asm.h>

ENTRY (_setjmp)
  li	a1, 0
  j	__sigsetjmp
END (_setjmp)
ENTRY (setjmp)
  li	a1, 1
  /* Fallthrough */
END (setjmp)
ENTRY (__sigsetjmp)
	REG_S ra,  0*SZREG(a0)
	REG_S s0,  1*SZREG(a0)
	REG_S s1,  2*SZREG(a0)
	REG_S s2,  3*SZREG(a0)
	REG_S s3,  4*SZREG(a0)
	REG_S s4,  5*SZREG(a0)
	REG_S s5,  6*SZREG(a0)
	REG_S s6,  7*SZREG(a0)
	REG_S s7,  8*SZREG(a0)
	REG_S s8,  9*SZREG(a0)
	REG_S s9, 10*SZREG(a0)
	REG_S s10,11*SZREG(a0)
	REG_S s11,12*SZREG(a0)
	REG_S sp, 13*SZREG(a0)

#ifdef __riscv_hard_float
	frsr a3

	fsd  fs0, 16*SZREG+ 0*8(a0)
	fsd  fs1, 16*SZREG+ 1*8(a0)
	fsd  fs2, 16*SZREG+ 2*8(a0)
	fsd  fs3, 16*SZREG+ 3*8(a0)
	fsd  fs4, 16*SZREG+ 4*8(a0)
	fsd  fs5, 16*SZREG+ 5*8(a0)
	fsd  fs6, 16*SZREG+ 6*8(a0)
	fsd  fs7, 16*SZREG+ 7*8(a0)
	fsd  fs8, 16*SZREG+ 8*8(a0)
	fsd  fs9, 16*SZREG+ 9*8(a0)
	fsd  fs10,16*SZREG+10*8(a0)
	fsd  fs11,16*SZREG+11*8(a0)

	REG_S a3, 15*SZREG(a0)
#endif

#if !IS_IN (libc) && IS_IN (rtld)
  /* In ld.so we never save the signal mask.  */
  li a0, 0
  ret
#else
  /* Make a tail call to __sigjmp_save; it takes the same args.  */
  j __sigjmp_save
#endif


END(__sigsetjmp)

weak_alias(_setjmp, __GI__setjmp)
