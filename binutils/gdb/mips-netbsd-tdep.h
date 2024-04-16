/* Common target dependent code for GDB on MIPS systems running NetBSD.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef MIPS_NBSD_TDEP_H
#define MIPS_NBSD_TDEP_H

void mipsnbsd_supply_reg (struct regcache *, const char *, int);
void mipsnbsd_fill_reg (const struct regcache *, char *, int);

void mipsnbsd_supply_fpreg (struct regcache *, const char *, int);
void mipsnbsd_fill_fpreg (const struct regcache *, char *, int);

#endif /* MIPS_NBSD_TDEP_H */
