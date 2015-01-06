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

ENTRY (__longjmp)
	REG_L ra,  0*SZREG(a0)
	REG_L s0,  1*SZREG(a0)
	REG_L s1,  2*SZREG(a0)
	REG_L s2,  3*SZREG(a0)
	REG_L s3,  4*SZREG(a0)
	REG_L s4,  5*SZREG(a0)
	REG_L s5,  6*SZREG(a0)
	REG_L s6,  7*SZREG(a0)
	REG_L s7,  8*SZREG(a0)
	REG_L s8,  9*SZREG(a0)
	REG_L s9, 10*SZREG(a0)
	REG_L s10,11*SZREG(a0)
	REG_L s11,12*SZREG(a0)
	REG_L sp, 13*SZREG(a0)

#ifdef __riscv_hard_float
	REG_L a3, 15*SZREG(a0)

	fld  fs0, 16*SZREG+ 0*8(a0)
	fld  fs1, 16*SZREG+ 1*8(a0)
	fld  fs2, 16*SZREG+ 2*8(a0)
	fld  fs3, 16*SZREG+ 3*8(a0)
	fld  fs4, 16*SZREG+ 4*8(a0)
	fld  fs5, 16*SZREG+ 5*8(a0)
	fld  fs6, 16*SZREG+ 6*8(a0)
	fld  fs7, 16*SZREG+ 7*8(a0)
	fld  fs8, 16*SZREG+ 8*8(a0)
	fld  fs9, 16*SZREG+ 9*8(a0)
	fld  fs10,16*SZREG+10*8(a0)
	fld  fs11,16*SZREG+11*8(a0)

	fssr a3
#endif

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	ret

END(__longjmp)
