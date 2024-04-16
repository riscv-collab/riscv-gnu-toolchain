/* Obstack wrapper for GDB.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "gdb_obstack.h"

/* Concatenate NULL terminated variable argument list of `const char *'
   strings; return the new string.  Space is found in the OBSTACKP.
   Argument list must be terminated by a sentinel expression `(char *)
   NULL'.  */

char *
obconcat (struct obstack *obstackp, ...)
{
  va_list ap;

  va_start (ap, obstackp);
  for (;;)
    {
      const char *s = va_arg (ap, const char *);

      if (s == NULL)
	break;

      obstack_grow_str (obstackp, s);
    }
  va_end (ap);
  obstack_1grow (obstackp, 0);

  return (char *) obstack_finish (obstackp);
}
