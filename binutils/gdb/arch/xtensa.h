/* Common Target-dependent code for the Xtensa port of GDB, the GNU debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_XTENSA_H
#define ARCH_XTENSA_H

/* Xtensa ELF core file register set representation ('.reg' section).
   Copied from target-side ELF header <xtensa/elf.h>.  */

typedef uint32_t xtensa_elf_greg_t;

typedef struct
{
  xtensa_elf_greg_t pc;
  xtensa_elf_greg_t ps;
  xtensa_elf_greg_t lbeg;
  xtensa_elf_greg_t lend;
  xtensa_elf_greg_t lcount;
  xtensa_elf_greg_t sar;
  xtensa_elf_greg_t windowstart;
  xtensa_elf_greg_t windowbase;
  xtensa_elf_greg_t threadptr;
  xtensa_elf_greg_t reserved[7+48];
  xtensa_elf_greg_t ar[64];
} xtensa_elf_gregset_t;

#define XTENSA_ELF_NGREG (sizeof (xtensa_elf_gregset_t) \
			  / sizeof (xtensa_elf_greg_t))

#define C0_NREGS   16	/* Number of A-registers to track in call0 ABI.  */

#endif /* ARCH_XTENSA_H */
