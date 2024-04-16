/* Manage register sets.

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

#ifndef REGSET_H
#define REGSET_H 1

struct gdbarch;
struct regcache;

/* Data structure describing a register set.  */

typedef void (supply_regset_ftype) (const struct regset *, struct regcache *,
				    int, const void *, size_t);
typedef void (collect_regset_ftype) (const struct regset *, 
				     const struct regcache *,
				     int, void *, size_t);

struct regset
{
  /* Pointer to a "register map", for private use by the methods
     below.  Typically describes how the regset's registers are
     arranged in the buffer collected to or supplied from.  */
  const void *regmap;

  /* Function supplying values in a register set to a register cache.  */
  supply_regset_ftype *supply_regset;

  /* Function collecting values in a register set from a register cache.  */
  collect_regset_ftype *collect_regset;

  unsigned flags;
};

/* Values for a regset's 'flags' field.  */

#define REGSET_VARIABLE_SIZE 1	/* Accept a larger regset section size
				   in a core file without warning.  */

#endif /* regset.h */
