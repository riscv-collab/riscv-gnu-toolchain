/* Trap handlers for eBPF.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

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

#define WANT_CPU bpfbf
#define WANT_CPU_BPFBF

#include <stdlib.h>

#include "sim-main.h"

SEM_PC
sim_engine_invalid_insn (SIM_CPU *current_cpu ATTRIBUTE_UNUSED,
                         IADDR cia ATTRIBUTE_UNUSED,
                         SEM_PC pc ATTRIBUTE_UNUSED)
{
  /* Can't just return 0 here: the return value is used to set vpc
     (see decdde-{le,be}.c)
     Returning 0 will cause an infinite loop! */
  abort();
}
