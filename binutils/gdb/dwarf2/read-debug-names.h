/* Reading code for .debug_names

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#ifndef DWARF2_READ_DEBUG_NAMES_H
#define DWARF2_READ_DEBUG_NAMES_H

struct dwarf2_per_objfile;

/* Read .debug_names.  If everything went ok, initialize the "quick"
   elements of all the CUs and return true.  Otherwise, return false.  */

bool dwarf2_read_debug_names (dwarf2_per_objfile *per_objfile);

#endif /* DWARF2_READ_DEBUG_NAMES_H */
