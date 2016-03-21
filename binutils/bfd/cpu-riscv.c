/* BFD backend for RISC-V
   Copyright 2011-2015 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

/* This routine is provided two arch_infos and returns an arch_info
   that is compatible with both, or NULL if none exists.  */

static const bfd_arch_info_type *
riscv_compatible (const bfd_arch_info_type *a, const bfd_arch_info_type *b)
{
  if (a->arch != b->arch)
    return NULL;

  /* Machine compatibility is checked in
     _bfd_riscv_elf_merge_private_bfd_data.  */

  return a;
}

#define N(BITS_WORD, BITS_ADDR, NUMBER, PRINT, DEFAULT, NEXT)		\
  {							\
    BITS_WORD, /*  bits in a word */			\
    BITS_ADDR, /* bits in an address */			\
    8,	/* 8 bits in a byte */				\
    bfd_arch_riscv,					\
    NUMBER,						\
    "riscv",						\
    PRINT,						\
    3,							\
    DEFAULT,						\
    riscv_compatible,					\
    bfd_default_scan,					\
    bfd_arch_default_fill,				\
    NEXT,						\
  }

enum
{
  I_riscv64,
  I_riscv32
};

#define NN(index) (&arch_info_struct[(index) + 1])

static const bfd_arch_info_type arch_info_struct[] =
{
  N (64, 64, bfd_mach_riscv64, "riscv:rv64", FALSE, NN (I_riscv64)),
  N (32, 32, bfd_mach_riscv32, "riscv:rv32", FALSE, 0)
};

/* The default architecture is riscv:rv64.  */

const bfd_arch_info_type bfd_riscv_arch =
  N (64, 64, 0, "riscv", TRUE, &arch_info_struct[0]);
