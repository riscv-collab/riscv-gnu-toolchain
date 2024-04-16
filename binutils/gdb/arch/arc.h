/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_ARC_H
#define ARCH_ARC_H

#include "gdbsupport/tdesc.h"

/* Supported ARC ISAs.  */
enum arc_isa
{
  ARC_ISA_ARCV1 = 1,  /* a.k.a. ARCompact (ARC600, ARC700)  */
  ARC_ISA_ARCV2	      /* such as ARC EM and ARC HS  */
};

struct arc_arch_features
{
  arc_arch_features (int reg_size, arc_isa isa)
    : reg_size (reg_size), isa (isa)
  {}

  /* Register size in bytes.  Possible values are 4, and 8.  A 0 indicates
     an uninitialised value.  */
  const int reg_size;

  /* See ARC_ISA enum.  */
  const arc_isa isa;

  /* Equality operator.  */
  bool operator== (const struct arc_arch_features &rhs) const
  {
    return (reg_size == rhs.reg_size && isa == rhs.isa);
  }

  /* Inequality operator.  */
  bool operator!= (const struct arc_arch_features &rhs) const
  {
    return !(*this == rhs);
  }

  /* Used by std::unordered_map to hash the feature sets.  The hash is
     calculated in the manner below:
     REG_SIZE |  ISA
      5-bits  | 4-bits  */

  std::size_t hash () const noexcept
  {
    std::size_t val = ((reg_size & 0x1f) << 8 | (isa & 0xf) << 0);
    return val;
  }
};

#ifdef GDBSERVER

/* Create and return a target description that is compatible with FEATURES.
   The only external client of this must be the gdbserver which manipulates
   the returned data.  */

target_desc_up arc_create_target_description
	(const struct arc_arch_features &features);

#else

/* Lookup the cache for a target description matching the FEATURES.
   If nothing is found, then create one and return it.  */

const target_desc *arc_lookup_target_description
	(const struct arc_arch_features &features);

#endif /* GDBSERVER */


#endif /* ARCH_ARC_H */
