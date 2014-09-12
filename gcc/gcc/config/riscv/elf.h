/* Target macros for mips*-elf targets.
   Copyright (C) 1994, 1997, 1999, 2000, 2002, 2003, 2004, 2007, 2010
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Leave the linker script to choose the appropriate libraries.  */
#undef  LIB_SPEC
#define LIB_SPEC ""

#undef  STARTFILE_SPEC
#define STARTFILE_SPEC "crt0%O%s crtbegin%O%s"

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC "crtend%O%s"

#define NO_IMPLICIT_EXTERN_C 1
