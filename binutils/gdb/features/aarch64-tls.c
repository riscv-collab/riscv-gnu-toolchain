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

#include "gdbsupport/tdesc.h"

/* This function is NOT auto generated from xml.

   Create the aarch64 description containing the TLS registers.  TPIDR is
   always available, but TPIDR2 is only available on some systems.

   COUNT is the number of registers in this set.  The minimum is 1.  */

static int
create_feature_aarch64_tls (struct target_desc *result, long regnum, int count)
{
  /* TPIDR is always present.  */
  gdb_assert (count >= 1);

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.tls");
  tdesc_create_reg (feature, "tpidr", regnum++, 1, NULL, 64, "data_ptr");

  /* Add TPIDR2.  */
  if (count > 1)
    tdesc_create_reg (feature, "tpidr2", regnum++, 1, NULL, 64, "data_ptr");

  return regnum;
}
