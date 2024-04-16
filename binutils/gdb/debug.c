/* Debug printing functions.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-debug.h"

/* See gdbsupport/common-debug.h.  */

int debug_print_depth = 0;

/* See gdbsupport/common-debug.h.  */

void
debug_vprintf (const char *fmt, va_list ap)
{
  gdb_vprintf (gdb_stdlog, fmt, ap);
}
