/* Native-dependent code for Solaris SPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "regcache.h"

#include <sys/procfs.h>
#include "gregset.h"

#include "sparc-tdep.h"
#include "target.h"
#include "procfs.h"

/* This file provids the (temporary) glue between the Solaris SPARC
   target dependent code and the machine independent SVR4 /proc
   support.  */

/* Solaris 7 (Solaris 2.7, SunOS 5.7) and up support two process data
   models, the traditional 32-bit data model (ILP32) and the 64-bit
   data model (LP64).  The format of /proc depends on the data model
   of the observer (the controlling process, GDB in our case).  The
   Solaris header files conveniently define PR_MODEL_NATIVE to the
   data model of the controlling process.  If its value is
   PR_MODEL_LP64, we know that GDB is being compiled as a 64-bit
   program.

   Note that a 32-bit GDB won't be able to debug a 64-bit target
   process using /proc on Solaris.  */

#if PR_MODEL_NATIVE == PR_MODEL_LP64

#include "sparc64-tdep.h"

#define sparc_supply_gregset sparc64_supply_gregset
#define sparc_supply_fpregset sparc64_supply_fpregset
#define sparc_collect_gregset sparc64_collect_gregset
#define sparc_collect_fpregset sparc64_collect_fpregset

#define sparc_sol2_gregmap sparc64_sol2_gregmap
#define sparc_sol2_fpregmap sparc64_sol2_fpregmap

#else

#define sparc_supply_gregset sparc32_supply_gregset
#define sparc_supply_fpregset sparc32_supply_fpregset
#define sparc_collect_gregset sparc32_collect_gregset
#define sparc_collect_fpregset sparc32_collect_fpregset

#define sparc_sol2_gregmap sparc32_sol2_gregmap
#define sparc_sol2_fpregmap sparc32_sol2_fpregmap

#endif

void
supply_gregset (struct regcache *regcache, const prgregset_t *gregs)
{
  sparc_supply_gregset (&sparc_sol2_gregmap, regcache, -1, gregs);
}

void
supply_fpregset (struct regcache *regcache, const prfpregset_t *fpregs)
{
  sparc_supply_fpregset (&sparc_sol2_fpregmap, regcache, -1, fpregs);
}

void
fill_gregset (const struct regcache *regcache, prgregset_t *gregs, int regnum)
{
  sparc_collect_gregset (&sparc_sol2_gregmap, regcache, regnum, gregs);
}

void
fill_fpregset (const struct regcache *regcache,
	       prfpregset_t *fpregs, int regnum)
{
  sparc_collect_fpregset (&sparc_sol2_fpregmap, regcache, regnum, fpregs);
}
