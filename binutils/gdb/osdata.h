/* Routines for handling XML generic OS data provided by target.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef OSDATA_H
#define OSDATA_H

#include <vector>

struct osdata_column
{
  osdata_column (std::string &&name_, std::string &&value_)
  : name (std::move (name_)), value (std::move (value_))
  {}

  std::string name;
  std::string value;
};

struct osdata_item
{
  std::vector<osdata_column> columns;
};

struct osdata
{
  osdata (std::string &&type_)
  : type (std::move (type_))
  {}

  std::string type;
  std::vector<osdata_item> items;
};

std::unique_ptr<osdata> osdata_parse (const char *xml);
std::unique_ptr<osdata> get_osdata (const char *type);
const std::string *get_osdata_column (const osdata_item &item,
				      const char *name);

/* Dump TYPE info to the current uiout builder.  If TYPE is either
   NULL or empty, then dump the top level table that lists the
   available types of OS data.  */
void info_osdata (const char *type);

#endif /* OSDATA_H */
