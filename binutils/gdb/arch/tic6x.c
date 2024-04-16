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

#include "gdbsupport/tdesc.h"
#include "tic6x.h"
#include "gdbsupport/common-defs.h"

#include "../features/tic6x-core.c"
#include "../features/tic6x-gp.c"
#include "../features/tic6x-c6xp.c"

/* Create tic6x target descriptions according to FEATURE.  */

target_desc *
tic6x_create_target_description (enum c6x_feature feature)
{
  target_desc_up tdesc = allocate_target_description ();

  set_tdesc_architecture (tdesc.get (), "tic6x");
  set_tdesc_osabi (tdesc.get (), "GNU/Linux");

  long regnum = 0;

  regnum = create_feature_tic6x_core (tdesc.get (), regnum);

  if (feature == C6X_GP || feature == C6X_C6XP)
    regnum = create_feature_tic6x_gp (tdesc.get (), regnum);

  if (feature == C6X_C6XP)
    regnum = create_feature_tic6x_c6xp (tdesc.get (), regnum);

  return tdesc.release ();
}
