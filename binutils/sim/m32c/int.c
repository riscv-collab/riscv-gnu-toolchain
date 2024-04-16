/* int.c --- M32C interrupt handling.

Copyright (C) 2005-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include "int.h"
#include "cpu.h"
#include "mem.h"

static void
trigger_interrupt (int addr, int clear_u)
{
  int s = get_reg (sp);
  int f = get_reg (flags);
  int p = get_reg (pc);

  if (clear_u)
    set_flags (FLAGBIT_U, 0);
  set_flags (FLAGBIT_I | FLAGBIT_D, 0);

  if (A16)
    {
      s -= 4;
      put_reg (sp, s);
      mem_put_hi (s, p);
      mem_put_qi (s + 2, f);
      mem_put_qi (s + 3, ((f >> 4) & 0x0f) | (p >> 16));
    }
  else
    {
      s -= 6;
      put_reg (sp, s);
      mem_put_si (s, p);
      mem_put_hi (s + 4, f);
    }
  put_reg (pc, mem_get_psi (addr));
}

void
trigger_fixed_interrupt (int addr)
{
  trigger_interrupt (addr, 1);
}

void
trigger_based_interrupt (int vector)
{
  int addr = get_reg (intb) + vector * 4;
  trigger_interrupt (addr, vector <= 31);
}

void
trigger_peripheral_interrupt (int vector, int icaddr)
{
  unsigned char old_ic = mem_get_qi (icaddr);
  int addr = get_reg (intb) + vector * 4;
  trigger_interrupt (addr, 1);
  put_reg (flags, (get_reg (flags) & 0x8fff) | ((old_ic & 7) << 12));
  mem_put_qi (icaddr, old_ic & ~ 0x08);
}
