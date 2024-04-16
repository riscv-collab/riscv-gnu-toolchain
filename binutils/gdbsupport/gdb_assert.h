/* GDB-friendly replacement for <assert.h>.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_ASSERT_H
#define COMMON_GDB_ASSERT_H

#include "errors.h"

/* PRAGMATICS: "gdb_assert.h":gdb_assert() is a lower case (rather
   than upper case) macro since that provides the closest fit to the
   existing lower case macro <assert.h>:assert() that it is
   replacing.  */

#define gdb_assert(expr)                                                      \
  ((void) ((expr) ? 0 :                                                       \
	   (gdb_assert_fail (#expr, __FILE__, __LINE__, __func__), 0)))

/* This prints an "Assertion failed" message, asking the user if they
   want to continue, dump core, or just exit.  */
#define gdb_assert_fail(assertion, file, line, function)                      \
  internal_error_loc (file, line, _("%s: Assertion `%s' failed."),                \
		      function, assertion)

/* The canonical form of gdb_assert (0).
   MESSAGE is a string to include in the error message.  */

#define gdb_assert_not_reached(message, ...) \
  internal_error_loc (__FILE__, __LINE__, _("%s: " message), __func__, \
		      ##__VA_ARGS__)

#endif /* COMMON_GDB_ASSERT_H */
