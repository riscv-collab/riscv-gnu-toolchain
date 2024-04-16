/* Target-dependent code for the GNU C Library (glibc).

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

#include "defs.h"
#include "frame.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"

#include "glibc-tdep.h"

/* Calling functions in shared libraries.  */

/* See the comments for SKIP_SOLIB_RESOLVER at the top of infrun.c.
   This function:
   1) decides whether a PLT has sent us into the linker to resolve
      a function reference, and 
   2) if so, tells us where to set a temporary breakpoint that will
      trigger when the dynamic linker is done.  */

CORE_ADDR
glibc_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* The GNU dynamic linker is part of the GNU C library, and is used
     by all GNU systems (GNU/Hurd, GNU/Linux).  An unresolved PLT
     entry points to "_dl_runtime_resolve", which calls "fixup" to
     patch the PLT, and then passes control to the function.

     We look for the symbol `_dl_runtime_resolve', and find `fixup' in
     the same objfile.  If we are at the entry point of `fixup', then
     we set a breakpoint at the return address (at the top of the
     stack), and continue.
  
     It's kind of gross to do all these checks every time we're
     called, since they don't change once the executable has gotten
     started.  But this is only a temporary hack --- upcoming versions
     of GNU/Linux will provide a portable, efficient interface for
     debugging programs that use shared libraries.  */

  struct bound_minimal_symbol resolver 
    = lookup_bound_minimal_symbol ("_dl_runtime_resolve");

  if (resolver.minsym)
    {
      /* The dynamic linker began using this name in early 2005.  */
      struct bound_minimal_symbol fixup
	= lookup_minimal_symbol ("_dl_fixup", NULL, resolver.objfile);
      
      /* This is the name used in older versions.  */
      if (! fixup.minsym)
	fixup = lookup_minimal_symbol ("fixup", NULL, resolver.objfile);

      if (fixup.minsym && fixup.value_address () == pc)
	return frame_unwind_caller_pc (get_current_frame ());
    }

  return 0;
}      
