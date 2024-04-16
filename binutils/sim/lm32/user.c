/* Semantics for user defined instructions on the Lattice Mico32.
   Contributed by Jon Beniston <jon@beniston.com>

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

/* This must come before any other includes.  */
#include "defs.h"

#define WANT_CPU lm32bf
#define WANT_CPU_LM32BF

#include "sim-main.h"

/* Handle user defined instructions.  */

UINT
lm32bf_user_insn (SIM_CPU * current_cpu, INT r0, INT r1, UINT imm)
{
  /* FIXME: Should probably call code in a user supplied library.  */
  return 0;
}
