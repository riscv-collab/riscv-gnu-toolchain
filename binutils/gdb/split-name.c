/* Split a symbol name.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "split-name.h"
#include "cp-support.h"

/* See split-name.h.  */

std::vector<std::string_view>
split_name (const char *name, split_style style)
{
  std::vector<std::string_view> result;
  unsigned int previous_len = 0;

  switch (style)
    {
    case split_style::CXX:
      for (unsigned int current_len = cp_find_first_component (name);
	   name[current_len] != '\0';
	   current_len += cp_find_first_component (name + current_len))
	{
	  gdb_assert (name[current_len] == ':');
	  result.emplace_back (&name[previous_len],
			       current_len - previous_len);
	  /* Skip the '::'.  */
	  current_len += 2;
	  previous_len = current_len;
	}
      break;

    case split_style::DOT_STYLE:
      /* D and Go-style names.  */
      for (const char *iter = strchr (name, '.');
	   iter != nullptr;
	   iter = strchr (iter, '.'))
	{
	  result.emplace_back (&name[previous_len],
			       iter - &name[previous_len]);
	  ++iter;
	  previous_len = iter - name;
	}
      break;

    default:
      break;
    }

  result.emplace_back (&name[previous_len]);
  return result;
}

