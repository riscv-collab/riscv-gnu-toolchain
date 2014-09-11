/* Copyright (C) 2005, 2009 Free Software Foundation, Inc.
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

#ifndef	_LINK_H
# error "Never include <bits/link.h> directly; use <link.h> instead."
#endif

typedef struct La_mips_64_regs
{
  unsigned long lr_reg[8]; /* $a0 through $a7 */
  double lr_fpreg[8]; /* $f4 throgh $f11 */
  unsigned long lr_ra;
  unsigned long lr_sp;
} La_mips_64_regs;

/* Return values for calls from PLT on MIPS.  */
typedef struct La_mips_64_retval
{
  unsigned long lrv_v0;
  unsigned long lrv_v1;
  double lrv_fv0;
  double lrv_fv1;
} La_mips_64_retval;

__BEGIN_DECLS

#if _RISCV_SIM == _ABI32

extern Elf32_Addr la_mips_n32_gnu_pltenter (Elf32_Sym *__sym, unsigned int __ndx,
					    uintptr_t *__refcook,
					    uintptr_t *__defcook,
					    La_mips_64_regs *__regs,
					    unsigned int *__flags,
					    const char *__symname,
					    long int *__framesizep);
extern unsigned int la_mips_n32_gnu_pltexit (Elf32_Sym *__sym, unsigned int __ndx,
					     uintptr_t *__refcook,
					     uintptr_t *__defcook,
					     const La_mips_64_regs *__inregs,
					     La_mips_64_retval *__outregs,
					     const char *__symname);

#else

extern Elf64_Addr la_mips_n64_gnu_pltenter (Elf64_Sym *__sym, unsigned int __ndx,
					    uintptr_t *__refcook,
					    uintptr_t *__defcook,
					    La_mips_64_regs *__regs,
					    unsigned int *__flags,
					    const char *__symname,
					    long int *__framesizep);
extern unsigned int la_mips_n64_gnu_pltexit (Elf64_Sym *__sym, unsigned int __ndx,
					     uintptr_t *__refcook,
					     uintptr_t *__defcook,
					     const La_mips_64_regs *__inregs,
					     La_mips_64_retval *__outregs,
					     const char *__symname);

#endif

__END_DECLS
