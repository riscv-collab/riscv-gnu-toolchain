/* OpenRISC simulator main header
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#define WITH_SCACHE_PBB 1

#include "opcodes/or1k-desc.h"
#include "opcodes/or1k-opc.h"
#include "sim-basics.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"

/* TODO: Move this to the CGEN generated files instead.  */
#include "or1k-sim.h"

#endif /* SIM_MAIN_H */
