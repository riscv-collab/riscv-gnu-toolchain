/* Some commonly-used VEC types.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "gdb_vecs.h"
#include "host-defs.h"

/* Worker function to split character delimiter separated string of fields
   STR into a char pointer vector.  */

static void
delim_string_to_char_ptr_vec_append
  (std::vector<gdb::unique_xmalloc_ptr<char>> *vecp, const char *str,
   char delimiter)
{
  do
    {
      size_t this_len;
      const char *next_field;
      char *this_field;

      next_field = strchr (str, delimiter);
      if (next_field == NULL)
	this_len = strlen (str);
      else
	{
	  this_len = next_field - str;
	  next_field++;
	}

      this_field = (char *) xmalloc (this_len + 1);
      memcpy (this_field, str, this_len);
      this_field[this_len] = '\0';
      vecp->emplace_back (this_field);

      str = next_field;
    }
  while (str != NULL);
}

/* See gdb_vecs.h.  */

std::vector<gdb::unique_xmalloc_ptr<char>>
delim_string_to_char_ptr_vec (const char *str, char delimiter)
{
  std::vector<gdb::unique_xmalloc_ptr<char>> retval;
  
  delim_string_to_char_ptr_vec_append (&retval, str, delimiter);

  return retval;
}

/* See gdb_vecs.h.  */

void
dirnames_to_char_ptr_vec_append
  (std::vector<gdb::unique_xmalloc_ptr<char>> *vecp, const char *dirnames)
{
  delim_string_to_char_ptr_vec_append (vecp, dirnames, DIRNAME_SEPARATOR);
}

/* See gdb_vecs.h.  */

std::vector<gdb::unique_xmalloc_ptr<char>>
dirnames_to_char_ptr_vec (const char *dirnames)
{
  std::vector<gdb::unique_xmalloc_ptr<char>> retval;
  
  dirnames_to_char_ptr_vec_append (&retval, dirnames);

  return retval;
}
