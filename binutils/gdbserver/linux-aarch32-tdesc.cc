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

#include "server.h"

#include "linux-aarch32-tdesc.h"

#include "tdesc.h"
#include "arch/aarch32.h"
#include <inttypes.h>

static struct target_desc *tdesc_aarch32;

/* See linux-aarch32-tdesc.h.  */

const target_desc *
aarch32_linux_read_description ()
{
  if (tdesc_aarch32 == nullptr)
    {
      tdesc_aarch32 = aarch32_create_target_description ();

      static const char *expedite_regs[] = { "r11", "sp", "pc", 0 };
      init_target_desc (tdesc_aarch32, expedite_regs);
    }
  return tdesc_aarch32;
}

/* See linux-aarch32-tdesc.h.  */

bool
is_aarch32_linux_description (const target_desc *tdesc)
{
  gdb_assert (tdesc != nullptr);
  return tdesc == tdesc_aarch32;
}
