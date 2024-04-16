/* Exception (throw catch) mechanism, for GDB, the GNU debugger.

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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "ui-out.h"

/* If E is an exception, print it's error message on the specified
   stream.  For _fprintf, prefix the message with PREFIX...  */
extern void exception_print (struct ui_file *file,
			     const struct gdb_exception &e);
extern void exception_fprintf (struct ui_file *file,
			       const struct gdb_exception &e,
			       const char *prefix,
			       ...) ATTRIBUTE_PRINTF (3, 4);

#endif
