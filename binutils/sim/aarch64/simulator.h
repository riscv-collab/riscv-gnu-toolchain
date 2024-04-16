/* simulator.h -- Prototypes for AArch64 simulator functions.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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

#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include <sys/types.h>
#include <setjmp.h>

#include "sim-main.h"
#include "decode.h"

#define TOP_LEVEL_RETURN_PC 0xffffffffffffffecULL

/* Call this to set the start stack pointer, frame pointer and pc
   before calling run or step.  Also sets link register to a special
   value (-20) so we can detect a top level return.  This function
   should be called from the sim setup routine running on the alt
   stack, and should pass in the current top of C stack for SP and
   the FP of the caller for fp.  PC should be sthe start of the
   AARCH64 code segment to be executed.  */

extern void         aarch64_init (sim_cpu *, uint64_t);

/* Call this to run from the current PC without stopping until we
   either return from the top frame, execute a halt or break or we
   hit an error.  */

extern void         aarch64_run (SIM_DESC);
extern const char * aarch64_get_func (SIM_DESC, uint64_t);
extern void         aarch64_init_LIT_table (void);

#endif /* _SIMULATOR_H */
