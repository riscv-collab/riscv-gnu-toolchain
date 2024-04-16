/* Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "gdb_regex.h"
#include "gdbsupport/def-vector.h"

compiled_regex::compiled_regex (const char *regex, int cflags,
				const char *message)
{
  gdb_assert (regex != NULL);
  gdb_assert (message != NULL);

  int code = regcomp (&m_pattern, regex, cflags);
  if (code != 0)
    {
      size_t length = regerror (code, &m_pattern, NULL, 0);
      gdb::def_vector<char> err (length);

      regerror (code, &m_pattern, err.data (), length);
      error (("%s: %s"), message, err.data ());
    }
}

compiled_regex::~compiled_regex ()
{
  regfree (&m_pattern);
}

int
compiled_regex::exec (const char *string, size_t nmatch,
		      regmatch_t pmatch[], int eflags) const
{
  return regexec (&m_pattern, string, nmatch, pmatch, eflags);
}

int
compiled_regex::search (const char *string,
			int length, int start, int range,
			struct re_registers *regs)
{
  return re_search (&m_pattern, string, length, start, range, regs);
}
