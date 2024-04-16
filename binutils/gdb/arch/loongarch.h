/* Common target-dependent functionality for LoongArch

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_LOONGARCH_H
#define ARCH_LOONGARCH_H

#include "gdbsupport/tdesc.h"

/* Register numbers of various important registers.  */
enum loongarch_regnum
{
  LOONGARCH_RA_REGNUM = 1,		/* Return Address.  */
  LOONGARCH_SP_REGNUM = 3,		/* Stack Pointer.  */
  LOONGARCH_A0_REGNUM = 4,		/* First Argument/Return Value.  */
  LOONGARCH_A7_REGNUM = 11,		/* Seventh Argument/Syscall Number.  */
  LOONGARCH_FP_REGNUM = 22,		/* Frame Pointer.  */
  LOONGARCH_ORIG_A0_REGNUM = 32,	/* Syscall's original arg0.  */
  LOONGARCH_PC_REGNUM = 33,		/* Program Counter.  */
  LOONGARCH_BADV_REGNUM = 34,		/* Bad Vaddr for Addressing Exception.  */
  LOONGARCH_LINUX_NUM_GREGSET = 45,	/* 32 GPR, ORIG_A0, PC, BADV, RESERVED 10.  */
  LOONGARCH_ARG_REGNUM = 8,            /* r4-r11: general-purpose argument registers.
					  f0-f7: floating-point argument registers.  */
  LOONGARCH_FIRST_FP_REGNUM = LOONGARCH_LINUX_NUM_GREGSET,
  LOONGARCH_LINUX_NUM_FPREGSET = 32,
  LOONGARCH_FIRST_FCC_REGNUM = LOONGARCH_FIRST_FP_REGNUM + LOONGARCH_LINUX_NUM_FPREGSET,
  LOONGARCH_LINUX_NUM_FCC = 8,
  LOONGARCH_FCSR_REGNUM = LOONGARCH_FIRST_FCC_REGNUM + LOONGARCH_LINUX_NUM_FCC,
};

enum loongarch_fputype
{
  SINGLE_FLOAT = 1,
  DOUBLE_FLOAT = 2,
};

/* The set of LoongArch architectural features that we track that impact how
   we configure the actual gdbarch instance.  We hold one of these in the
   gdbarch_tdep structure, and use it to distinguish between different
   LoongArch gdbarch instances.

   The information in here ideally comes from the target description,
   however, if the target doesn't provide a target description then we will
   create a default target description by first populating one of these
   based on what we know about the binary being executed, and using that to
   drive default target description creation.  */

struct loongarch_gdbarch_features
{
  /* The size of the x-registers in bytes.  This is either 4 (loongarch32)
     or 8 (loongarch64).  No other value is valid.  Initialise to the invalid
     0 value so we can spot if one of these is used uninitialised.  */
  int xlen = 0;

  /* The type of floating-point.  This is either 1 (single float) or 2
     (double float).  No other value is valid.  Initialise to the invalid
     0 value so we can spot if one of these is used uninitialised.  */
  int fputype = 0;

  /* Equality operator.  */
  bool operator== (const struct loongarch_gdbarch_features &rhs) const
  {
    return (xlen == rhs.xlen);
  }

  /* Inequality operator.  */
  bool operator!= (const struct loongarch_gdbarch_features &rhs) const
  {
    return !((*this) == rhs);
  }

  /* Used by std::unordered_map to hash feature sets.  */
  std::size_t hash () const noexcept
  {
    std::size_t val = (xlen & 0x1f) << 5;
    return val;
  }
};

#ifdef GDBSERVER

/* Create and return a target description that is compatible with FEATURES.
   This is only used directly from the gdbserver where the created target
   description is modified after it is return.  */

target_desc_up loongarch_create_target_description
	(const struct loongarch_gdbarch_features features);

#else

/* Lookup an already existing target description matching FEATURES, or
   create a new target description if this is the first time we have seen
   FEATURES.  For the same FEATURES the same target_desc is always
   returned.  This is important when trying to lookup gdbarch objects as
   GDBARCH_LIST_LOOKUP_BY_INFO performs a pointer comparison on target
   descriptions to find candidate gdbarch objects.  */

const target_desc *loongarch_lookup_target_description
	(const struct loongarch_gdbarch_features features);

#endif /* GDBSERVER */

#endif /* ARCH_LOONGARCH_H  */
