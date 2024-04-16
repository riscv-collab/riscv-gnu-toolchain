/* Ravenscar SPARC target support.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbcore.h"
#include "regcache.h"
#include "sparc-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "sparc-ravenscar-thread.h"
#include "gdbarch.h"

/* Register offsets from a referenced address (exempli gratia the
   Thread_Descriptor).  The referenced address depends on the register
   number.  The Thread_Descriptor layout and the stack layout are documented
   in the GNAT sources, in sparc-bb.h.  */

static const int sparc_register_offsets[] =
{
  /* G0 - G7 */
  -1,   0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
  /* O0 - O7 */
  0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
  /* L0 - L7 */
  0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
  /* I0 - I7 */
  0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
  /* F0 - F31 */
  0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C,
  0x70, 0x74, 0x78, 0x7C, 0x80, 0x84, 0x88, 0x8C,
  0x90, 0x94, 0x99, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC,
  0xB0, 0xB4, 0xBB, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC,
  /* Y  PSR   WIM   TBR   PC    NPC   FPSR  CPSR */
  0x40, 0x20, 0x44, -1,   0x1C, -1,   0x4C, -1
};

static struct ravenscar_arch_ops sparc_ravenscar_ops (sparc_register_offsets,
						      SPARC_L0_REGNUM,
						      SPARC_I7_REGNUM);

/* Register ravenscar_arch_ops in GDBARCH.  */

void
register_sparc_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &sparc_ravenscar_ops);
}
