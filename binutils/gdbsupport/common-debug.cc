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

#include "common-defs.h"
#include "common-debug.h"

/* See gdbsupport/common-debug.h.  */

bool show_debug_regs;

/* See gdbsupport/common-debug.h.  */

void
debug_printf (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  debug_vprintf (fmt, ap);
  va_end (ap);
}

/* See gdbsupport/common-debug.h.  */

void
debug_prefixed_printf (const char *module, const char *func,
		       const char *format, ...)
{
  va_list ap;

  va_start (ap, format);
  debug_prefixed_vprintf (module, func, format, ap);
  va_end (ap);
}

/* See gdbsupport/common-debug.h.  */

void
debug_prefixed_vprintf (const char *module, const char *func,
			const char *format, va_list args)
{
  if (func != nullptr)
    debug_printf ("%*s[%s] %s: ", debug_print_depth * 2, "", module, func);
  else
    debug_printf ("%*s[%s] ", debug_print_depth * 2, "", module);

  debug_vprintf (format, args);
  debug_printf ("\n");
}
