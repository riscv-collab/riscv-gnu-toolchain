/* Common internal types for the DWARF reader

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef DWARF2_TYPES_H
#define DWARF2_TYPES_H

#include "gdbsupport/offset-type.h"
#include "gdbsupport/underlying.h"

/* Offset relative to the start of its containing CU (compilation
   unit).  */
DEFINE_OFFSET_TYPE (cu_offset, unsigned int);

/* Offset relative to the start of its .debug_info or .debug_types
   section.  */
DEFINE_OFFSET_TYPE (sect_offset, uint64_t);

static inline char *
sect_offset_str (sect_offset offset)
{
  return hex_string (to_underlying (offset));
}

#endif /* DWARF2_TYPES_H */
