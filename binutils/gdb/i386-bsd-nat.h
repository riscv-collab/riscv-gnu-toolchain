/* Native-dependent code for modern i386 BSD's.

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

#ifndef I386_BSD_NAT_H
#define I386_BSD_NAT_H

#include "x86-bsd-nat.h"

/* Helper functions.  See definitions.  */
extern void i386bsd_fetch_inferior_registers (struct regcache *regcache,
					      int regnum);
extern void i386bsd_store_inferior_registers (struct regcache *regcache,
					      int regnum);

/* A prototype *BSD/i386 target.  */

template<typename BaseTarget>
class i386_bsd_nat_target : public x86bsd_nat_target<BaseTarget>
{
public:
  void fetch_registers (struct regcache *regcache, int regnum) override
  { i386bsd_fetch_inferior_registers (regcache, regnum); }

  void store_registers (struct regcache *regcache, int regnum) override
  { i386bsd_store_inferior_registers (regcache, regnum); }
};

#endif /* i386-bsd-nat.h */
