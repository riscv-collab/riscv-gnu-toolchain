/* Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
#include "aarch32.h"

#include "../features/arm/arm-core.c"
#include "../features/arm/arm-tls.c"
#include "../features/arm/arm-vfpv3.c"

/* See aarch32.h.  */

target_desc *
aarch32_create_target_description ()
{
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc.get (), "arm");
#endif

  long regnum = 0;

  regnum = create_feature_arm_arm_core (tdesc.get (), regnum);
  /* Create a vfpv3 feature, then a blank NEON feature.  */
  regnum = create_feature_arm_arm_vfpv3 (tdesc.get (), regnum);
  tdesc_create_feature (tdesc.get (), "org.gnu.gdb.arm.neon");
  regnum = create_feature_arm_arm_tls (tdesc.get (), regnum);

  return tdesc.release ();
}
