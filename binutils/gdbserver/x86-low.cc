/* Low level support for x86 (i386 and x86-64).

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "x86-low.h"

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */

void
x86_low_init_dregs (struct x86_debug_reg_state *state)
{
  int i;

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      state->dr_mirror[i] = 0;
      state->dr_ref_count[i] = 0;
    }
  state->dr_control_mirror = 0;
  state->dr_status_mirror  = 0;
}
