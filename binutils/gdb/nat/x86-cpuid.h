/* C API for x86 cpuid insn.
   Copyright (C) 2007-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef NAT_X86_CPUID_H
#define NAT_X86_CPUID_H

/* Always include the header for the cpu bit defines.  */
#include "x86-gcc-cpuid.h"

#ifndef __cplusplus
/* This header file is also used in C code for some test-cases, so define
   nullptr in C terms to avoid a compilation error.  */
#define nullptr ((void *) 0)
#endif

/* Return cpuid data for requested cpuid level, as found in returned
   eax, ebx, ecx and edx registers.  The function checks if cpuid is
   supported and returns 1 for valid cpuid information or 0 for
   unsupported cpuid level.  Pointers may be non-null.  */

static __inline int
x86_cpuid (unsigned int __level,
	    unsigned int *__eax, unsigned int *__ebx,
	    unsigned int *__ecx, unsigned int *__edx)
{
  unsigned int __scratch;

  if (!__eax)
    __eax = &__scratch;
  if (!__ebx)
    __ebx = &__scratch;
  if (!__ecx)
    __ecx = &__scratch;
  if (!__edx)
    __edx = &__scratch;

  return __get_cpuid (__level, __eax, __ebx, __ecx, __edx);
}

/* Return cpuid data for requested cpuid level and sub-level, as found
   in returned eax, ebx, ecx and edx registers.  The function checks
   if cpuid is supported and returns 1 for valid cpuid information or
   0 for unsupported cpuid level.  Pointers may be non-null.  */

static __inline int
x86_cpuid_count (unsigned int __level, unsigned int __sublevel,
		 unsigned int *__eax, unsigned int *__ebx,
		 unsigned int *__ecx, unsigned int *__edx)
{
  unsigned int __scratch;

  if (__eax == nullptr)
    __eax = &__scratch;
  if (__ebx == nullptr)
    __ebx = &__scratch;
  if (__ecx == nullptr)
    __ecx = &__scratch;
  if (__edx == nullptr)
    __edx = &__scratch;

  return __get_cpuid_count (__level, __sublevel, __eax, __ebx, __ecx, __edx);
}

#ifndef __cplusplus
/* Avoid leaking this local definition beyond the scope of this header
   file.  */
#undef nullptr
#endif

#endif /* NAT_X86_CPUID_H */
