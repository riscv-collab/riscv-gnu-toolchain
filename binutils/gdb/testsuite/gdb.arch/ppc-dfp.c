/* Copyright 2008-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <elf.h>

#ifdef __powerpc64__
typedef Elf64_auxv_t auxv_t;
#else
typedef Elf32_auxv_t auxv_t;
#endif

#ifndef PPC_FEATURE_HAS_DFP
#define PPC_FEATURE_HAS_DFP	0x00000400
#endif

int
main (int argc, char *argv[], char *envp[], auxv_t auxv[])
{
  int i;

  for (i = 0; auxv[i].a_type != AT_NULL; i++)
    if (auxv[i].a_type == AT_HWCAP) {
      if (!(auxv[i].a_un.a_val & PPC_FEATURE_HAS_DFP))
	return 1;

      break;
    }

  asm ("mtfsfi 7, 5, 1\n");  /* Set DFP rounding mode.  */

  return 0;
}
