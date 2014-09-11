/* Run-time dynamic linker data structures for loaded ELF shared objects.
   Copyright (C) 2000, 2002, 2003, 2006, 2007 Free Software Foundation, Inc.
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

#ifndef _MIPS_LDSODEFS_H
#define _MIPS_LDSODEFS_H 1

#include <elf.h>

struct La_mips_32_regs;
struct La_mips_32_retval;
struct La_mips_64_regs;
struct La_mips_64_retval;

#define ARCH_PLTENTER_MEMBERS						    \
    Elf32_Addr (*mips_o32_gnu_pltenter) (Elf32_Sym *, unsigned int,	    \
					 uintptr_t *, uintptr_t *,	    \
					 const struct La_mips_32_regs *,    \
					 unsigned int *, const char *name,  \
					 long int *framesizep);		    \
    Elf32_Addr (*mips_n32_gnu_pltenter) (Elf32_Sym *, unsigned int,	    \
					 uintptr_t *, uintptr_t *,	    \
					 const struct La_mips_64_regs *,    \
					 unsigned int *, const char *name,  \
					 long int *framesizep);		    \
    Elf64_Addr (*mips_n64_gnu_pltenter) (Elf64_Sym *, unsigned int,	    \
					 uintptr_t *, uintptr_t *,	    \
					 const struct La_mips_64_regs *,    \
					 unsigned int *, const char *name,  \
					 long int *framesizep);

#define ARCH_PLTEXIT_MEMBERS						    \
    unsigned int (*mips_o32_gnu_pltexit) (Elf32_Sym *, unsigned int,	    \
					  uintptr_t *, uintptr_t *,	    \
					  const struct La_mips_32_regs *,   \
					  struct La_mips_32_retval *,	    \
					  const char *);		    \
    unsigned int (*mips_n32_gnu_pltexit) (Elf32_Sym *, unsigned int,	    \
					  uintptr_t *, uintptr_t *,	    \
					  const struct La_mips_64_regs *,   \
					  struct La_mips_64_retval *,	    \
					  const char *);		    \
    unsigned int (*mips_n64_gnu_pltexit) (Elf64_Sym *, unsigned int,	    \
					  uintptr_t *, uintptr_t *,	    \
					  const struct La_mips_64_regs *,   \
					  struct La_mips_64_retval *,	    \
					  const char *);

/* The MIPS ABI specifies that the dynamic section has to be read-only.  */

#define DL_RO_DYN_SECTION 1

#include_next <ldsodefs.h>

#endif
