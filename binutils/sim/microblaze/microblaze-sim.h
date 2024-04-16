/* Copyright 2009-2024 Free Software Foundation, Inc.

   This file is part of the Xilinx MicroBlaze simulator.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef MICROBLAZE_SIM_H
#define MICROBLAZE_SIM_H

#include "microblaze.h"

/* The machine state.
   This state is maintained in host byte order.  The
   fetch/store register functions must translate between host
   byte order and the target processor byte order.
   Keeping this data in target byte order simplifies the register
   read/write functions.  Keeping this data in native order improves
   the performance of the simulator.  Simulation speed is deemed more
   important.  */

/* The ordering of the microblaze_regset structure is matched in the
   gdb/config/microblaze/tm-microblaze.h file in the REGISTER_NAMES macro.  */
 struct microblaze_regset
{
  signed_4	regs[32];		/* primary registers */
  signed_4	spregs[2];		/* pc + msr */
  int		cycles;
  int		insts;
  unsigned_1	imm_enable;
  signed_2	imm_high;
};

#define MICROBLAZE_SIM_CPU(cpu) ((struct microblaze_regset *) CPU_ARCH_DATA (cpu))

#endif /* MICROBLAZE_SIM_H */
