/* Common target-dependent code for ppc64.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef PPC64_TDEP_H
#define PPC64_TDEP_H

struct gdbarch;
class frame_info_ptr;
struct target_ops;

extern CORE_ADDR ppc64_skip_trampoline_code (frame_info_ptr frame,
					     CORE_ADDR pc);

extern CORE_ADDR ppc64_convert_from_func_ptr_addr (struct gdbarch *gdbarch,
						   CORE_ADDR addr,
						   struct target_ops *targ);

extern void ppc64_elf_make_msymbol_special (asymbol *,
					    struct minimal_symbol *);
#endif /* PPC64_TDEP_H  */
