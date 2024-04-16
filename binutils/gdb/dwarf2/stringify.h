/* DWARF stringify code

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_STRINGIFY_H
#define GDB_DWARF2_STRINGIFY_H

/* Convert a DIE tag into its string name.  */
extern const char *dwarf_tag_name (unsigned tag);

/* Convert a DWARF attribute code into its string name.  */
extern const char *dwarf_attr_name (unsigned attr);

/* Convert a DWARF value form code into its string name.  */
extern const char *dwarf_form_name (unsigned form);

/* Convert a boolean to a string form.  */
extern const char *dwarf_bool_name (unsigned mybool);

/* Convert a DWARF type code into its string name.  */
extern const char *dwarf_type_encoding_name (unsigned enc);

/* Convert a DWARF unit type into its string name.  */
extern const char *dwarf_unit_type_name (int unit_type);

#endif /* GDB_DWARF2_STRINGIFY_H */
