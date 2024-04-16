/* Wrapper around libiberty's safe-ctype.h for GDB, the GNU debugger.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef GDB_SAFE_CTYPE_H
#define GDB_SAFE_CTYPE_H

/* After safe-ctype.h is included, we can no longer use the host's
   ctype routines.  Trying to do so results in compile errors.  Code
   that uses safe-ctype.h that wants to refer to the locale-dependent
   ctype functions must call these wrapper versions instead.
   When compiling in C++ mode, also include <locale> before "safe-ctype.h"
   which also defines is* functions.  */

static inline int
gdb_isprint (int ch)
{
  return isprint (ch);
}

/* readline.h defines these symbols too, but we want libiberty's
   versions.  */
#undef ISALPHA
#undef ISALNUM
#undef ISDIGIT
#undef ISLOWER
#undef ISPRINT
#undef ISUPPER
#undef ISXDIGIT

#include <locale>
#include "safe-ctype.h"

#endif
