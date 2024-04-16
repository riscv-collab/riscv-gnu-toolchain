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

#include "gdbsupport/common-defs.h"
#include "aarch64.h"
#include <stdlib.h>

#include "../features/aarch64-core.c"
#include "../features/aarch64-fpu.c"
#include "../features/aarch64-sve.c"
#include "../features/aarch64-pauth.c"
#include "../features/aarch64-mte.c"
#include "../features/aarch64-sme.c"
#include "../features/aarch64-sme2.c"
#include "../features/aarch64-tls.c"

/* See arch/aarch64.h.  */

target_desc *
aarch64_create_target_description (const aarch64_features &features)
{
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc.get (), "aarch64");
#endif

  long regnum = 0;

  regnum = create_feature_aarch64_core (tdesc.get (), regnum);

  if (features.vq == 0)
    regnum = create_feature_aarch64_fpu (tdesc.get (), regnum);
  else
    regnum = create_feature_aarch64_sve (tdesc.get (), regnum, features.vq);

  if (features.pauth)
    regnum = create_feature_aarch64_pauth (tdesc.get (), regnum);

  /* Memory tagging extension registers.  */
  if (features.mte)
    regnum = create_feature_aarch64_mte (tdesc.get (), regnum);

  /* TLS registers.  */
  if (features.tls > 0)
    regnum = create_feature_aarch64_tls (tdesc.get (), regnum, features.tls);

  if (features.svq)
    regnum = create_feature_aarch64_sme (tdesc.get (), regnum,
					 sve_vl_from_vq (features.svq));

  if (features.sme2)
    regnum = create_feature_aarch64_sme2 (tdesc.get (), regnum);

  return tdesc.release ();
}

/* See arch/aarch64.h.  */

CORE_ADDR
aarch64_remove_top_bits (CORE_ADDR pointer, CORE_ADDR mask)
{
  /* The VA range select bit is 55.  This bit tells us if we have a
     kernel-space address or a user-space address.  */
  bool kernel_address = (pointer & VA_RANGE_SELECT_BIT_MASK) != 0;

  /* Remove the top non-address bits.  */
  pointer &= ~mask;

  /* Sign-extend if we have a kernel-space address.  */
  if (kernel_address)
    pointer |= mask;

  return pointer;
}

/* See arch/aarch64.h.  */

CORE_ADDR
aarch64_mask_from_pac_registers (const CORE_ADDR cmask, const CORE_ADDR dmask)
{
  /* If the masks differ, default to using the one with the most coverage.  */
  if (dmask != cmask)
    return dmask > cmask ? dmask : cmask;

  return cmask;
}
