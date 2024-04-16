/* Target-dependent code for FreeBSD x86.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef I386_FBSD_TDEP_H
#define I386_FBSD_TDEP_H

#include "gdbsupport/x86-xstate.h"
#include "regset.h"

/* Read the XSAVE extended state xcr0 value from the ABFD core file.
   If it appears to be valid, return it and fill LAYOUT with values
   inferred from that value.

   Otherwise, return 0 to indicate no state was found and leave LAYOUT
   untouched.  */
uint64_t i386_fbsd_core_read_xsave_info (bfd *abfd, x86_xsave_layout &layout);

/* Implement the core_read_x86_xsave_layout gdbarch method.  */
bool i386_fbsd_core_read_x86_xsave_layout (struct gdbarch *gdbarch,
					   x86_xsave_layout &layout);

/* The format of the XSAVE extended area is determined by hardware.
   Cores store the XSAVE extended area in a NT_X86_XSTATE note that
   matches the layout on Linux.  */
#define I386_FBSD_XSAVE_XCR0_OFFSET 464

extern const struct regset i386_fbsd_gregset;

#endif /* i386-fbsd-tdep.h */
