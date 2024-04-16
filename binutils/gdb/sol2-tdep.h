/* Target-dependent code for Solaris.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#ifndef SOL2_TDEP_H
#define SOL2_TDEP_H 1

struct gdbarch;

int sol2_sigtramp_p (frame_info_ptr this_frame);

void sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch);

#endif /* sol2-tdep.h */
