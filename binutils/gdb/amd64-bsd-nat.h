/* Native-dependent code for modern AMD64 BSD's.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef AMD64_BSD_NAT_H
#define AMD64_BSD_NAT_H

#include "x86-bsd-nat.h"

/* Helper functions.  See definitions.  */
extern void amd64bsd_fetch_inferior_registers (struct regcache *regcache,
					       int regnum);
extern void amd64bsd_store_inferior_registers (struct regcache *regcache,
					       int regnum);

/* A prototype *BSD/AMD64 target.  */

template<typename BaseTarget>
class amd64_bsd_nat_target : public x86bsd_nat_target<BaseTarget>
{
public:
  void fetch_registers (struct regcache *regcache, int regnum) override
  { amd64bsd_fetch_inferior_registers (regcache, regnum); }

  void store_registers (struct regcache *regcache, int regnum) override
  { amd64bsd_store_inferior_registers (regcache, regnum); }
};

#endif /* i386-bsd-nat.h */
