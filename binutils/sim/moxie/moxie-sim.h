/* Moxie Simulator definition.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Anthony Green <green@moxielogic.com>

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

#ifndef MOXIE_SIM_H
#define MOXIE_SIM_H

#define PCIDX 17

struct moxie_sim_cpu {
  /* To keep this default simulator simple, and fast, we use a direct
     vector of registers. The internal simulator engine then uses
     manifests to access the correct slot. */
  unsigned_word registers[19];
};

#define MOXIE_SIM_CPU(cpu) ((struct moxie_sim_cpu *) CPU_ARCH_DATA (cpu))

#endif
