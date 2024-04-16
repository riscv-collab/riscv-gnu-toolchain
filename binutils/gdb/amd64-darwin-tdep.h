/* Target-dependent code for Darwin x86-64.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef AMD64_DARWIN_TDEP_H
#define AMD64_DARWIN_TDEP_H

/* Mapping between the general-purpose registers in Darwin x86-64 thread
   state and GDB's register cache layout.
   Indexed by amd64_regnum.  */
extern int amd64_darwin_thread_state_reg_offset[];
extern const int amd64_darwin_thread_state_num_regs;

#endif /* AMD64_DARWIN_TDEP_H */
