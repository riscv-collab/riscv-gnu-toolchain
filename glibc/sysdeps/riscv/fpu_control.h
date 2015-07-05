/* FPU control word bits.  Mips version.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2006, 2008
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Olaf Flebbe and Ralf Baechle.

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

#ifndef _FPU_CONTROL_H
#define _FPU_CONTROL_H

#include <features.h>

#ifdef __riscv_soft_float

#define _FPU_RESERVED 0xffffffff
#define _FPU_DEFAULT  0x00000000
typedef unsigned int fpu_control_t;
#define _FPU_GETCW(cw) (cw) = 0
#define _FPU_GETROUND(cw) (cw) = 0
#define _FPU_GETFLAGS(cw) (cw) = 0
#define _FPU_SETCW(cw) do { } while (0)
#define _FPU_SETROUND(cw) do { } while (0)
#define _FPU_SETFLAGS(cw) do { } while (0)
extern fpu_control_t __fpu_control;

#else /* __riscv_soft_float */

/* rounding control */
#define _FPU_RC_NEAREST 0x0
#define _FPU_RC_ZERO    0x1
#define _FPU_RC_DOWN    0x2
#define _FPU_RC_UP      0x3

#define _FPU_RESERVED   0    /* No reserved bits in FSR */

/* The fdlibm code requires strict IEEE double precision arithmetic,
   and no interrupts for exceptions, rounding to nearest.  */

#define _FPU_DEFAULT  0

/* IEEE:  same as above */
#define _FPU_IEEE     _FPU_DEFAULT

/* Type of the control word.  */
typedef unsigned int fpu_control_t __attribute__ ((__mode__ (__SI__)));

/* Macros for accessing the hardware control word.  */
#define _FPU_GETCW(cw) __asm__ volatile ("frsr %0" : "=r" (cw))
#define _FPU_GETROUND(cw) __asm__ volatile ("frrm %0" : "=r" (cw))
#define _FPU_GETFLAGS(cw) __asm__ volatile ("frflags %0" : "=r" (cw))
#define _FPU_SETCW(cw) __asm__ volatile ("fssr %z0" : : "rJ" (cw))
#define _FPU_SETROUND(cw) __asm__ volatile ("fsrm %z0" : : "rJ" (cw))
#define _FPU_SETFLAGS(cw) __asm__ volatile ("fsflags %z0" : : "rJ" (cw))

/* Default control word set at startup.  */
extern fpu_control_t __fpu_control;

#define _FCLASS(x) ({ int res; \
  if (sizeof(x) == 4) asm ("fclass.s %0, %1" : "=r"(res) : "f"(x)); \
  else if (sizeof(x) == 8) asm ("fclass.d %0, %1" : "=r"(res) : "f"(x)); \
  else abort(); \
  res; })

#define _FCLASS_MINF     (1<<0)
#define _FCLASS_MNORM    (1<<1)
#define _FCLASS_MSUBNORM (1<<2)
#define _FCLASS_MZERO    (1<<3)
#define _FCLASS_PZERO    (1<<4)
#define _FCLASS_PSUBNORM (1<<5)
#define _FCLASS_PNORM    (1<<6)
#define _FCLASS_PINF     (1<<7)
#define _FCLASS_SNAN     (1<<8)
#define _FCLASS_QNAN     (1<<9)
#define _FCLASS_ZERO     (_FCLASS_MZERO | _FCLASS_PZERO)
#define _FCLASS_SUBNORM  (_FCLASS_MSUBNORM | _FCLASS_PSUBNORM)
#define _FCLASS_NORM     (_FCLASS_MNORM | _FCLASS_PNORM)
#define _FCLASS_INF      (_FCLASS_MINF | _FCLASS_PINF)
#define _FCLASS_NAN      (_FCLASS_SNAN | _FCLASS_QNAN)

#endif /* __riscv_soft_float */

#endif	/* fpu_control.h */
