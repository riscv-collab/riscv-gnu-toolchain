/* x86 XSAVE extended state functions.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef NAT_X86_XSTATE_H
#define NAT_X86_XSTATE_H

#include "gdbsupport/x86-xstate.h"

/* Return the size of the XSAVE extended state fetched via CPUID.  */

int x86_xsave_length ();

/* Return the layout (size and offsets) of the XSAVE extended regions
   for the running host.  Offsets of each of the enabled regions in
   XCR0 are fetched via CPUID.  */

x86_xsave_layout x86_fetch_xsave_layout (uint64_t xcr0, int len);

#endif /* NAT_X86_XSTATE_H */
