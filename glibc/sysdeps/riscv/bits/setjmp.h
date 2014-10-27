/* Define the machine-dependent type `jmp_buf'.  RISC-V version.
   Copyright (C) 1992,1993,1995,1997,2000,2002,2003,2004,2005,2006
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

#ifndef _RISCV_BITS_SETJMP_H
#define _RISCV_BITS_SETJMP_H

typedef struct __jmp_buf_internal_tag
  {
    /* Program counter.  */
    long __pc;
    /* Callee-saved registers. */
    long __regs[12];
    /* Stack pointer.  */
    long __sp;
    /* Thread pointer. */
    long __tp;
    /* Floating point status register.  */
    long __fsr;

    /* Callee-saved floating point registers.
       Note that there are an even number of preceding words in this struct,
       so no padding will be inserted before __fpregs, even for RV32. */
    double __fpregs[12];
  } __jmp_buf[1];

#endif /* _RISCV_BITS_SETJMP_H */
