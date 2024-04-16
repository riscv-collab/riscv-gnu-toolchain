/* Ravenscar x86-64 target support.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "gdbcore.h"
#include "regcache.h"
#include "amd64-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "amd64-ravenscar-thread.h"

/* x86-64 Ravenscar stores registers as:

   type Context_Buffer is record
      RIP    : System.Address;
      RFLAGS : EFLAGS;
      RSP    : System.Address;

      RBX    : System.Address;
      RBP    : System.Address;
      R12    : System.Address;
      R13    : System.Address;
      R14    : System.Address;
      R15    : System.Address;
   end record;
*/
static const int register_layout[] =
{
  /* RAX */ -1,
  /* RBX */ 3 * 8,
  /* RCX */ -1,
  /* RDX */ -1,
  /* RSI */ -1,
  /* RDI */ -1,
  /* RBP */ 4 * 8,
  /* RSP */ 2 * 8,
  /* R8 */ -1,
  /* R9 */ -1,
  /* R10 */ -1,
  /* R11 */ -1,
  /* R12 */ 5 * 8,
  /* R13 */ 6 * 8,
  /* R14 */ 7 * 8,
  /* R15 */ 8 * 8,
  /* RIP */ 0 * 8,
  /* EFLAGS */ 1 * 8,
};

/* The ravenscar_arch_ops vector for AMD64 targets.  */

static struct ravenscar_arch_ops amd64_ravenscar_ops (register_layout);

/* Register amd64_ravenscar_ops in GDBARCH.  */

void
register_amd64_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &amd64_ravenscar_ops);
}
