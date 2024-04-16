/* Target-dependent code for Darwin x86.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef I386_DARWIN_TDEP_H
#define I386_DARWIN_TDEP_H

#include "frame.h"

/* Mapping between the general-purpose registers in Darwin x86 thread_state
   struct and GDB's register cache layout.  */
extern int i386_darwin_thread_state_reg_offset[];
extern const int i386_darwin_thread_state_num_regs;

int darwin_dwarf_signal_frame_p (struct gdbarch *, frame_info_ptr);

#endif /* I386_DARWIN_TDEP_H */
