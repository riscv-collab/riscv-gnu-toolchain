/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "amd64.h"
#include "gdbsupport/x86-xstate.h"
#include <stdlib.h>

#include "../features/i386/64bit-avx.c"
#include "../features/i386/64bit-avx512.c"
#include "../features/i386/64bit-core.c"
#include "../features/i386/64bit-linux.c"
#include "../features/i386/64bit-mpx.c"
#include "../features/i386/64bit-segments.c"
#include "../features/i386/64bit-sse.c"
#include "../features/i386/pkeys.c"

#include "../features/i386/x32-core.c"

/* Create amd64 target descriptions according to XCR0.  If IS_X32 is
   true, create the x32 ones.  If IS_LINUX is true, create target
   descriptions for Linux.  If SEGMENTS is true, then include
   the "org.gnu.gdb.i386.segments" feature registers.  */

target_desc *
amd64_create_target_description (uint64_t xcr0, bool is_x32, bool is_linux,
				 bool segments)
{
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc.get (),
			  is_x32 ? "i386:x64-32" : "i386:x86-64");

  if (is_linux)
    set_tdesc_osabi (tdesc.get (), "GNU/Linux");
#endif

  long regnum = 0;

  if (is_x32)
    regnum = create_feature_i386_x32_core (tdesc.get (), regnum);
  else
    regnum = create_feature_i386_64bit_core (tdesc.get (), regnum);

  regnum = create_feature_i386_64bit_sse (tdesc.get (), regnum);
  if (is_linux)
    regnum = create_feature_i386_64bit_linux (tdesc.get (), regnum);
  if (segments)
    regnum = create_feature_i386_64bit_segments (tdesc.get (), regnum);

  if (xcr0 & X86_XSTATE_AVX)
    regnum = create_feature_i386_64bit_avx (tdesc.get (), regnum);

  if ((xcr0 & X86_XSTATE_MPX) && !is_x32)
    regnum = create_feature_i386_64bit_mpx (tdesc.get (), regnum);

  if (xcr0 & X86_XSTATE_AVX512)
    regnum = create_feature_i386_64bit_avx512 (tdesc.get (), regnum);

  if (xcr0 & X86_XSTATE_PKRU)
    regnum = create_feature_i386_pkeys (tdesc.get (), regnum);

  return tdesc.release ();
}
