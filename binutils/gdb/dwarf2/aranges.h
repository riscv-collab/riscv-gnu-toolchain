/* DWARF aranges handling

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_ARANGES_H
#define GDB_DWARF2_ARANGES_H

class dwarf2_per_objfile;
class dwarf2_section_info;
class addrmap;

/* Read the address map data from DWARF-5 .debug_aranges, and use it
   to populate given addrmap.  Returns true on success, false on
   failure.  */

extern bool read_addrmap_from_aranges (dwarf2_per_objfile *per_objfile,
				       dwarf2_section_info *section,
				       addrmap *mutable_map,
				       deferred_warnings *warn);

#endif /* GDB_DWARF2_ARANGES_H */
