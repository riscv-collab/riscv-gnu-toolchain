/* i387-specific utility functions, for the remote server for GDB.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_I387_FP_H
#define GDBSERVER_I387_FP_H

void i387_cache_to_fsave (struct regcache *regcache, void *buf);
void i387_fsave_to_cache (struct regcache *regcache, const void *buf);

void i387_cache_to_fxsave (struct regcache *regcache, void *buf);
void i387_fxsave_to_cache (struct regcache *regcache, const void *buf);

void i387_cache_to_xsave (struct regcache *regcache, void *buf);
void i387_xsave_to_cache (struct regcache *regcache, const void *buf);

/* Set the XSAVE mask and fetch the XSAVE layout via CPUID.  */

void i387_set_xsave_mask (uint64_t xcr0, int len);

#endif /* GDBSERVER_I387_FP_H */
