/* Copyright 2017-2024 Free Software Foundation, Inc.

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

typedef Elf64_auxv_t auxv_t;

#ifndef PPC_FEATURE2_ARCH_2_07
#define PPC_FEATURE2_ARCH_2_07	0x80000000
#endif

extern void test_atomic_sequences (void);

int
main (int argc, char *argv[], char *envp[], auxv_t auxv[])
{
  int i;

  for (i = 0; auxv[i].a_type != AT_NULL; i++)
    if (auxv[i].a_type == AT_HWCAP2) {
      if (!(auxv[i].a_un.a_val & PPC_FEATURE2_ARCH_2_07))
        return 1;
      break;
    }

  test_atomic_sequences ();
  return 0;
}
