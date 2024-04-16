/* x86 XSAVE extended state functions.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/x86-xstate.h"
#include "nat/x86-cpuid.h"
#include "nat/x86-xstate.h"

/* Fetch the offset of a specific XSAVE extended region.  */

static int
xsave_feature_offset (uint64_t xcr0, int feature)
{
  uint32_t ebx;

  if ((xcr0 & (1ULL << feature)) == 0)
    return 0;

  if (!x86_cpuid_count (0xd, feature, nullptr, &ebx, nullptr, nullptr))
    return 0;
  return ebx;
}

/* See x86-xstate.h.  */

int
x86_xsave_length ()
{
  uint32_t ebx;

  if (!x86_cpuid_count (0xd, 0, nullptr, &ebx, nullptr, nullptr))
    return 0;
  return ebx;
}

/* See x86-xstate.h.  */

x86_xsave_layout
x86_fetch_xsave_layout (uint64_t xcr0, int len)
{
  x86_xsave_layout layout;
  layout.sizeof_xsave = len;
  layout.avx_offset = xsave_feature_offset (xcr0, X86_XSTATE_AVX_ID);
  layout.bndregs_offset = xsave_feature_offset (xcr0, X86_XSTATE_BNDREGS_ID);
  layout.bndcfg_offset = xsave_feature_offset (xcr0, X86_XSTATE_BNDCFG_ID);
  layout.k_offset = xsave_feature_offset (xcr0, X86_XSTATE_K_ID);
  layout.zmm_h_offset = xsave_feature_offset (xcr0, X86_XSTATE_ZMM_H_ID);
  layout.zmm_offset = xsave_feature_offset (xcr0, X86_XSTATE_ZMM_ID);
  layout.pkru_offset = xsave_feature_offset (xcr0, X86_XSTATE_PKRU_ID);
  return layout;
}
