/* Definitions for virtual tail call frames unwinder for GDB.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

#ifndef DWARF2_FRAME_TAILCALL_H
#define DWARF2_FRAME_TAILCALL_H 1

class frame_info_ptr;
struct frame_unwind;

/* The tail call frame unwinder.  */

extern void
  dwarf2_tailcall_sniffer_first (frame_info_ptr this_frame,
				 void **tailcall_cachep,
				 const LONGEST *entry_cfa_sp_offsetp);

extern struct value *
  dwarf2_tailcall_prev_register_first (frame_info_ptr this_frame,
				       void **tailcall_cachep, int regnum);

extern const struct frame_unwind dwarf2_tailcall_frame_unwind;

#endif /* !DWARF2_FRAME_TAILCALL_H */
