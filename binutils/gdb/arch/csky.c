/* Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "csky.h"
#include <stdlib.h>

#include "../features/cskyv2-linux.c"

target_desc_up
csky_create_target_description (void)
{
  /* Now we should create a new target description.  */
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  std::string arch_name = "csky";
  set_tdesc_architecture (tdesc.get (), arch_name.c_str ());
#endif

  create_feature_cskyv2_linux (tdesc.get (), 0);

  return tdesc;
}
